#ifndef COMMLINKUSB_H_
#define COMMLINKUSB_H_

#include "common_defs.h"

class CommLinkUSB {
public:
    CommLinkUSB();
    ~CommLinkUSB();

    bool Initialize();

    bool SendByte(uint8 data, uint32 timeout);
    bool ReceiveByte(uint8* data, uint32 timeout);

private:
};


#endif
