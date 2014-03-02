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

#ifndef CHIP16_H 
#define CHIP16_H

/*
 * Chip16 output functionality.
 */

#include "midi.h"

/* Write a standard MIDI track straight to file in Chip16 assembly */
int chip16_write_track(const char *fn_asm, const char *fn_notes,
                       midi_track_t *track, uint32_t mspq);

#endif

