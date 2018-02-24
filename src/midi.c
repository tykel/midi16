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
#include <string.h>

#include "midi.h"

static uint32_t read_varlen(uint8_t **m)
{
    int i;
    uint32_t dt;
    uint8_t **p;

    dt = 0, p = m;

    if(!(**p & 0x80))
        return *(*p)++;
    for(i = 0; i < sizeof(uint32_t); i++) {
        dt = (dt << 7) | (*(*p)++ & 0x7f);
        if(!(**p & 0x80)) 
            break;
    }
    dt = (dt << 8) | (*(*p)++ & 0x7f);
    return dt;
}

midi_event_t* midi_event_next(void **m, uint8_t last_status)
{
    midi_event_t* e;
    uint8_t i, **p = (uint8_t **) m;

    e = calloc(1, sizeof(midi_event_t));
    e->dt = read_varlen(p);
    
    e->status = *(*p)++;
    if(!(e->status & MIDI_CMD_FLAG)) {
        e->status = last_status;
        (*p)--;
    }

    switch(e->status & 0xF0) {
    /* 2-parameter commands/events */
    case MIDI_CMD_NOTE_OFF:
    case MIDI_CMD_NOTE_ON:
    case MIDI_CMD_CONT_CTRL:
        e->params[0] = *(*p)++;
        e->params[1] = *(*p)++;
        e->param_len = 2;
        e->channel = e->status & 0x0F;
        break;
    /* 2-parameter; pitch bend specifies 7 LSB and MSB for params */
    case MIDI_CMD_PITCH_BEND:
        e->params[0] = *(*p)++ & 0x7F;
        e->params[1] = *(*p)++ >> 1;
        e->param_len = 2;
        e->channel = e->status & 0x0F;
        break;
    /* 1-parameter commands/events */
    case MIDI_CMD_AFTERTOUCH:
    case MIDI_CMD_PATCH_CHG:
    case MIDI_CMD_CHAN_PRSS:
        e->params[0] = *(*p)++;
        e->param_len = 1;
        e->channel = e->status & 0x0F;
        break;
    /* Special command/event F0 */
    case MIDI_CMD_NON_MUS:
        switch(e->status) {
        case MIDI_CMD_SYSEX_START:
            e->varlen = read_varlen(p);
            /* Clamp length to 255... hope this doesn't break much. */
            e->param_len = e->varlen & 0xFF;
            for(i = 0; i < e->param_len; i++) {
                e->params[i] = *(*p)++;
            }
            break;
        case MIDI_CMD_TCQF:
        case MIDI_CMD_SONG_SEL:
            e->param_len = 1;
            e->params[0] = *(*p)++;
            break;
        case MIDI_CMD_SONG_POS:
            e->param_len = 2;
            e->params[0] = *(*p)++;
            e->params[1] = *(*p)++;
            break;
        case MIDI_CMD_TUNE_REQ:
        case MIDI_CMD_SYSEX_END:
        case MIDI_CMD_TIMING_CLK:
        case MIDI_CMD_START:
        case MIDI_CMD_CONTINUE:
        case MIDI_CMD_STOP:
        case MIDI_CMD_ACT_SENS:
            break;
        case MIDI_CMD_SYS_RESET:
            switch(e->meta = *(*p)++) {
            case MIDI_META_SEQ_NUM:
                e->param_len = *(*p)++; /* 0 or 2 */
                if(e->param_len) {
                    e->params[0] = *(*p)++;
                    e->params[1] = *(*p)++;
                }
                break;
            case MIDI_META_TEXT:
            case MIDI_META_COPYRIGHT:
            case MIDI_META_SEQ_NAME:
            case MIDI_META_INSTR:
            case MIDI_META_LYRIC:
            case MIDI_META_MARKER:
            case MIDI_META_CUE_PT:
            case MIDI_META_PRG_NAME:
            case MIDI_META_DEV_NAME:
                e->varlen = read_varlen(p);
                e->param_len = e->varlen & 0xFF;
                for(i = 0; i < e->param_len; i++)
                    e->params[i] = *(*p)++;
                e->is_ascii = 1;
                /*printf("text: '%s'\n", (char *) e->params);*/
                break;
            case MIDI_META_END:
                e->param_len = *(*p)++; /* 0 */
                break;
            case MIDI_META_TEMPO:
                e->param_len = *(*p)++; /* 3 */
                e->params[0] = *(*p)++;
                e->params[1] = *(*p)++;
                e->params[2] = *(*p)++;
                break;
            case MIDI_META_TIMESIG:
                e->param_len = *(*p)++; /* 4 */
                e->params[0] = *(*p)++;
                e->params[1] = *(*p)++;
                e->params[2] = *(*p)++;
                e->params[3] = *(*p)++;
                break;
            case MIDI_META_KEYSIG:
                e->param_len = *(*p)++; /* 2 */
                e->params[0] = *(*p)++;
                e->params[1] = *(*p)++;
                break;
            case MIDI_META_PROPR:
                break;
            }
            break;
        }
        break;
    }     
    return e;
}

