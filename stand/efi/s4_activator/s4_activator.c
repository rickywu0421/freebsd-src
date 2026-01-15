/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2024 Ricky Wu
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * FreeBSD S4 Activator - UEFI Application
 *
 * This application reads a FreeBSD S4 hibernate image from the swap partition
 * and restores the system state. The S4 image is in ELF64 format with custom
 * program headers for PCB and trampoline.
 */

#include <stand.h>
#include <sys/param.h>
#include <sys/elf64.h>
#include <sys/elf_common.h>

#include <efi.h>
#include <efilib.h>

#include <Protocol/BlockIo.h>
#include <Protocol/PartitionInfo.h>
#include <Guid/Gpt.h>

#include "s4_activator.h"

// FreeBSD Swap Partition GUID: 516e7cb5-6ecf-11d6-8ff8-00022d09712b
static EFI_GUID gFreeBSDSwapGuid = FREEBSD_SWAP_GUID;

extern void AsmTransferControl(UINT64 EntryPoint, UINT64 Context);

static void
print_status(const char *msg, EFI_STATUS status)
{
	printf("%s: %s\n", msg, efi_status_to_errno(status) == 0 ? "OK" : "Failed");
}

static EFI_STATUS
locate_swap_partition(EFI_BLOCK_IO **swap_blkio)
{
	EFI_STATUS status;
	UINTN handle_count;
	EFI_HANDLE *handle_buffer = NULL;
	EFI_PARTITION_INFO_PROTOCOL *part_info = NULL;
	EFI_BLOCK_IO *blkio = NULL;
	UINTN i;

	printf("[Activator] Locating FreeBSD swap partition...\n");

	// Locate all BlockIO protocol handles
	status = BS->LocateHandleBuffer(ByProtocol,
					&gEfiBlockIoProtocolGuid,
					NULL,
					&handle_count,
					&handle_buffer);
	if (EFI_ERROR(status)) {
		printf("Error: LocateHandleBuffer failed\n");
		return (status);
	}

	printf("[Debug] Found %zu BlockIO handles\n", handle_count);

	// Iterate through handles to find FreeBSD swap partition
	for (i = 0; i < handle_count; i++) {
		status = BS->HandleProtocol(handle_buffer[i],
					    &gEfiPartitionInfoProtocolGuid,
					    (void **)&part_info);
		if (EFI_ERROR(status)) {
			continue;
		}

		// Check if this is a GPT partition
		if (part_info->Type == PARTITION_TYPE_GPT) {
			// Compare partition type GUID with FreeBSD swap GUID
			if (memcmp(&gFreeBSDSwapGuid,
				   &part_info->Info.Gpt.PartitionTypeGUID,
				   sizeof(EFI_GUID)) == 0) {

				status = BS->HandleProtocol(handle_buffer[i],
							    &gEfiBlockIoProtocolGuid,
							    (void **)&blkio);
				if (!EFI_ERROR(status)) {
					printf("[Activator] Found FreeBSD Swap Partition\n");
					*swap_blkio = blkio;
					BS->FreePool(handle_buffer);
					return (EFI_SUCCESS);
				}
			}
		}
	}

	if (handle_buffer)
		BS->FreePool(handle_buffer);

	printf("Error: FreeBSD Swap Partition not found\n");
	return (EFI_NOT_FOUND);
}

static EFI_STATUS
validate_elf_header(Elf64_Ehdr *ehdr)
{
	// Check ELF magic number
	if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
	    ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
	    ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
	    ehdr->e_ident[EI_MAG3] != ELFMAG3) {
		printf("Error: Invalid ELF magic number\n");
		return (EFI_INVALID_PARAMETER);
	}

	// Check for 64-bit ELF
	if (ehdr->e_ident[EI_CLASS] != ELFCLASS64) {
		printf("Error: Not a 64-bit ELF file\n");
		return (EFI_INVALID_PARAMETER);
	}

	// Check for little-endian
	if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
		printf("Error: Not a little-endian ELF file\n");
		return (EFI_INVALID_PARAMETER);
	}

	// Check for x86_64 architecture
	if (ehdr->e_machine != EM_X86_64) {
		printf("Error: Not an x86_64 ELF file\n");
		return (EFI_INVALID_PARAMETER);
	}

	printf("[Activator] ELF header validation passed\n");
	return (EFI_SUCCESS);
}

