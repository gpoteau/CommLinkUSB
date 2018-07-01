@echo off
makecert -r -pe -ss PrivateCertStore -n CN=CommLinkUSB CommLinkUSB.cer
certutil -f -v -AddStore "Root" CommLinkUSB.cer
certutil -f -v -AddStore "TrustedPublisher" CommLinkUSB.cer
signtool sign /v /s PrivateCertStore /n CommLinkUSB /t http://timestamp.verisign.com/scripts/timstamp.dll ..\\drivers\\libusb_device.cat

