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

static const char *asm_string = 
"init:\n"
"    sng 0x11, 0xf1d6\n"
"    ldi r4, __notes\n"
"    ldi r2, note\n"
"    ldi r3, dur\n"
"play_note:\n"
"    ldm r1, r4 ; read note\n"
"    cmpi r1, 0xFFFF\n"
"    jz init\n"
"    mov r2, r1 ; store it in note\n"
"    mov r5, r1\n"
"    addi r4, 2 ; point at dt\n"
"    ldm r1, r4 ; read dt\n"
"    stm r1, r3 ; store it in dur\n"
"    cmpi r5, 0\n"
"    jz play_note_wait\n"
"    call play\n"
"play_note_wait:\n"
"    mov ra, r1\n"
"    call wait\n"
"    addi r4, 2\n"
"    jmp play_note\n\n" 
"wait:\n"
"    divi ra, 16\n"
"wait_loop:\n"
"    cmpi ra, 0\n"
"    jz wait_end\n"
"    vblnk\n"
"    subi ra, 1\n"
"    jmp wait_loop\n"
"wait_end:\n"
"    ret\n"
"end:\n"
"    vblnk\n"
"    jmp end\n"
"note:\n"
"    dw 0\n"
"play:\n"
"    db 0x0d, 0x02\n"
"dur:\n"
"    dw 0\n"
"    ret\n";


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
    FILE *fasm, *fnotes, *fnotes_alt;
    char importstr[256];
    midi_event_t *evt;
    /* Timing information */
    int i, start, last_start, end, last_end, total;
    /* Note and dt packed together for writing */
    uint16_t note_dt[2];
    int16_t packet[4];
    /* Milliseconds per pulse */
    uint32_t mspp;
    /* For each key/note, check if on or off */
    int note_on[NUM_NOTES];
    /* For each key/note, record the start time */
    int note_start[NUM_NOTES] = {0};
    int note_end[NUM_NOTES] = {0};
    
    /* Write the notes to a separate file */
    if((fnotes = fopen(fn_notes, "wb")) == NULL) {
        printf("error: could not open %s for writing\n", fn_notes);
        return -2;
    }
    if((fnotes_alt = fopen(fn_notes_alt, "wb")) == NULL) {
        printf("error: could not open %s for writing\n", fn_notes_alt);
        return -2;
    }
  
    memset(note_on, 0, NUM_NOTES * sizeof(int));
    memset(note_start, 0, NUM_NOTES * sizeof(int));

    mspp = track->pulse_len;
    printf("using %d ms per pulse\n", mspp); 
    evt = track->events;

    last_end = 0;

    for(i = total = start = last_start = end = 0; i < track->num_events; i++) {
        if(evt->status >= MIDI_CMD_NON_MUS) {
            evt = evt->next;
            continue;
        }
        
        if((evt->status & 0xF0) == MIDI_CMD_NOTE_ON && evt->params[1]) {
            start += evt->dt;
            note_start[evt->params[0]] = start;
            note_on[evt->params[0]] = 1;
            /* Write gaps too */
            if(end > 0) {
                uint16_t note_dt[2];
                note_dt[0] = 0;
                note_dt[1] = (start - last_start) * mspp;
                fwrite(note_dt, sizeof(uint16_t), 2, fnotes);
                total++;
            }
        } else if((evt->status & 0xF0) == MIDI_CMD_NOTE_OFF ||
                  ((evt->status & 0xF0) == MIDI_CMD_NOTE_ON && !evt->params[1])) {
            uint16_t note_dt[2];
            
            /* Note is released, time to write it now we know its duration */
            note_dt[0] = (uint16_t) key2hz(evt->params[0]);
               packet[1] = (int16_t) key2hz(evt->params[0]);
            end = note_start[evt->params[0]] + evt->dt;
            if (last_end == 0) {
               last_end = end;
            }
               packet[0] = (end - last_end) * mspp / 16;
            if (packet[0] < 0) packet[0] = 0;
               last_end = end;
               packet[2] = (end - note_start[evt->params[0]]) * mspp / 16;
               packet[3] = 0x0432; 
            note_dt[1] = (end - note_start[evt->params[0]]) * mspp;

            fwrite(note_dt, sizeof(uint16_t), 2, fnotes);
               fwrite(packet, sizeof(int16_t), 4, fnotes_alt);
            total++;
            start += evt->dt;
            note_on[evt->params[0]] = 0;
            last_start = start;
        }
        
        evt = evt->next;
    }

    printf("wrote %d notes, ", total);
    
    fclose(fnotes_alt);
    fclose(fnotes);

    /* Write the Chip16 code to play the notes */
    if((fasm = fopen(fn_asm, "w")) == NULL) {
        printf("error: could not open %s for writing\n", fn_asm);
        return -1;
    }

    fputs("; track autogenerated from midi16\n"
          "; see http://github.com/tykel/midi16\n\n", fasm);
    snprintf(importstr, 255, "importbin %s 0 %lu %s\n\n",
            fn_notes, total * sizeof(uint16_t) * 2, "__notes");
    fputs(importstr, fasm);
    fputs(asm_string, fasm);

    fclose(fasm);

    return 1;
}

