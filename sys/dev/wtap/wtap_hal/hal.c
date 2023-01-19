/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2010-2011 Monthadar Al Jaberi, TerraNet AB
 * All rights reserved.
 *
 * Copyright (c) 2023 The FreeBSD Foundation
 *
 * Portions of this software were developed by En-Wei Wu
 * under sponsorship from the FreeBSD Foundation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    similar to the "NO WARRANTY" disclaimer below ("Disclaimer") and any
 *    redistribution must be conditioned upon including a substantially
 *    similar Disclaimer requirement for further binary redistribution.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF NONINFRINGEMENT, MERCHANTIBILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGES.
 *
 * $FreeBSD$
 */
#include "hal.h"
#include "../if_medium.h"
#include "handler.h"

static void
hal_tx_proc(void *arg, int npending)
{
	struct wtap_hal *hal = (struct wtap_hal *)arg;
	struct packet *p;

#if 0
	DWTAP_PRINTF("%s\n", __func__);
#endif

	hal = (struct wtap_hal *)arg;
	for(;;){
		p = medium_get_next_packet(hal->hal_md);
		if(p == NULL)
		return;

		hal->plugin->work(hal->plugin, p);

#if 0
		DWTAP_PRINTF("[%d] freeing m=%p\n", p->id, p->m);
#endif
		m_free(p->m);
		free(p, M_WTAP_PACKET);
	}
}

void
init_hal(struct wtap_hal *hal)
{

	DWTAP_PRINTF("%s\n", __func__);
	memset(hal->hal_devs_set, 0, sizeof(uint32_t) * ARRAY_SIZE);
	mtx_init(&hal->hal_mtx, "wtap_hal mtx", NULL, MTX_DEF | MTX_RECURSE);

	hal->hal_md = (struct wtap_medium *)malloc(sizeof(struct wtap_medium),
	    M_WTAP, M_NOWAIT | M_ZERO);

	init_medium(hal->hal_md);
	/* register event handler for packets */
	TASK_INIT(&hal->hal_md->tx_handler->proc, 0, hal_tx_proc, hal);

	callout_init_mtx(&hal->hw.timer_intr, &hal->hal_mtx, 0);
	hal->hw.timer_intr_intval = msecs_to_ticks(HAL_TIMER_INTVAL);
}

void
register_plugin(struct wtap_hal *hal, struct wtap_plugin *plugin)
{

	plugin->init(plugin);
	hal->plugin = plugin;
}

void
deregister_plugin(struct wtap_hal *hal)
{

	hal->plugin->deinit(hal->plugin);
	hal->plugin = NULL; /* catch illegal usages */
}

void
deinit_hal(struct wtap_hal *hal)
{

	DWTAP_PRINTF("%s\n", __func__);
	deinit_medium(hal->hal_md);
	free(hal->hal_md, M_WTAP);
	mtx_destroy(&hal->hal_mtx);
}