midi_track_t midi_read_track(void **m, midi_header_t *h)
{
    int i;
    uint8_t **p;
    midi_track_t t;
    midi_event_t *e, *old_e;
    uint32_t ppqn = h->time_div[0] << 8 | h->time_div[1];

    /* Copy id and chunk size */
    memcpy(&t, *m, sizeof(t.id) + sizeof(t.size));
    t.num_events = 0;
    /* Default tempo of 120 bpm? */
    t.tempo = 500000;
    t.pulse_len = 60000 / ((60000000/t.tempo) * ppqn);
    t.patch = 0;
    *(uint8_t *) m += sizeof(t.id) + sizeof(t.size);

    for(i = 0; ; i++) {
        e = midi_event_next(m, i > 0 ? old_e->status : 0);

        if(i == 0)
            t.events = e;
        else
            old_e->next = e;

        old_e = e;
        t.num_events++;

        if(e->meta == MIDI_META_TEMPO) {
            t.tempo = e->params[0] << 16 | e->params[1] << 8 | e->params[2];
            t.pulse_len = 60000 / ((60000000/t.tempo) * ppqn);
        }
        if((e->status & 0xF0) == MIDI_CMD_PATCH_CHG) {
           t.patch = e->params[0] & 0x7f;
        }
        if(e->meta == MIDI_META_END)
            break;
    }

    return t;
}

void midi_free_track(midi_track_t *t)
{
    int i;
    midi_event_t *e, *temp;

    e = t->events;
    while(e != NULL) {
        temp = e;
        e = e->next;
        free(temp);
    }
}

const char* midi_cmd_str(uint8_t cmd)
{
    const char *name;

    switch(cmd & 0xF0) {
    case MIDI_CMD_NOTE_OFF:
        name = "CMD_NOTE_OFF";
        break;
    case MIDI_CMD_NOTE_ON:
        name = "CMD_NOTE_ON";
        break;
    case MIDI_CMD_AFTERTOUCH:
        name = "CMD_AFTERTOUCH";
        break;
    case MIDI_CMD_CONT_CTRL:
        name = "CMD_CONT_CTRL";
        break;
    case MIDI_CMD_PATCH_CHG:
        name = "CMD_PATCH_CHG";
        break;
    case MIDI_CMD_CHAN_PRSS:
        name = "CMD_CHAN_PRSS";
        break;
    case MIDI_CMD_PITCH_BEND:
        name = "CMD_PITCH_BEND";
        break;
    case MIDI_CMD_SYSEX_START:
        name = "CMD_SYSEX_START";
        break;
    case MIDI_CMD_TCQF:
        name = "CMD_TCQF";
        break;
    case MIDI_CMD_SONG_POS:
        name = "CMD_SONG_POS";
        break;
    case MIDI_CMD_SONG_SEL:
        name = "CMD_SONG_SEL";
        break;
    case MIDI_CMD_TUNE_REQ:
        name = "CMD_TUNE_REQ";
        break;
    case MIDI_CMD_SYSEX_END:
        name = "CMD_SYSEX_END";
        break;
    case MIDI_CMD_TIMING_CLK:
        name = "CMD_TIMING_CLK";
        break;
    case MIDI_CMD_START:
        name = "CMD_START";
        break;
    case MIDI_CMD_CONTINUE:
        name = "CMD_CONTINUE";
        break;
    case MIDI_CMD_STOP:
        name = "CMD_STOP";
        break;
    case MIDI_CMD_ACT_SENS:
        name = "CMD_ACT_SENS";
        break;
    case MIDI_CMD_SYS_RESET:
        name = "CMD_SYS_RESET";
        break;
    default:
        name = "(unknown)";
        break;
    }
    
    return name;
}

