/*
	sim_vcd_file.c

	Implements a Value Change Dump file outout to generate
	traces & curves and display them in gtkwave.

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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <ctype.h>
#include "sim_vcd_file.h"
#include "sim_avr.h"
#include "sim_time.h"

DEFINE_FIFO(avr_vcd_log_t, avr_vcd_fifo);

#define strdupa(__s) strcpy(alloca(strlen(__s)+1), __s)

static void
_avr_vcd_notify(
		struct avr_irq_t * irq,
		uint32_t value,
		void * param);

int
avr_vcd_init(
		struct avr_t * avr,
		const char * filename,
		avr_vcd_t * vcd,
		uint32_t period)
{
	memset(vcd, 0, sizeof(avr_vcd_t));
	vcd->avr = avr;
	vcd->filename = strdup(filename);
	vcd->period = avr_usec_to_cycles(vcd->avr, period);

	return 0;
}

/*
 * Read some signals from the file and fill the FIFO with it, we read
 * a completely arbitrary amount of stuff to fill the FIFO reasonably well
 */
static int
avr_vcd_input_read(
		avr_vcd_t * vcd )
{
	char line[1024];

	while (fgets(line, sizeof(line), vcd->input)) {
	//	printf("%s", line);
		if (!line[0])	// technically can't happen, but make sure next line works
			continue;
        /// vcd->input_line = argv_parse(vcd->input_line, line);
        /// avr_vcd_input_parse_line(vcd, vcd->input_line);
		/* stop once the fifo is full enough */
		if (avr_vcd_fifo_get_read_size(&vcd->log) >= 128)
			break;
	}
	return avr_vcd_fifo_isempty(&vcd->log);
}

/*
 * This is called when we need to change the state of one or more IRQ,
 * so look in the FIFO to know 'our' stamp time, read as much as we can
 * that is still on that same timestamp.
 * When when the FIFO content has too far in the future, re-schedule the
 * timer for that time and shoot of.
 * Also try to top up the FIFO with new read stuff when it's drained
 */
static avr_cycle_count_t
_avr_vcd_input_timer(
		struct avr_t * avr,
		avr_cycle_count_t when,
		void * param)
{
	avr_vcd_t * vcd = param;

	// get some more if needed
	if (avr_vcd_fifo_get_read_size(&vcd->log) < (vcd->signal_count * 16))
		avr_vcd_input_read(vcd);

	if (avr_vcd_fifo_isempty(&vcd->log)) {
		printf("%s DONE but why are we here?\n", __func__);
		return 0;
	}

	avr_vcd_log_t log = avr_vcd_fifo_read_at(&vcd->log, 0);
	uint64_t stamp = log.when;
	while (!avr_vcd_fifo_isempty(&vcd->log)) {
		log = avr_vcd_fifo_read_at(&vcd->log, 0);
		if (log.when != stamp)	// leave those in the FIFO
			break;
		// we already have it
		avr_vcd_fifo_read_offset(&vcd->log, 1);
		avr_vcd_signal_p signal = &vcd->signal[log.sigindex];
		avr_raise_irq_float(&signal->irq, log.value, log.floating);
	}

	if (avr_vcd_fifo_isempty(&vcd->log)) {
		AVR_LOG(vcd->avr, LOG_TRACE,
				"%s Finished reading, ending simavr\n",
				vcd->filename);
		avr->state = cpu_Done;
		return 0;
	}
	log = avr_vcd_fifo_read_at(&vcd->log, 0);

	when += avr_usec_to_cycles(avr, log.when - stamp);

	return when;
}

int
avr_vcd_init_input(
		struct avr_t * avr,
		const char * filename, 	// filename to read
		avr_vcd_t * vcd )		// vcd struct to initialize
{
	return 0;
}

void
avr_vcd_close(
		avr_vcd_t * vcd)
{
	avr_vcd_stop(vcd);

	/* dispose of any link and hooks */
	for (int i = 0; i < vcd->signal_count; i++) {
		avr_vcd_signal_t * s = &vcd->signal[i];

		avr_free_irq(&s->irq, 1);
	}

	if (vcd->filename) {
		free(vcd->filename);
		vcd->filename = NULL;
	}
}

static char *
_avr_vcd_get_float_signal_text(
		avr_vcd_signal_t * s,
		char * out)
{
	char * dst = out;

	if (s->size > 1)
		*dst++ = 'b';

	for (int i = s->size; i > 0; i--)
		*dst++ = 'x';
	if (s->size > 1)
		*dst++ = ' ';
	*dst++ = s->alias;
	*dst = 0;
	return out;
}

static char *
_avr_vcd_get_signal_text(
		avr_vcd_signal_t * s,
		char * out,
		uint32_t value)
{
	char * dst = out;

	if (s->size > 1)
		*dst++ = 'b';

	for (int i = s->size; i > 0; i--)
		*dst++ = value & (1 << (i-1)) ? '1' : '0';
	if (s->size > 1)
		*dst++ = ' ';
	*dst++ = s->alias;
	*dst = 0;
	return out;
}