static EFI_STATUS
read_blocks_aligned(EFI_BLOCK_IO *blkio, UINT64 offset, UINTN size, void *buffer)
{
	EFI_STATUS status;
	UINT32 media_id = blkio->Media->MediaId;
	UINT32 block_size = blkio->Media->BlockSize;
	EFI_LBA start_lba = offset / block_size;
	UINTN offset_in_block = offset % block_size;
	UINTN total_read_size = offset_in_block + size;

	// Align to block size
	if (total_read_size % block_size != 0)
		total_read_size = ((total_read_size / block_size) + 1) * block_size;

	// Allocate temporary buffer
	void *temp_buffer = malloc(total_read_size);
	if (!temp_buffer)
		return (EFI_OUT_OF_RESOURCES);

	// Read blocks
	status = blkio->ReadBlocks(blkio, media_id, start_lba,
				   total_read_size, temp_buffer);
	if (EFI_ERROR(status)) {
		free(temp_buffer);
		return (status);
	}

	// Copy the relevant portion
	memcpy(buffer, (UINT8 *)temp_buffer + offset_in_block, size);
	free(temp_buffer);

	return (EFI_SUCCESS);
}

static EFI_STATUS
load_s4_image(EFI_BLOCK_IO *swap_blkio,
	      EFI_PHYSICAL_ADDRESS *pcb_paddr,
	      EFI_PHYSICAL_ADDRESS *trampoline_paddr)
{
	EFI_STATUS status;
	Elf64_Ehdr ehdr;
	Elf64_Phdr *phdr_table = NULL;
	UINT16 i;
	UINT32 block_size = swap_blkio->Media->BlockSize;

	*pcb_paddr = 0;
	*trampoline_paddr = 0;

	// Phase 1: Read ELF header
	printf("[Activator] Reading ELF header...\n");
	status = read_blocks_aligned(swap_blkio, 0, sizeof(Elf64_Ehdr), &ehdr);
	if (EFI_ERROR(status)) {
		printf("Error: Failed to read ELF header\n");
		return (status);
	}

	// Validate ELF header
	status = validate_elf_header(&ehdr);
	if (EFI_ERROR(status))
		return (status);

	// Phase 2: Read program headers
	printf("[Activator] Reading %d program headers...\n", ehdr.e_phnum);
	UINTN phdr_table_size = ehdr.e_phnum * ehdr.e_phentsize;
	phdr_table = malloc(phdr_table_size);
	if (!phdr_table) {
		printf("Error: Out of memory\n");
		return (EFI_OUT_OF_RESOURCES);
	}

	status = read_blocks_aligned(swap_blkio, ehdr.e_phoff,
				     phdr_table_size, phdr_table);
	if (EFI_ERROR(status)) {
		printf("Error: Failed to read program headers\n");
		free(phdr_table);
		return (status);
	}

	// Phase 3: Process program headers
	for (i = 0; i < ehdr.e_phnum; i++) {
		Elf64_Phdr *phdr = &phdr_table[i];
		EFI_PHYSICAL_ADDRESS paddr = phdr->p_paddr;
		UINTN file_size = phdr->p_filesz;
		UINTN mem_size = phdr->p_memsz;
		UINTN segment_offset = phdr->p_offset;

		switch (phdr->p_type) {
		case PT_LOAD: {
			UINTN pages = EFI_SIZE_TO_PAGES(mem_size);

			printf("[Activator] Segment %d: PT_LOAD at 0x%lx (%zu pages)\n",
			       i, paddr, pages);

			// Allocate pages at specific physical address
			status = BS->AllocatePages(AllocateAddress,
						   EfiLoaderData,
						   pages,
						   &paddr);
			if (EFI_ERROR(status)) {
				printf("Error: Failed to allocate pages at 0x%lx\n", paddr);
				goto error_exit;
			}

			// Read segment data
			// Assume segment offset is block-aligned
			EFI_LBA start_lba = segment_offset / block_size;
			UINTN total_read_size = file_size;
			if (file_size % block_size != 0)
				total_read_size = ((file_size / block_size) + 1) * block_size;

			status = swap_blkio->ReadBlocks(swap_blkio,
							swap_blkio->Media->MediaId,
							start_lba,
							total_read_size,
							(void *)paddr);
			if (EFI_ERROR(status)) {
				printf("Error: Failed to read PT_LOAD segment\n");
				goto error_exit;
			}

			// Zero out extra bytes and BSS
			if (file_size % block_size != 0) {
				UINTN extra_size = block_size - (file_size % block_size);
				memset((UINT8 *)paddr + file_size, 0, extra_size);
			}

			printf("[Activator] Successfully loaded PT_LOAD segment\n");
			break;
		}

		case PT_FREEBSD_S4_PCB: {
			UINTN pages = EFI_SIZE_TO_PAGES(file_size);

			printf("[Activator] Segment %d: PT_FREEBSD_S4_PCB\n", i);

			// Allocate pages for PCB
			status = BS->AllocatePages(AllocateAnyPages,
						   EfiLoaderData,
						   pages,
						   pcb_paddr);
			if (EFI_ERROR(status)) {
				printf("Error: Failed to allocate pages for PCB\n");
				goto error_exit;
			}

			// Read PCB data
			EFI_LBA start_lba = segment_offset / block_size;
			UINTN total_read_size = file_size;
			if (file_size % block_size != 0)
				total_read_size = ((file_size / block_size) + 1) * block_size;

			status = swap_blkio->ReadBlocks(swap_blkio,
							swap_blkio->Media->MediaId,
							start_lba,
							total_read_size,
							(void *)*pcb_paddr);
			if (EFI_ERROR(status)) {
				printf("Error: Failed to read PCB segment\n");
				goto error_exit;
			}

			// Zero extra bytes
			if (file_size % block_size != 0) {
				UINTN extra_size = block_size - (file_size % block_size);
				memset((UINT8 *)*pcb_paddr + file_size, 0, extra_size);
			}

			printf("[Activator] PCB loaded at: 0x%lx\n", *pcb_paddr);
			break;
		}

		case PT_FREEBSD_S4_TRAMPOLINE:
			printf("[Activator] Segment %d: PT_FREEBSD_S4_TRAMPOLINE\n", i);
			*trampoline_paddr = paddr;
			break;

		default:
			break;
		}
	}

	free(phdr_table);

	// Verify we have both PCB and trampoline
	if (*pcb_paddr == 0 || *trampoline_paddr == 0) {
		printf("Error: Missing PCB or trampoline in S4 image\n");
		return (EFI_INVALID_PARAMETER);
	}

	return (EFI_SUCCESS);

error_exit:
	if (phdr_table)
		free(phdr_table);
	return (status);
}

