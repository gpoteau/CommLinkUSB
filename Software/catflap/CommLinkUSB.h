#ifndef COMMLINKUSB_H_
#define COMMLINKUSB_H_

#include "common_defs.h"

struct libusb_device_handle;

class CommLinkUSB {
public:
    CommLinkUSB();
    ~CommLinkUSB();

    bool Initialize();

    int SendByte(uint8 data, uint32 timeout);
    int ReceiveByte(uint8* data, uint32 timeout);

private:
    libusb_device_handle* deviceHandle;
};


#endif