int32_t
new_wtap(struct wtap_hal *hal, int32_t id)
{
	static const uint8_t mac_pool[MAX_NBR_WTAP][IEEE80211_ADDR_LEN] = {
	    {0,152,154,152,150,151},
	    {0,152,154,152,150,152},
	    {0,152,154,152,150,153},
	    {0,152,154,152,150,154},
	    {0,152,154,152,150,155},
	    {0,152,154,152,150,156},
	    {0,152,154,152,150,157},
	    {0,152,154,152,150,158},
	    {0,152,154,152,151,151},
	    {0,152,154,152,151,152},
	    {0,152,154,152,151,153},
	    {0,152,154,152,151,154},
	    {0,152,154,152,151,155},
	    {0,152,154,152,151,156},
	    {0,152,154,152,151,157},
	    {0,152,154,152,151,158},
	    {0,152,154,152,152,151},
	    {0,152,154,152,152,152},
	    {0,152,154,152,152,153},
	    {0,152,154,152,152,154},
	    {0,152,154,152,152,155},
	    {0,152,154,152,152,156},
	    {0,152,154,152,152,157},
	    {0,152,154,152,152,158},
	    {0,152,154,152,153,151},
	    {0,152,154,152,153,152},
	    {0,152,154,152,153,153},
	    {0,152,154,152,153,154},
	    {0,152,154,152,153,155},
	    {0,152,154,152,153,156},
	    {0,152,154,152,153,157},
	    {0,152,154,152,153,158},
	    {0,152,154,152,154,151},
	    {0,152,154,152,154,152},
	    {0,152,154,152,154,153},
	    {0,152,154,152,154,154},
	    {0,152,154,152,154,155},
	    {0,152,154,152,154,156},
	    {0,152,154,152,154,157},
	    {0,152,154,152,154,158},
	    {0,152,154,152,155,151},
	    {0,152,154,152,155,152},
	    {0,152,154,152,155,153},
	    {0,152,154,152,155,154},
	    {0,152,154,152,155,155},
	    {0,152,154,152,155,156},
	    {0,152,154,152,155,157},
	    {0,152,154,152,155,158},
	    {0,152,154,152,156,151},
	    {0,152,154,152,156,152},
	    {0,152,154,152,156,153},
	    {0,152,154,152,156,154},
	    {0,152,154,152,156,155},
	    {0,152,154,152,156,156},
	    {0,152,154,152,156,157},
	    {0,152,154,152,156,158},
	    {0,152,154,152,157,151},
	    {0,152,154,152,157,152},
	    {0,152,154,152,157,153},
	    {0,152,154,152,157,154},
	    {0,152,154,152,157,155},
	    {0,152,154,152,157,156},
	    {0,152,154,152,157,157},
	    {0,152,154,152,157,158}
	    };

	DWTAP_PRINTF("%s\n", __func__);

	/* When id < 0, automate the assignmanet of wtap device by 
	 * finding the least significant clear bit in hal->hal_devs_set
	 */
	if (id < 0) {
		int i;
		for (i = 0; i < MAX_NBR_WTAP; i++) {
			if (!isset(hal->hal_devs_set, i)) {
				id = i;
				break;
			}
		}
		
		if (i == MAX_NBR_WTAP) {
			DWTAP_PRINTF("%s: wtap device is full\n", __func__);
			return -1;
		}
	}

	uint8_t const *macaddr = mac_pool[id];
	if (isset(hal->hal_devs_set, id)) {
		printf("error, wtap_id=%d already created\n", id);
		return -1;
	}

	if (id >= MAX_NBR_WTAP) {
		DWTAP_PRINTF("error, wtap_id=%d must be between 0 and 63\n", id);
		return -1;
	}

	setbit(hal->hal_devs_set, id);
	hal->hal_devs[id] = (struct wtap_softc *)malloc(
	    sizeof(struct wtap_softc), M_WTAP, M_NOWAIT | M_ZERO);
	hal->hal_devs[id]->sc_md = hal->hal_md;
	hal->hal_devs[id]->id = id;
	hal->hal_devs[id]->hal = hal;
	snprintf(hal->hal_devs[id]->name, sizeof(hal->hal_devs[id]->name),
	    "wtap%d", id);
	mtx_init(&hal->hal_devs[id]->sc_mtx, "wtap_softc mtx", NULL,
	    MTX_DEF | MTX_RECURSE);

	if(wtap_attach(hal->hal_devs[id], macaddr)){
		printf("%s, cant alloc new wtap\n", __func__);
		return -1;
	}

	return id;
}

int32_t
free_wtap(struct wtap_hal *hal, int32_t id)
{

	DWTAP_PRINTF("%s\n", __func__);

	if (id < 0 || id >= MAX_NBR_WTAP) {
		DWTAP_PRINTF("error, wtap_id=%d must be between 0 and 63\n", id);
		return -1;
	}

	if(!isset(hal->hal_devs_set, id)){
		printf("error, wtap_id=%d never created\n", id);
		return -1;
	}

	if(wtap_detach(hal->hal_devs[id]))
		printf("%s, cant alloc new wtap\n", __func__);

	clrbit(hal->hal_devs_set, id);
	mtx_destroy(&hal->hal_devs[id]->sc_mtx);
	free(hal->hal_devs[id], M_WTAP);
	hal->hal_devs[id] = NULL;
	return 0;
}

void 
wtap_hal_timer_intr(void *arg)
{
	struct wtap_hal *hal = arg;
	uint32_t intval = hal->hw.timer_intr_intval;

	hal->hw.tsf += ticks_to_msecs(intval);

	callout_schedule(&hal->hw.timer_intr, intval);
}

void 
wtap_hal_reset_tsf(struct wtap_hal *hal)
{
	mtx_lock(&hal->hal_mtx);

	callout_stop(&hal->hw.timer_intr);
	hal->hw.tsf = 0;
	callout_reset(&hal->hw.timer_intr, hal->hw.timer_intr_intval,
		wtap_hal_timer_intr, hal);

	mtx_unlock(&hal->hal_mtx);
}

uint64_t 
wtap_hal_get_tsf(struct wtap_hal *hal)
{
	return (hal->hw.tsf);
}
