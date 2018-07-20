#include <usb.h>
#include "CommLinkUSB.h"

CommLinkUSB::CommLinkUSB() {
}

CommLinkUSB::~CommLinkUSB() {
}

bool CommLinkUSB::Initialize() {
    return false;
}

bool CommLinkUSB::SendByte(uint8 data, uint32 timeout) {
    return false;
}

bool CommLinkUSB::ReceiveByte(uint8* data, uint32 timeout) {
    return false;
}