static void
avr_vcd_flush_log(
		avr_vcd_t * vcd)
{
#if AVR_VCD_MAX_SIGNALS > 32
	uint64_t seen = 0;
#else
	uint32_t seen = 0;
#endif
	uint64_t oldbase = 0;	// make sure it's different
	char out[48];

	if (avr_vcd_fifo_isempty(&vcd->log) || !vcd->output)
		return;

	while (!avr_vcd_fifo_isempty(&vcd->log)) {
		avr_vcd_log_t l = avr_vcd_fifo_read(&vcd->log);
		// 10ns base -- 100MHz should be enough
		uint64_t base = avr_cycles_to_nsec(vcd->avr, l.when - vcd->start) / 10;

		/*
		 * if that trace was seen in this nsec already, we fudge the
		 * base time to make sure the new value is offset by one nsec,
		 * to make sure we get at least a small pulse on the waveform.
		 *
		 * This is a bit of a fudge, but it is the only way to represent
		 * very short "pulses" that are still visible on the waveform.
		 */
		if (base == oldbase &&
				(seen & (1 << l.sigindex)))
			base++;	// this forces a new timestamp

		if (base > oldbase || !seen) {
			seen = 0;
			fprintf(vcd->output, "#%" PRIu64  "\n", base);
			oldbase = base;
		}
		// mark this trace as seen for this timestamp
		seen |= (1 << l.sigindex);
		fprintf(vcd->output, "%s\n",
				l.floating ?
					_avr_vcd_get_float_signal_text(
							&vcd->signal[l.sigindex],
							out) :
					_avr_vcd_get_signal_text(
							&vcd->signal[l.sigindex],
							out, l.value));
	}
}

static avr_cycle_count_t
_avr_vcd_timer(
		struct avr_t * avr,
		avr_cycle_count_t when,
		void * param)
{
	avr_vcd_t * vcd = param;
	avr_vcd_flush_log(vcd);
	return when + vcd->period;
}

static void
_avr_vcd_notify(
		struct avr_irq_t * irq,
		uint32_t value,
		void * param)
{
	avr_vcd_t * vcd = (avr_vcd_t *)param;

	if (!vcd->output)
		return;

	avr_vcd_signal_t * s = (avr_vcd_signal_t*)irq;
	avr_vcd_log_t l = {
		.sigindex = s->irq.irq,
		.when = vcd->avr->cycle,
		.value = value,
		.floating = !!(avr_irq_get_flags(irq) & IRQ_FLAG_FLOATING),
	};
	if (avr_vcd_fifo_isfull(&vcd->log)) {
		AVR_LOG(vcd->avr, LOG_WARNING,
				"%s FIFO Overload, flushing!\n",
				__func__);
		/* Decrease period by a quarter, for next time */
		vcd->period -= vcd->period >> 2;
		avr_vcd_flush_log(vcd);
	}
	avr_vcd_fifo_write(&vcd->log, l);
}

int
avr_vcd_add_signal(
		avr_vcd_t * vcd,
		avr_irq_t * signal_irq,
		int signal_bit_size,
		const char * name )
{
	if (vcd->signal_count == AVR_VCD_MAX_SIGNALS)
		return -1;
	int index = vcd->signal_count++;
	avr_vcd_signal_t * s = &vcd->signal[index];
	strncpy(s->name, name, sizeof(s->name));
	s->size = signal_bit_size;
	s->alias = ' ' + vcd->signal_count ;

	/* manufacture a nice IRQ name */
	int l = strlen(name);
	char iname[10 + l + 1];
	if (signal_bit_size > 1)
		sprintf(iname, "%d>vcd.%s", signal_bit_size, name);
	else
		sprintf(iname, ">vcd.%s", name);

	const char * names[1] = { iname };
	avr_init_irq(&vcd->avr->irq_pool, &s->irq, index, 1, names);
	avr_irq_register_notify(&s->irq, _avr_vcd_notify, vcd);

	avr_connect_irq(signal_irq, &s->irq);
	return 0;
}


int
avr_vcd_start(
		avr_vcd_t * vcd)
{
	vcd->start = vcd->avr->cycle;
	avr_vcd_fifo_reset(&vcd->log);

	if (vcd->input) {
		/*
		 * nothing to do here, the first cycle timer will take care
		 * if it.
		 */
		return 0;
	}
	if (vcd->output)
		avr_vcd_stop(vcd);
	vcd->output = fopen(vcd->filename, "w");
	if (vcd->output == NULL) {
		perror(vcd->filename);
		return -1;
	}

	fprintf(vcd->output, "$timescale 10ns $end\n");	// 10ns base, aka 100MHz
	fprintf(vcd->output, "$scope module logic $end\n");

	for (int i = 0; i < vcd->signal_count; i++) {
		fprintf(vcd->output, "$var wire %d %c %s $end\n",
			vcd->signal[i].size, vcd->signal[i].alias, vcd->signal[i].name);
	}

	fprintf(vcd->output, "$upscope $end\n");
	fprintf(vcd->output, "$enddefinitions $end\n");

	fprintf(vcd->output, "$dumpvars\n");
	for (int i = 0; i < vcd->signal_count; i++) {
		avr_vcd_signal_t * s = &vcd->signal[i];
		char out[48];
		fprintf(vcd->output, "%s\n",
				_avr_vcd_get_float_signal_text(s, out));
	}
	fprintf(vcd->output, "$end\n");
	avr_cycle_timer_register(vcd->avr, vcd->period, _avr_vcd_timer, vcd);
	return 0;
}

int
avr_vcd_stop(
		avr_vcd_t * vcd)
{
	avr_cycle_timer_cancel(vcd->avr, _avr_vcd_timer, vcd);
	avr_cycle_timer_cancel(vcd->avr, _avr_vcd_input_timer, vcd);

	avr_vcd_flush_log(vcd);

	if (vcd->input_line)
		free(vcd->input_line);
	vcd->input_line = NULL;
	if (vcd->input)
		fclose(vcd->input);
	vcd->input = NULL;
	if (vcd->output)
		fclose(vcd->output);
	vcd->output = NULL;
	return 0;
}


