/*
 *  ScottFree-64 - a Reworking of ScottFree Revision 1.14b for the Commodore 64
 *  Optimization Library by Mark Seelye
 *  (C) 2020 - Mark Seelye / mseelye@yahoo.com
 *  Version 2.1.0
 *  Original copyright and license for ScottFree Revision 1.14b follows:
 */
/*
 *	ScottFree Revision 1.14b
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */
#ifndef __SF64_H__
#define __SF64_H__

#include <stdbool.h>
#include <stdint.h>

void __fastcall__ print_char(char c);
void __fastcall__ print(char *text);
void __fastcall__ print_number(uint32_t);
void __fastcall__ print_signed_8(int8_t);

uint16_t __fastcall__ bufnum8(uint8_t);
uint16_t __fastcall__ bufnum16(uint16_t);
uint16_t __fastcall__ bufnum32(uint32_t);

uint8_t __fastcall__ a2p (uint8_t);
void __fastcall__ a2p_string (uint8_t *text);

uint8_t __fastcall__ parse_verb_noun(char *buf, uint8_t wl, char *verb, char *noun);
uint8_t __fastcall__ next_word(char *buf, uint8_t wl, char *word);

uint8_t __fastcall__ is_space(uint8_t c);

// DEBUG - Used to output stackpointer value when debugging stack issues
//uint16_t __fastcall__ getsp(void);

#endif
