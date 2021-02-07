# NMEA2000 SignalK Gateway
This is a gateway to send  NMEA2000 PGNs to a SignalK server via WLAN.

SignalK becomes more and more popular to integrate and visualise data from different sources.
This repository shows how to build a simple NMEA2000 to [SignalK](https://signalk.org/) gateway using an ESP32 and only a few more elements (CAN bus transceiver, DC converter).

I know, there are other ways to integrate NMEA2000 in SignalK. But with this implementation you do have the full contol about how NMEA2000 PGNs are interpreted and what "path" information is used to forward it to SignalK.

The software is based on the ESP8266 library for SignalK ([EspSigK](https://github.com/mxtommy/EspSigK)). But it was necessary to change the software to use it on an ESP32 with up-to-date libraries for ESP32 (ArduinoJson, ESP32SSDP, WebSockets).

And I had to add also a String-Method for "EspSigK::addDeltaValue()" and "EspSigK::addDeltaValue()". Without the extension it was not possible to send a position.

To use the gateway, the following libraries have to be intalled:

- NMEA2000 (via [ZIP-File](https://github.com/ttlappalainen/NMEA2000))
- NMEA2000_esp32 (via [ZIP-File](https://github.com/ttlappalainen/NMEA2000_esp32))
- ESP32SSDP (via [ZIP-File](https://github.com/luc-github/ESP32SSDP))
- WebSockets (fom Markus Sattler, via Library Manager)
- ArduinoJson (via Library Manager).

In the main programm, you have to change the WLAN informatiom to allow the ESP32 to connect to you WLAN network.

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
But server discovery via MDNS is also supported.

If you do have security enabled on your SignalK server you have use sigK.setServerToken("secret") with the correct value generate on the gateway.

The gateway has many predefined conversion from NMEA 2000 to SignalK. Feel free to add more. 

Information regarding the different "path" information for SignalK conversion can be found [here](https://signalk.org/specification/1.5.0/doc/vesselsBranch.html).

The following PGNs are currently supported:
```
// Set the information for other bus devices, which messages we support
const unsigned long ReceiveMessages[] PROGMEM = {/*126992L,*/ // System time
      127250L, // Heading
      128259L, // Boat speed
      128267L, // Depth
      129025L, // Position
      129026L, // COG and SOG
      129029L, // GNSS
      130306L, // Wind
      128275L, // Log
      130310L, // Water temperature
      127245L, // Rudder
      0
    };
    
 ```
 
This picture shows the data in te SignalK "Instrumentpanel WebApp" window.
 
![Intruments](https://github.com/AK-Homberger/NMEA2000-SignalK-Gateway/blob/main/Signalk-Instrumentpanel.png)
 
The following schematics shows the required components and connections.

![Schematics](https://github.com/AK-Homberger/NMEA2000-SignalK-Gateway/blob/main/ESP32-Minimum.png)
 
 
 

