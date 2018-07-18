#ifndef _PAR_H_
#define _PAR_H_

#include <stdint.h>
#include <stdbool.h>

extern void PAR_Init(void);
extern bool PAR_ReadAvailable(void);
extern uint8_t PAR_ReadByte(void);
extern void PAR_WriteByte(uint8_t data);


#endif
