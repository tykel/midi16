#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "midi.h"

static uint32_t read_varlen(uint8_t **m)
{
    int i;
    uint32_t dt = 0;
    uint8_t **p = m;

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

midi_track_t midi_read_track(void **m)
{
    int i;
    uint8_t **p;
    midi_track_t t;
    midi_event_t *e, *old_e;

    /* Copy id and chunk size */
    memcpy(&t, *m, sizeof(t.id) + sizeof(t.size));
    *m += sizeof(t.id) + sizeof(t.size);

    for(i = 0; ; i++) {
        e = midi_event_next(m, i > 0 ? old_e->status : 0);

        if(i == 0)
            t.events = e;
        else
            old_e->next = e;

        old_e = e;
        t.num_events++;

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

