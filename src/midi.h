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
    uint8_t *events;

} midi_track_t;


/* Big-endian chunk size access */
static inline uint32_t chk_size_le(midi_track_t *t)
{
    return (uint32_t)(t->size[3]       | t->size[2] << 8 |
                      t->size[1] << 16 | t->size[0] << 24);
}



/* MIDI command mask*/
#define MIDI_CMD_FLAG 0x80

/* MIDI event command types */
#define MIDI_CMD_NOTE_OFF   0x80
#define MIDI_CMD_NOTE_ON    0x90
#define MIDI_CMD_AFTERTOUCH 0xA0
#define MIDI_CMD_CONT_CTRL  0xB0
#define MIDI_CMD_PATCH_CHG  0xC0
#define MIDI_CMD_CHAN_PRSS  0xD0
#define MIDI_CMD_PITCH_BEND 0xE0
#define MIDI_CMD_NON_MUS    0xF0

/* MIDI SYSEX event types */
#define MIDI_CMD_SYSEX_START    0xF0
#define MIDI_CMD_TCQF           0xF1
#define MIDI_CMD_SONG_POS       0xF2
#define MIDI_CMD_SONG_SEL       0xF3
#define MIDI_CMD_TUNE_REQ       0xF6
#define MIDI_CMD_SYSEX_END      0xF7
#define MIDI_CMD_TIMING_CLK     0xF8
#define MIDI_CMD_START          0xFA
#define MIDI_CMD_CONTINUE       0xFB
#define MIDI_CMD_STOP           0xFC
#define MIDI_CMD_ACT_SENS       0xFE
#define MIDI_CMD_SYS_RESET      0xFF

/* MIDI Meta-event types */
#define MIDI_META_SEQ_NUM   0x00
#define MIDI_META_TEXT      0x01
#define MIDI_META_COPYRIGHT 0x02
#define MIDI_META_SEQ_NAME  0x03
#define MIDI_META_INSTR     0x04
#define MIDI_META_LYRIC     0x05
#define MIDI_META_MARKER    0x06
#define MIDI_META_CUE_PT    0x07
#define MIDI_META_PRG_NAME  0x08
#define MIDI_META_DEV_NAME  0x09
#define MIDI_META_END       0x2F
#define MIDI_META_TEMPO     0x51
#define MIDI_META_TIMESIG   0x58
#define MIDI_META_KEYSIG    0x59
#define MIDI_META_PROPR     0x7F

#define MIDI_CMD_MAX_SIZE       255

/* MIDI Event structure */
typedef struct
{
    /* Delta-time since last event, in time divs */
    uint32_t dt;
    /* Event status */
    uint8_t status;
    /* Meta-event type (only set when status==0xFF) */
    uint8_t meta;
    /* Parameters */
    uint8_t params[MIDI_CMD_MAX_SIZE];
    /* Lookup number of params quickly */
    uint8_t param_len;
    /* Variable length value (only SYSEX events) */
    uint32_t varlen;
    /* Lookup if params are an ASCII string */
    int8_t is_ascii;

} midi_event_t;


/* (Internal) Read variable-length dt used in event */
static uint32_t read_varlen(uint8_t **m);

/* Read the next MIDI event from memoru */
midi_event_t midi_event_next(void **m);


/* Helper functions for octave and note extraction */
inline int get_octave(uint32_t n)
{
    return n / 12;
}

inline int get_note(uint32_t n)
{
    return n % 12;
}

#endif