const char* midi_meta_str(uint8_t meta)
{
    const char *name;

    switch(meta) {
    case MIDI_META_SEQ_NUM:
        name = "META_SEQ_NUM";
        break;
    case MIDI_META_TEXT:
        name = "META_TEXT";
        break;
    case MIDI_META_COPYRIGHT:
        name = "META_COPYRIGHT";
        break;
    case MIDI_META_SEQ_NAME:
        name = "META_SEQ_NAME";
        break;
    case MIDI_META_INSTR:
        name = "META_INSTR";
        break;
    case MIDI_META_LYRIC:
        name = "META_LYRIC";
        break;
    case MIDI_META_MARKER:
        name = "META_MARKER";
        break;
    case MIDI_META_CUE_PT:
        name = "META_CUE_PT";
        break;
    case MIDI_META_PRG_NAME:
        name = "META_PRG_NAME";
        break;
    case MIDI_META_DEV_NAME:
        name = "META_DEV_NAME";
        break;
    case MIDI_META_END:
        name = "META_END";
        break;
    case MIDI_META_TEMPO:
        name = "META_TEMPO";
        break;
    case MIDI_META_TIMESIG:
        name = "META_TIMESIG";
        break;
    case MIDI_META_KEYSIG:
        name = "META_KEYSIG";
        break;
    case MIDI_META_PROPR:
        name = "META_PROPR";
        break;
    default:
        name = "(unknown)";
        break;
    }

    return name;
}

const char *str_patch[128] = {
   "Acoustic Grand Piano",
   "Bright Acoustic Piano",
   "Electric Grand Piano",
   "Honky-tonk Piano",
   "Electric Piano 1",
   "Electric Piano 2",
   "Harpsichord",
   "Clavi",
   "Celesta",
   "Glockenspiel",
   "Music Box",
   "Vibraphone",
   "Marimba",
   "Xylophone",
   "Tubular Bells",
   "Dulcimer",
   "Drawbar Organ",
   "Percussive Organ",
   "Rock Organ",
   "Church Organ",
   "Reed Organ",
   "Accordion",
   "Harmonica",
   "Tango Accordion",
   "Acoustic Guitar (nylon)",
   "Acoustic Guitar (steel)",
   "Electric Guitar (jazz)",
   "Electric Guitar (clean)",
   "Electric Guitar (muted)",
   "Overdriven Guitar",
   "Distortion Guitar",
   "Guitar harmonics",
   "Acoustic Bass",
   "Electric Bass (finger)",
   "Electric Bass (pick)",
   "Fretless Bass",
   "Slap Bass 1",
   "Slap Bass 2",
   "Synth Bass 1",
   "Synth Bass 2",
   "Violin",
   "Viola",
   "Cello",
   "Contrabass",
   "Tremolo Strings",
   "Pizzicato Strings",
   "Orchestral Harp",
   "Timpani",
   "String Ensemble 1",
   "String Ensemble 2",
   "SynthStrings 1",
   "SynthStrings 2",
   "Choir Aahs",
   "Voice Oohs",
   "Synth Voice",
   "Orchestra Hit",
   "Trumpet",
   "Trombone",
   "Tuba",
   "Muted Trumpet",
   "French Horn",
   "Brass Section",
   "SynthBrass 1",
   "SynthBrass 2",
   "Soprano Sax",
   "Alto Sax",
   "Tenor Sax",
   "Baritone Sax",
   "Oboe",
   "English Horn",
   "Bassoon",
   "Clarinet",
   "Piccolo",
   "Flute",
   "Recorder",
   "Pan Flute",
   "Blown Bottle",
   "Shakuhachi",
   "Whistle",
   "Ocarina",
   "Lead 1 (square)",
   "Lead 2 (sawtooth)",
   "Lead 3 (calliope)",
   "Lead 4 (chiff)",
   "Lead 5 (charang)",
   "Lead 6 (voice)",
   "Lead 7 (fifths)",
   "Lead 8 (bass + lead)",
   "Pad 1 (new age)",
   "Pad 2 (warm)",
   "Pad 3 (polysynth)",
   "Pad 4 (choir)",
   "Pad 5 (bowed)",
   "Pad 6 (metallic)",
   "Pad 7 (halo)",
   "Pad 8 (sweep)",
   "FX 1 (rain)",
   "FX 2 (soundtrack)",
   "FX 3 (crystal)",
   "FX 4 (atmosphere)",
   "FX 5 (brightness)",
   "FX 6 (goblins)",
   "FX 7 (echoes)",
   "FX 8 (sci-fi)",
   "Sitar",
   "Banjo",
   "Shamisen",
   "Koto",
   "Kalimba",
   "Bag pipe",
   "Fiddle",
   "Shanai",
   "Tinkle Bell",
   "Agogo",
   "Steel Drums",
   "Woodblock",
   "Taiko Drum",
   "Melodic Tom",
   "Synth Drum",
   "Reverse Cymbal",
   "Guitar Fret Noise",
   "Breath Noise",
   "Seashore",
   "Bird Tweet",
   "Telephone Ring",
   "Helicopter",
   "Applause",
   "Gunshot",
};
