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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "midi.h"
#include "chip16.h"

int main(int argc, char **argv)
{
    void *p;
    FILE *fmid;
    int size, i, j, t, channel;
    uint8_t *bufmid;
    midi_header_t *h;
    midi_track_t *tc;
    uint16_t tdiv;

    fmid = NULL, tc = NULL, bufmid = p = h = NULL;

    if(argc <= 1) {
        fprintf(stderr,"error: no MIDI file specified\n");
        exit(1);
    }

    fmid = fopen(argv[1],"rb");
    if(fmid == NULL) {
        fprintf(stderr,"error: MIDI file %s could not be opened\n",argv[1]);
        exit(1);
    }

    channel = 1;
    if(argc > 2) {
        if(!strcmp(argv[2], "--channel") || !strcmp(argv[2], "-c")) {
            if(argc > 3)
                channel = atoi(argv[3]);
            else
                fprintf(stderr,"warning: no parameter passed to '%s', ignoring\n",
                        argv[2]);
        }
        else
            fprintf(stderr,"warning: unknown option '%s'\n", argv[2]);

    } else {
        printf("No channel specified for conversion, defaulting to %d\n",
               channel);
    }

    fseek(fmid,0,SEEK_END);
    size = ftell(fmid);
    fseek(fmid,0,SEEK_SET);
    printf("debug: file size = %d bytes\n",size);

    bufmid = malloc(size);
    fread(bufmid,1,size,fmid);
    fclose(fmid);

    h = (midi_header_t*) bufmid;
    tdiv = hdr_tdiv_le(h);
    printf("debug: id: '%c%c%c%c', size: %u, type: 0x%x, tracks: %u, "
           "timediv: %u %s\n",
           h->id[0], h->id[1], h->id[2], h->id[3],
           hdr_size_le(h), hdr_type_le(h), hdr_tracks_le(h),
           tdiv, tdiv & 0x8000 ? "fps" : "ppq");

    tc = malloc(hdr_tracks_le(h) * sizeof(midi_track_t));
    p = (bufmid + sizeof(midi_header_t));
    
    for(t = 0; t < hdr_tracks_le(h); t++) {
        midi_event_t *ev = NULL;
        tc[t] = midi_read_track(&p, h);
        printf("debug: [track %i] id: '%c%c%c%c', size: %u, tempo: %u (%u bpm)\n",
               t, tc[t].id[0], tc[t].id[1], tc[t].id[2], tc[t].id[3],
               chk_size_le(&tc[t]), tc[t].tempo, 60000000/tc[t].tempo);

#ifdef DEBUG_EVENTS
        ev = tc[t].events;
        for(i = 0; i < tc[t].num_events; i++) {
            printf("       +[event %d] dt: %u, event: %s (0x%02x), ",
                   i, ev->dt, midi_cmd_str(ev->status), ev->status);
            if(ev->status == MIDI_CMD_SYS_RESET)
                printf("meta: %s (0x%02x)\n",
                       midi_meta_str(ev->meta), ev->meta);
            else if(ev->status < 0xF0)
                printf("channel: %d\n", ev->channel);
            else
                printf("\n");
            if(ev->is_ascii) {
                printf("                  params: '%s'\n", (char *) ev->params);
            }
            else {
                printf("                  params: [");
                for(j = 0; j < ev->param_len; j++)
                    printf(" %02x", ev->params[j]);
                printf(" ]\n");
            }

            ev = ev->next;
        }
#else
        printf("                 events: %u\n", tc[t].num_events);
#endif

    }
    
    printf("writing chip16 asm to 'test.s', notes to 'test.bin' ... ");
    chip16_write_track("test.s", "test.bin", &tc[channel]);
    printf("done.\n");
    
    for(t = 0; t < hdr_tracks_le(h); t++)
        midi_free_track(&tc[t]);
    free(tc);
    free(bufmid);
    
    return 0;
}
