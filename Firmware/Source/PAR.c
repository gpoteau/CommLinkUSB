/*
  Copyright 2018  Carsten Elton Sorensen (cso [at] rift [dot] dk)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

#include "PAR.h"

#include <avr/io.h>

/* Port B bit definitions */
#define PCWR   0x01
#define PCRD_  0x02
#define STATUS 0x04

/* Direction enum, bit pattern directly corresponds to port B bits */
typedef enum
{
	READ = 0x00,
	NONE = 0x02,
	WRITE = 0x03
} PARDirection;


static void PAR_SetDirection(PARDirection direction)
{
	PORTB = (PORTB & ~(PCWR | PCRD_)) | direction;
    DDRD = direction == READ ? 0x00 : 0xFF;
}

bool PAR_ReadAvailable()
{
    PAR_SetDirection(NONE);
	return (PINB & STATUS) == 0;
}

uint8_t PAR_ReadByte()
{
    PAR_SetDirection(READ);
	uint8_t data = PIND;
    PAR_SetDirection(NONE);

    return data;
}

void PAR_WriteByte(uint8_t data)
{
    PORTD = data;
    PAR_SetDirection(WRITE);
}

void PAR_Init()
{
    /* PCWR and PCRD_ are outputs, STATUS is input */
	DDRB = PCWR | PCRD_;
}