static EFI_STATUS
exit_boot_services_and_jump(EFI_PHYSICAL_ADDRESS trampoline_paddr,
			    EFI_PHYSICAL_ADDRESS pcb_paddr)
{
	EFI_STATUS status;
	UINTN map_size = 0;
	EFI_MEMORY_DESCRIPTOR *memory_map = NULL;
	UINTN map_key;
	UINTN descriptor_size;
	UINT32 descriptor_version;

	printf("[Activator] Preparing to exit boot services...\n");

	// Get memory map size
	status = BS->GetMemoryMap(&map_size, memory_map, &map_key,
				  &descriptor_size, &descriptor_version);
	if (status != EFI_BUFFER_TOO_SMALL) {
		printf("Error: GetMemoryMap failed\n");
		return (status);
	}

	// Allocate buffer for memory map (add extra space for new allocations)
	map_size += 8 * descriptor_size;
	memory_map = malloc(map_size);
	if (!memory_map) {
		printf("Error: Out of memory for memory map\n");
		return (EFI_OUT_OF_RESOURCES);
	}

	// Get actual memory map
	status = BS->GetMemoryMap(&map_size, memory_map, &map_key,
				  &descriptor_size, &descriptor_version);
	if (EFI_ERROR(status)) {
		printf("Error: GetMemoryMap failed\n");
		free(memory_map);
		return (status);
	}

	// Exit boot services
	printf("[Activator] Exiting boot services...\n");
	status = efi_exit_boot_services(map_key);
	if (EFI_ERROR(status)) {
		// Retry once with updated memory map
		status = BS->GetMemoryMap(&map_size, memory_map, &map_key,
					  &descriptor_size, &descriptor_version);
		if (!EFI_ERROR(status)) {
			status = efi_exit_boot_services(map_key);
		}
	}

	if (EFI_ERROR(status)) {
		printf("Error: Failed to exit boot services\n");
		return (status);
	}

	// Boot services are now inactive, jump to trampoline
	printf("[Activator] Jumping to S4 trampoline at 0x%lx\n", trampoline_paddr);
	AsmTransferControl(trampoline_paddr, pcb_paddr);

	// Should never reach here
	return (EFI_LOAD_ERROR);
}

EFI_STATUS
main(int argc __unused, CHAR16 *argv[] __unused)
{
	EFI_STATUS status;
	EFI_BLOCK_IO *swap_blkio = NULL;
	EFI_PHYSICAL_ADDRESS pcb_paddr = 0;
	EFI_PHYSICAL_ADDRESS trampoline_paddr = 0;

	printf("FreeBSD S4 Activator - UEFI Application\n");
	printf("========================================\n\n");

	// Step 1: Locate swap partition
	status = locate_swap_partition(&swap_blkio);
	if (EFI_ERROR(status))
		return (status);

	// Step 2: Load S4 image from swap partition
	status = load_s4_image(swap_blkio, &pcb_paddr, &trampoline_paddr);
	if (EFI_ERROR(status))
		return (status);

	printf("\n[Activator] S4 image loaded successfully\n");
	printf("Trampoline: 0x%lx\n", trampoline_paddr);
	printf("PCB:        0x%lx\n\n", pcb_paddr);

	// Step 3: Exit boot services and jump to trampoline
	status = exit_boot_services_and_jump(trampoline_paddr, pcb_paddr);

	// Should never reach here
	printf("Error: Returned from trampoline!\n");
	return (status);
}
