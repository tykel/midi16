#include "midi.h"

static uint32_t read_varlen(uint8_t **m)
{
    uint32_t dt;
    uint8_t **p = m;

    if(!(**p & 0x80)) return UINT32_MAX;
    while(**p & 0x80) {
        dt = (dt << 8) | (**p++ & 0x7f);
    }
    return dt;
}

static midi_event_t midi_event_next(void *m)
{
    midi_event_t e;
    uint8_t i, *p = m;

    e.dt = read_varlen(&p);
    if(e.dt == UINT32_MAX) 
        /* invalid delta time; e is in an error state */
        return e;
    
    e.status = *p++;
    if(e.status & MIDI_CMD_FLAG) {
        switch(e.status & 0xF0) {
        /* 2-parameter commands/events */
        case MIDI_CMD_NOTE_OFF:
        case MIDI_CMD_NOTE_ON:
        case MIDI_CMD_AFTERTOUCH:
        case MIDI_CMD_CONT_CTRL:
        case MIDI_CMD_PATCH_CHG:
            e.params[0] = *p++;
            e.params[1] = *p++;
            e.param_len = 2;
            break;
        /* 2-parameter; pitch bend specifies 7 LSB and MSB for params */
        case MIDI_CMD_PITCH_BEND:
            e.params[0] = *p++ & 0x7F;
            e.params[1] = *p++ >> 1;
            e.param_len = 2;
            break;
        /* 1-parameter commands/events */
        case MIDI_CMD_CHAN_PRSS:
            e.params[0] = *p++;
            e.param_len = 1;
            break;
        /* Special command/event F0 */
        case MIDI_CMD_NON_MUS:
            switch(e.status) {
            case MIDI_CMD_SYSEX_START:
                e.varlen = read_varlen(&p);
                /* Clamp length to 255... hopefully this doesn't break much. */
                e.param_len = e.varlen & 0xFF;
                for(i = 0; i < e.param_len; i++) {
                    e.params[i] = *p++;
                }
                break;
            case MIDI_CMD_TCQF:
            case MIDI_CMD_SONG_POS:
            case MIDI_CMD_SONG_SEL:
            case MIDI_CMD_TUNE_REQ:
            case MIDI_CMD_SYSEX_END:
            case MIDI_CMD_TIMING_CLK:
            case MIDI_CMD_START:
            case MIDI_CMD_CONTINUE:
            case MIDI_CMD_STOP:
            case MIDI_CMD_ACT_SENS:
                break;
            case MIDI_CMD_SYS_RESET:
                switch(*p++) {
                case MIDI_META_SEQ_NUM:
                    e.param_len = *p++;
                    if(e.param_len) {
                        e.params[0] = *p++;
                        e.params[1] = *p++;
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
                    e.varlen = read_varlen(&p);
                    e.param_len = e.varlen & 0xFF;
                    for(i = 0; i < e.param_len; i++)
                        e.params[i] = *p++;
                    break;
                case MIDI_META_END:
                    e.param_len = *p++;
                    break;
                case MIDI_META_TEMPO:
                    e.param_len = *p++;
                    e.params[0] = *p++;
                    e.params[1] = *p++;
                    e.params[2] = *p++;
                    break;
                case MIDI_META_TIMESIG:
                case MIDI_META_KEYSIG:
                case MIDI_META_PROPR:
                    break;
                }
                break;
            }
            break;
        }
    }
     
    return e;
}

