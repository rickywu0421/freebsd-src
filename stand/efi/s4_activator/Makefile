# $FreeBSD$
#
# S4 Activator - UEFI application for FreeBSD S4 hibernate resume
#

.include <bsd.init.mk>

PROG=		s4_activator.sym
INTERNALPROG=
WARNS?=		3

# Source files
SRCS=		s4_activator.c

# NASM assembly for trampoline
SRCS+=		trampoline.nasm

# Include paths
CFLAGS+=	-I${EFIINC} -I${EDK2INC}
CFLAGS+=	-I${EFIINCMD}
CFLAGS+=	-I${LDRSRC}
CFLAGS+=	-DEFI

.PATH:		${EFISRC}/loader/arch/${MACHINE}
.PATH:		${EFISRC}/loader
.PATH:		${LDRSRC}

# Output EFI binary
FILES=		s4_activator.efi
FILESMODE_s4_activator.efi=	${BINMODE}

# Linker script (use same as loader)
LDSCRIPT=	${EFISRC}/loader/arch/${MACHINE}/${MACHINE}.ldscript
LDFLAGS+=	-Wl,-T${LDSCRIPT},-Bsymbolic,-znotext -pie

.if ${MACHINE_CPUARCH} == "amd64"
CFLAGS+=	-fPIC
LDFLAGS+=	-Wl,-znocombreloc
.endif

# NASM assembly compilation
.SUFFIXES: .nasm .o

trampoline.o: trampoline.nasm
	${NASM} -f elf64 -o ${.TARGET} ${.ALLSRC}

# Libraries
LIBEFI=		${BOOTOBJ}/efi/libefi/libefi.a

DPADD+=		${LIBEFI} ${LIBSA}
LDADD+=		${LIBEFI} ${LIBSA}
DPADD+=		${LDSCRIPT}

CLEANFILES+=	s4_activator.efi

# Convert ELF to EFI PE/COFF format
s4_activator.efi: ${PROG}
	@if ${NM} ${.ALLSRC} | grep ' U '; then \
		echo "Undefined symbols in ${.ALLSRC}"; \
		exit 1; \
	fi
	SOURCE_DATE_EPOCH=${SOURCE_DATE_EPOCH} \
	${EFI_OBJCOPY} -j .peheader -j .text -j .sdata -j .data \
		-j .dynamic -j .dynsym -j .rel.dyn \
		-j .rela.dyn -j .reloc -j .eh_frame \
		--output-target=${EFI_TARGET} ${.ALLSRC} ${.TARGET}

.include <bsd.prog.mk>
