"""
             LUFA Library
     Copyright (C) Dean Camera, 2017.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
"""

"""
    LUFA Bulk Vendor device demo host test script. This script will send and
    receive a continuous stream of packets to/from to the device, to show
    bidirectional communications.

    Requires the pyUSB library (http://sourceforge.net/projects/pyusb/).
"""

import sys
from time import sleep
import usb.core
import usb.util

# Bulk Vendor HID device VID and PID
device_vid = 0x1209
device_pid = 0xBAD2
device_in_ep  = 3
device_out_ep = 4

CAETLA_RESET = chr(3)


def get_vendor_device_handle():
	dev_handle = usb.core.find(idVendor=device_vid, idProduct=device_pid)
	return dev_handle


def write(device, packet):
    device.write(usb.util.ENDPOINT_OUT | device_out_ep, packet)
    print("Sent Packet: {0:x} ({0:c})".format(ord(packet[0])))


def read(device):
    while True:
        packet = device.read(usb.util.ENDPOINT_IN | device_in_ep, 1)
        if len(packet) > 0:
            break
    print("Received Packet: {0:x} ({0:c})".format(packet[0]))
    return packet

def swap8(device, packet):
    write(device, packet)
    return chr(read(device)[0])

def send_expect(device, request, expected):
    actual = swap8(device, request)
    if expected != actual:
        raise SystemError("Expected %s, got %s" % (expected, actual))

def issue_command(device, command):
    send_expect(device, 'C', 'd')
    send_expect(device, 'L', 'o')
    swap8(device, command)

def main():
    vendor_device = get_vendor_device_handle()

    if vendor_device is None:
        print("No valid Vendor device found.")
        sys.exit(1)

    vendor_device.set_configuration()

    print("Connected to device 0x%04X/0x%04X - %s [%s]" %
          (vendor_device.idVendor, vendor_device.idProduct,
           usb.util.get_string(vendor_device, vendor_device.iProduct),
           usb.util.get_string(vendor_device, vendor_device.iManufacturer)))

    issue_command(vendor_device, CAETLA_RESET)

    """
    x = 0
    while 1:
    	x = x + 1 % 255
    	write(vendor_device, "TEST PACKET %d" % x)
    	read(vendor_device)
    	sleep(1)
    """

if __name__ == '__main__':
    main()
