#include <libusb-1.0/libusb.h>
#include "CommLinkUSB.h"
#include "CCaetlaDefs.h"

#define VENDOR_ID 0x1209
#define PRODUCT_ID 0xBAD2

#define ENDPOINT_IN  (0x03 | LIBUSB_ENDPOINT_IN)
#define ENDPOINT_OUT (0x04 | LIBUSB_ENDPOINT_OUT)

CommLinkUSB::CommLinkUSB() 
:   deviceHandle(NULL) {
}

CommLinkUSB::~CommLinkUSB() {
    if (deviceHandle != NULL)
        libusb_close(deviceHandle);
}

bool CommLinkUSB::Initialize() {
    deviceHandle = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);
    if (deviceHandle != NULL) {
        bool success = libusb_set_configuration(deviceHandle, 1) == 0;
        return success;
    }
    return false;
}


static int USBErrorToCaetla(int result, int actualLength) {
    if (result == 0 && actualLength == 1)
        return CAETLA_ERROR_OK;

    if (result == LIBUSB_ERROR_TIMEOUT)
        return CAETLA_ERROR_TIMEOUT;

    if (result == LIBUSB_ERROR_NO_DEVICE)
        return CAETLA_ERROR_DEVICE_NOT_PRESENT;

    return CAETLA_ERROR_UNKNOWN;
}


int CommLinkUSB::SendByte(uint8 data, uint32 timeout) {
    int actualLength;
    int result = libusb_bulk_transfer(deviceHandle, ENDPOINT_OUT, &data, 1, &actualLength, timeout);
    return USBErrorToCaetla(result, actualLength);
}

int CommLinkUSB::ReceiveByte(uint8* data, uint32 timeout) {
    int actualLength;
    int result = libusb_bulk_transfer(deviceHandle, ENDPOINT_OUT, data, 1, &actualLength, timeout);
    return USBErrorToCaetla(result, actualLength);
}
