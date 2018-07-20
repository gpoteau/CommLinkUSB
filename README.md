CommLinkUSB
===========

![CommLinkUSB](https://raw.githubusercontent.com/gpoteau/CommLinkUSB/master/Hardware/IMG_20150629_0828400.JPG)
![CommLinkUSB](https://raw.githubusercontent.com/gpoteau/CommLinkUSB/master/Hardware/IMG_20150629_0834075.JPG)

CommLinkUSB is an open source USB adapter, it is effectively a replacement for the obsolete and hard to use ISA CommLink adapter.
It allows you to connect Action Replay, Game Hunter, or Clone card to a computer over USB.

License Information
-------------------

This project is derived from https://github.com/gpoteau/CommLinkUSB. However, the firmware and host software have been rewritten or replaced.

The hardware design is licensed under [![Creative Commons License] (https://i.creativecommons.org/l/by-sa/4.0/88x31.png) Creative Commons Attribution-ShareAlike 4.0 International License](http://creativecommons.org/licenses/by-sa/4.0/).  
The USB Stack used in the firmware is [LUFA](http://http://www.fourwalledcubicle.com/LUFA.php), created by Dean Camera.  
The CommLinkUSB firmware was created by Carsten Elton Sørensen.  
The CATFLAPU software is released under an unknown open source license. It is based on https://github.com/hkzlab/catflap4linux.

Repository Contents
-------------------
* **/Firmware**
    * CommLinkUSB.hex - Firmware for Atmega32u4 boards.
* **/Hardware** - Hardware design files for the CommLinkUSB. These files were designed in [Eagle](http://http://www.cadsoftusa.com/).
* **/Software** - Host software and Windows driver.
