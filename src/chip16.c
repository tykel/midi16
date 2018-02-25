/*
 * This file is part of midi16.
 *
 * midi16 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * midi16 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with midi16. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "chip16.h"

#define NUM_NOTES 0x80

/* Thanks to http://subsynth.sourceforge.net/midinote2freq.html */
static inline float key2hz(int key)
{
    float a = 440.0f / 32;
    return a * pow(2.0f, ((float)(key - 9) / 12));
}

int chip16_write_track(const char *fn_asm, const char *fn_notes,
                       midi_track_t *track)
{
    const char *fn_notes_alt = "mus_menu.bin";
    FILE *fnotes_alt;
    midi_event_t *evt;
    int16_t packet[4];
    /* Milliseconds per pulse */
    uint32_t mspp;
    /* For each key/note, record the start time */
    int note_start[NUM_NOTES] = {0};
    int note_end[NUM_NOTES] = {0};
    int i, total, clock, last_note_start;
    
    /* Write the notes to a separate file */
    if((fnotes_alt = fopen(fn_notes_alt, "wb")) == NULL) {
        printf("error: could not open %s for writing\n", fn_notes_alt);
        return -2;
    }
  
    memset(note_start, 0, NUM_NOTES * sizeof(int));

    mspp = track->pulse_len;
    printf("using %d ms per pulse\n", mspp); 
    evt = track->events;
    total = 0;
    clock = 0;
    last_note_start = 0;

    for(i = 0; i < track->num_events; i++) {
        clock += evt->dt;
        /* Find the next key press (on). */
        if((evt->status & 0xF0) == MIDI_CMD_NOTE_ON && evt->params[1]) {
            int16_t dur;
            int16_t delay;
            int t_start = clock;
            int t_end = clock;

            midi_event_t *e = evt->next;
            /* Find the corresponding note end (off). */
            while (e && (e->status & 0xF0) != MIDI_CMD_NOTE_OFF) {
                t_end += e->dt;
                e = e->next;
            }
            if (e) {
                t_end += e->dt;
            }

            dur = (t_end - t_start) * mspp / 16;
            delay = (t_start - last_note_start) * mspp / 16;
            if (delay > 1000) {
                printf("warning: event %d: NOTE ON abnormal delay: %d pulses (%d ms)\n",
                       i, t_start - last_note_start, delay);
                delay = 1000;
            }

            packet[0] = delay;
            packet[1] = key2hz(evt->params[0]);
            packet[2] = dur;
            packet[3] = 0x0432;
            fwrite(packet, sizeof(int16_t), 4, fnotes_alt);
            total += 1;
            last_note_start = t_start;
        }
        evt = evt->next;
    }

    printf("wrote %d notes, ", total);
    fclose(fnotes_alt);

    return 1;
}

