# NMEA2000-SignalK-Gateway
A gateway to send  NMEA2000 PGNs to a SignalK server via WLAN.

SignalK becomes more and mor popular to intgrate and visualise data from different sources.
This repository shows how to build a simple NMEA2000 to SignalK gateway using an ESP32 and only a few more elements.

The software is based on the ESP8266 library for SignalK. But it was necessary to change the software to use it on an ESP32 with up-to-date libraries fro ESP32 (ArduinoJson, ESP32SSDP, WebSockets).

To compile the Gateway the followig additional libraries are required:

- ESP32SSDP (via [ZIP-File](https://github.com/luc-github/ESP32SSDP))
- WebSockets (fom Markus Sattler, via Library Manager)
- ArduinoJson (via Library Manager).


