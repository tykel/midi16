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

#ifndef MIDI_H
#define MIDI_H

/*
 *  C API to standard MIDI files.
 *
 *  The MIDI header can be memory-mapped from a file since it retains the
 *  big-endian storage inside; use the hdr_*_le functions to access its
 *  fields.
 *  The size field of the MIDI track chunck structure should be accessed
 *  similarly.
 *
 *  Due to their use of variable length sizes on file, it is not possible
 *  to memory map the MIDI events directly. Instead, they should be read
 *  using the supplied functions.
 */

#include <stdint.h>

/* File format types */
#define FMT_SINGLE_TRACK        0x0000
#define FMT_MULTI_TRACK_SYNC    0x0001
#define FMT_MULTI_TRACK_ASYNC   0x0002


struct __midi_event_t;

/* MIDI Header structure. */
typedef struct
{
    /* "MThd" */
    char id[4];
    /* Chunk size (BE dword) -- 00 00 00 06 */
    uint8_t size[4];
    /* Format type (BE word) */
    uint8_t type[2];
    /* Number of tracks (BE word) */
    uint8_t tracks[2];
    /* Time division (BE word) */
    uint8_t time_div[2];

} midi_header_t;

/* Big-endian chunk size access */
static inline uint32_t hdr_size_le(midi_header_t *h)
{
    return (uint32_t)(h->size[3]       | h->size[2] << 8 |
                      h->size[1] << 16 | h->size[0] << 24);
}
/* Big-endian format type access */
static inline uint16_t hdr_type_le(midi_header_t *h)
{
    return (uint16_t)(h->type[1] | h->type[0] << 8);
}
/* Big-endian tracks total access */
static inline uint16_t hdr_tracks_le(midi_header_t *h)
{
    return (uint16_t)(h->tracks[1] | h->tracks[0] << 8);
}
/* Big-endian time division access */
static inline uint16_t hdr_tdiv_le(midi_header_t *h)
{
    return (uint32_t)(h->time_div[1] | h->time_div[0] << 8);
}


/* MIDI Track Chunk structure. */
typedef struct
{
    /* "MTrk" */
    char id[4];
    /* Chunk size (BE dword) */
    uint8_t size[4];

    /* Midi events pointer */
    struct __midi_event_t *events;

    /* Number of events */
    int num_events;
    /* Tempo -- set by event */
    uint32_t tempo;
    /* Pulse length */
    uint32_t pulse_len;
    /* Patch (instrument) */
    uint8_t patch;

} midi_track_t;

/* Big-endian chunk size access */
static inline uint32_t chk_size_le(midi_track_t *t)
{
    return (uint32_t)(t->size[3]       | t->size[2] << 8 |
                      t->size[1] << 16 | t->size[0] << 24);
}

/* MIDI command mask*/
#define MIDI_CMD_FLAG 0x80

/* MIDI Channel voic/mode event types
 * The "normal" events in the track event stream.
 * NOTE: unless the event is a SYSEX, the event type may be omitted for
 * subsequent events of the same type.
 */

/* Release the given note */
#define MIDI_CMD_NOTE_OFF   0x80
/* Engage the given note; if velocity is 0, same as note off */
#define MIDI_CMD_NOTE_ON    0x90
/* Polyphonic key pressure; sent by pressing after "bottoming out" */
#define MIDI_CMD_AFTERTOUCH 0xA0
/* Change a controller value; see MIDI documentation */
#define MIDI_CMD_CONT_CTRL  0xB0
/* Change the patch/program for the track */
#define MIDI_CMD_PATCH_CHG  0xC0
/* Channel pressure; similar to aftertouch */
#define MIDI_CMD_CHAN_PRSS  0xD0
/* */
#define MIDI_CMD_PITCH_BEND 0xE0
#define MIDI_CMD_NON_MUS    0xF0

/* MIDI System common event types */

/* Signals the start of a SYStem EXclusive event, with variable length */
#define MIDI_CMD_SYSEX_START    0xF0
/* Marks a MIDI Time Code Quarter Frame, used for external synchronisation */
#define MIDI_CMD_TCQF           0xF1
/* Song Position Pointer value, number of beats (= 6 clocks) since start */
#define MIDI_CMD_SONG_POS       0xF2
/* Specifies the song to be played */
#define MIDI_CMD_SONG_SEL       0xF3
/* Request for analog synthesizers to tune their oscillators */
#define MIDI_CMD_TUNE_REQ       0xF6
/* Signals the end of a SYStem EXclusive event */
#define MIDI_CMD_SYSEX_END      0xF7

