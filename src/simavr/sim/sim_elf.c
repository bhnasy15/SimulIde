/*
	sim_elf.c

	Loads a .elf file, extract the code, the data, the eeprom and
	the "mcu" specification section, also load usable code symbols
	to be able to print meaningful trace information.

	Copyright 2008, 2009 Michel Pollet <buserror@gmail.com>

 	This file is part of simavr.

	simavr is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	simavr is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with simavr.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sim_elf.h"
#include "sim_vcd_file.h"
#include "avr_eeprom.h"
#include "avr_ioport.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

int
avr_load_firmware(
		avr_t * avr,
		elf_firmware_t * firmware)
{
    if (firmware->frequency) avr->frequency = firmware->frequency;
    if (firmware->vcc)       avr->vcc = firmware->vcc;
    if (firmware->avcc)      avr->avcc = firmware->avcc;
    if (firmware->aref)      avr->aref = firmware->aref;

    if( avr_loadcode(avr, firmware->flash,
            firmware->flashsize, firmware->flashbase) != 0){
            return -1;
    }
	avr->codeend = firmware->flashsize +
			firmware->flashbase - firmware->datasize;

	if (firmware->eeprom && firmware->eesize) {
		avr_eeprom_desc_t d = {
				.ee = firmware->eeprom,
				.offset = 0,
				.size = firmware->eesize
		};
		avr_ioctl(avr, AVR_IOCTL_EEPROM_SET, &d);
	}
	if (firmware->fuse)
		memcpy(avr->fuse, firmware->fuse, firmware->fusesize);
	if (firmware->lockbits)
		avr->lockbits = firmware->lockbits[0];
	// load the default pull up/down values for ports
	for (int i = 0; i < 8 && firmware->external_state[i].port; i++) {
		avr_ioport_external_t e = {
			.name = firmware->external_state[i].port,
			.mask = firmware->external_state[i].mask,
			.value = firmware->external_state[i].value,
		};
		avr_ioctl(avr, AVR_IOCTL_IOPORT_SET_EXTERNAL(e.name), &e);
	}
	avr_set_command_register(avr, firmware->command_register_addr);
	avr_set_console_register(avr, firmware->console_register_addr);

	// rest is initialization of the VCD file
	if (firmware->tracecount == 0)
        return 0;
	avr->vcd = malloc(sizeof(*avr->vcd));
	memset(avr->vcd, 0, sizeof(*avr->vcd));
	avr_vcd_init(avr,
		firmware->tracename[0] ? firmware->tracename: "gtkwave_trace.vcd",
		avr->vcd,
		firmware->traceperiod >= 1000 ? firmware->traceperiod : 1000);

	AVR_LOG(avr, LOG_TRACE, "Creating VCD trace file '%s'\n",
			avr->vcd->filename);

	for (int ti = 0; ti < firmware->tracecount; ti++) {
		if (firmware->trace[ti].kind == AVR_MMCU_TAG_VCD_PORTPIN) {
			avr_irq_t * irq = avr_io_getirq(avr,
					AVR_IOCTL_IOPORT_GETIRQ(firmware->trace[ti].mask),
					firmware->trace[ti].addr);
			if (irq) {
				char name[16];
				sprintf(name, "%c%d", firmware->trace[ti].mask,
						firmware->trace[ti].addr);
				avr_vcd_add_signal(avr->vcd, irq, 1,
					firmware->trace[ti].name[0] ?
						firmware->trace[ti].name : name);
			}
		} else if (firmware->trace[ti].kind == AVR_MMCU_TAG_VCD_IRQ) {
			avr_irq_t * bit = avr_get_interrupt_irq(avr, firmware->trace[ti].mask);
			if (bit && firmware->trace[ti].addr < AVR_INT_IRQ_COUNT)
				avr_vcd_add_signal(avr->vcd,
						&bit[firmware->trace[ti].addr],
						firmware->trace[ti].mask == 0xff ? 8 : 1,
						firmware->trace[ti].name);
		} else if (firmware->trace[ti].mask == 0xff ||
				firmware->trace[ti].mask == 0) {
			// easy one
			avr_irq_t * all = avr_iomem_getirq(avr,
					firmware->trace[ti].addr,
					firmware->trace[ti].name,
					AVR_IOMEM_IRQ_ALL);
			if (!all) {
				AVR_LOG(avr, LOG_ERROR,
					"ELF: %s: unable to attach trace to address %04x\n",
					__FUNCTION__, firmware->trace[ti].addr);
			} else {
				avr_vcd_add_signal(avr->vcd, all, 8,
						firmware->trace[ti].name);
			}
		} else {
			int count = __builtin_popcount(firmware->trace[ti].mask);
		//	for (int bi = 0; bi < 8; bi++)
		//		if (firmware->trace[ti].mask & (1 << bi))
		//			count++;
			for (int bi = 0; bi < 8; bi++)
				if (firmware->trace[ti].mask & (1 << bi)) {
					avr_irq_t * bit = avr_iomem_getirq(avr,
							firmware->trace[ti].addr,
							firmware->trace[ti].name,
							bi);
					if (!bit) {
						AVR_LOG(avr, LOG_ERROR,
							"ELF: %s: unable to attach trace to address %04x\n",
							__FUNCTION__, firmware->trace[ti].addr);
						break;
					}

					if (count == 1) {
						avr_vcd_add_signal(avr->vcd,
								bit, 1, firmware->trace[ti].name);
						break;
					}
					char comp[128];
					sprintf(comp, "%s.%d", firmware->trace[ti].name, bi);
					avr_vcd_add_signal(avr->vcd,
							bit, 1, firmware->trace[ti].name);
				}
		}
	}
	// if the firmware has specified a command register, do NOT start the trace here
	// the firmware probably knows best when to start/stop it
	if (!firmware->command_register_addr)
		avr_vcd_start(avr->vcd);

    return 0;
}

