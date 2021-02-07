# NMEA2000-SignalK-Gateway
A gateway to send  NMEA2000 PGNs to a SignalK server via WLAN.

SignalK becomes more and more popular to integrate and visualise data from different sources.
This repository shows how to build a simple NMEA2000 to SignalK gateway using an ESP32 and only a few more elements.

The software is based on the ESP8266 library for SignalK ([EspSigK](https://github.com/mxtommy/EspSigK)). But it was necessary to change the software to use it on an ESP32 with up-to-date libraries fro ESP32 (ArduinoJson, ESP32SSDP, WebSockets).

To use the gateway the following libraries have to be intalled:

- NMEA2000 (via [ZIP-File](https://github.com/ttlappalainen/NMEA2000))
- NMEA2000_esp32 (via [ZIP-File](https://github.com/ttlappalainen/NMEA2000_esp32))
- ESP32SSDP (via [ZIP-File](https://github.com/luc-github/ESP32SSDP))
- WebSockets (fom Markus Sattler, via Library Manager)
- ArduinoJson (via Library Manager).

In the main programmyou have to change the WLAN informatiom to allow the ESP32 to connect to you WLAN netweork.

```
const String hostname  = "NMEA2000-Gateway";    //Hostname for network discovery
const String ssid      = "ssid";     //SSID to connect to
const String ssidPass  = "password";  // Password for wifi
```

In setup() you can also define details regarding your local SignalK gateway:
```
  sigK.setPrintDebugSerial(false);       // Default false, causes debug messages to be printed to Serial (connecting etc) 
  sigK.setPrintDeltaSerial(false);       // Default false, prints deltas to Serial.
  sigK.setServerHost("192.168.0.20");    // Optional. Sets the ip of the SignalKServer to connect to. If not set we try to discover server with mDNS
  sigK.setServerPort(80);                // If manually setting host, this sets the port for the signalK Server (default 80);
  
  //sigK.setServerToken("secret"); // if you have security enabled in node server, it wont accept deltas unles you auth
  // add a user via the admin console, and then run the "signalk-generate-token" script
  // included with signalk to generate the string. (or disable security)
```
If you do have security enabled on your SignalK server you have use sigK.setServerToken("secret") with the correct value generate on the gateway.

The gateway has many predefined conversion from NMEa 2000 to SignalK. Feel free to add more. Information regarding the different "path" information for SignalK conversion can be founf [here](https://signalk.org/specification/1.5.0/doc/vesselsBranch.html).


