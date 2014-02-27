#include "midi.h"

static uint32_t __midi_event_dt(void **m)
{
    uint32_t dt;
    uint8_t **p = (uint8_t **) m;

    if(!(**p & 0x80)) return UINT32_MAX;
    while(**p & 0x80) {
        dt = (dt << 8) | (**p++ & 0x7f);
    }
    return dt;
}

static midi_event_t midi_event_next(void *m)
{
    midi_event_t e;
    uint8_t *p = m;

    e.dt = __midi_event_dt((void **) &p);
    if(e.dt == UINT32_MAX) 
        /* invalid delta time; e is in an error state */
        return e;
    
    e.status = *p++;
    if(e.status & MIDI_CMD_FLAG) {
        switch(e.status & 0xF0) {
        case MIDI_CMD_NOTE_OFF:
            break;
        case MIDI_CMD_NOTE_ON:
            break;
        case MIDI_CMD_AFTERTOUCH:
            break;
        case MIDI_CMD_CONT_CTRL:
            break;
        case MIDI_CMD_PATCH_CHG:
            break;
        case MIDI_CMD_CHAN_PRSS:
            break;
        case MIDI_CMD_PITCH_BEND:
            break;
        case MIDI_CMD_NON_MUS:
            switch(e.status) {
            case MIDI_CMD_SYSEX_START:
                break;
            case MIDI_CMD_TCQF:
                break;
            case MIDI_CMD_SONG_POS:
                break;
            case MIDI_CMD_SONG_SEL:
                break;
            case MIDI_CMD_TUNE_REQ:
                break;
            case MIDI_CMD_SYSEX_END:
                break;
            case MIDI_CMD_TIMING_CLK:
                break;
            case MIDI_CMD_START:
                break;
            case MIDI_CMD_CONTINUE:
                break;
            case MIDI_CMD_STOP:
                break;
            case MIDI_CMD_ACT_SENS:
                break;
            case MIDI_CMD_SYS_RESET:
                break;
            }
            break;
        }
    }
     
    return e;
}

inline int get_octave(uint32_t n)
{
    return n / 12;
}

inline int get_note(uint32_t n)
{
    return n % 12;
}