/* MIDI System real-time event types
 * UNUSED in most standard MIDI files.
 */

/* Timing clock message, 24 times/quarter note if sync. required */
#define MIDI_CMD_TIMING_CLK     0xF8
/* Start current sequence playing (followed by TIMING_CLK) */
#define MIDI_CMD_START          0xFA
/* Continue current sequence playing (if stopped) */
#define MIDI_CMD_CONTINUE       0xFB
/* Stop current sequence playing */
#define MIDI_CMD_STOP           0xFC
/* Active sensing; a keep-alive message */
#define MIDI_CMD_ACT_SENS       0xFE
/* Reset receivers */
#define MIDI_CMD_SYS_RESET      0xFF

/* MIDI Meta-event types 
 * These events follow a SYS_RESET (= meta-event) command.
 * They are typically found at the start of a MIDI track.
 */
/* Defines the sequence/track number, instead of implicit file ordering */
#define MIDI_META_SEQ_NUM   0x00
/* Specifies a general text comment */
#define MIDI_META_TEXT      0x01
/* Specifies a copyright message */
#define MIDI_META_COPYRIGHT 0x02
/* Specifies the track/sequence name */
#define MIDI_META_SEQ_NAME  0x03
/* Specifies the instrument name, e.g. the MIDI keyboard */
#define MIDI_META_INSTR     0x04
/* Specifies a lyric at the current point */
#define MIDI_META_LYRIC     0x05
/* Specifies a text marker; e.g., a loop start/end */
#define MIDI_META_MARKER    0x06
/* Specifies a "cue point" to display */
#define MIDI_META_CUE_PT    0x07
/* Specifies the program (patch) name; often instrument name */
#define MIDI_META_PRG_NAME  0x08
/* Specifies the MIDI device (port) name */
#define MIDI_META_DEV_NAME  0x09
/* Marks the end of a MIDI track */
#define MIDI_META_END       0x2F
/* Defines the tempo, in microseconds per quarter note */
#define MIDI_META_TEMPO     0x51
/* Defines the time signature; see MIDI documentation */
#define MIDI_META_TIMESIG   0x58
/* Defines the key signature: flats/key/sharps, major/minor */
#define MIDI_META_KEYSIG    0x59
/* Proprietary meta event; ignore this */
#define MIDI_META_PROPR     0x7F

#define MIDI_CMD_MAX_SIZE   0xFF

/* MIDI Event structure */
typedef struct __midi_event_t
{
    /* Delta-time since last event, in time divs */
    uint32_t dt;
    /* Event status */
    uint8_t status;
    /* Meta-event type (only set when status==0xFF) */
    uint8_t meta;
    /* Lookup number of params quickly */
    uint8_t param_len;
    /* Parameters */
    uint8_t params[MIDI_CMD_MAX_SIZE];
    /* Used in case a string of MIDI_CMD_MAX_SIZE needs a '\0' */
    char __pnt;

    /* Variable length value (only SYSEX events) */
    uint32_t varlen;
    /* Channel the command applies to (if applicable) */
    uint8_t channel;
    /* Lookup if params are an ASCII string */
    int8_t is_ascii;
    /* Next event for traversal */
    struct __midi_event_t *next;

} midi_event_t;

/* (Internal) Read variable-length dt used in event */
/*static uint32_t read_varlen(uint8_t **m);*/

/* Read the next MIDI event from memoru */
midi_event_t* midi_event_next(void **m, uint8_t last_status);

/* Read a whole track of events */
midi_track_t midi_read_track(void **m, midi_header_t *h);

/* Traverse and free the linked list of events */
void midi_free_track(midi_track_t *t);

/* Helper functions for octave and note extraction */
inline int get_octave(uint32_t n)
{
    return n / 12;
}

inline int get_note(uint32_t n)
{
    return n % 12;
}

inline int get_bpm(uint32_t uspqn)
{
    return 60000000/uspqn;
}

inline int get_bps(uint32_t uspqn)
{
    return 1000000/uspqn;
}

/* Command code to string */
const char* midi_cmd_str(uint8_t cmd);

/* Meta event code to string */
const char* midi_meta_str(uint8_t meta);

#endif
