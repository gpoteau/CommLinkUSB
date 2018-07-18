#include "PAR.h"

#include <avr/io.h>

#define PCWR   0x01
#define PCRD_  0x02
#define STATUS 0x04

typedef enum
{
	READ = 0x00,
	NONE = 0x02,
	WRITE = 0x03
} PARDirection;


static void PAR_SetDirection(PARDirection direction)
{
	PORTB = (PORTB & (PCWR | PCRD_)) | direction;
}

bool PAR_ReadAvailable()
{
    PAR_SetDirection(NONE);
	return (bool)(PINB & STATUS);
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
    PAR_SetDirection(NONE);
}

void PAR_Init()
{
	DDRB = PCWR | PCRD_;
}
