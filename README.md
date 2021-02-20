# NMEA2000 to SignalK Gateway

SignalK becomes more and more popular to integrate and visualise data from different sources.

This repository shows how to build a simple NMEA2000 to [SignalK](https://signalk.org/) gateway using an ESP32 and only a few more elements (CAN bus transceiver, DC converter).

There are three different ways shown to implement a NMEA2000 to SignalK gateway:

1. [A WLAN gateway for specific PGNs](https://github.com/AK-Homberger/NMEA2000-SignalK-Gateway/blob/main/README.md#1-wlan-gateway-specific-pgns)
2. [A WLAN gateway for all PGNs (Yacht Devices format via UDP)](https://github.com/AK-Homberger/NMEA2000-SignalK-Gateway/blob/main/README.md#2-wlan-gateway-all-pgns)
3. [An USB-Serial gateway for all PGNs (Actisense format)](https://github.com/AK-Homberger/NMEA2000-SignalK-Gateway/blob/main/README.md#3-usb-serial-connection-to-signalk-server-all-pgns)

For all implementations the same hardware is used. The following schematics shows the required components and connections.

![Schematics](https://github.com/AK-Homberger/NMEA2000-SignalK-Gateway/blob/main/ESP32-Minimum.png)

The PCB from the [WLAN gateway](https://github.com/AK-Homberger/NMEA2000WifiGateway-with-ESP32) can be used for the gateway. Just ignore the not needed parts.

![PCB](https://github.com/AK-Homberger/NMEA2000WifiGateway-with-ESP32/blob/master/KiCAD/ESP32WifiAisTempVolt2/ESP32WifiAisTempVolt2-PCB.png)

The board can be ordered at Aisler.net: https://aisler.net/p/DNXXRLFU

**!!! Be careful with placing the the ESP32 on the PCB. The USB connector has to be on the right side !!!**


# 1. WLAN Gateway (specific PGNs)

With this implementation you do have the full control about how NMEA2000 PGNs are interpreted and what "path" information is used to forward it to SignalK. The connection between the ESP32 and the SignalK server is wireless with WLAN.

The software is based on the ESP8266 library for SignalK ([EspSigK](https://github.com/mxtommy/EspSigK)). But it was necessary to change the software to use it on an ESP32 with up-to-date libraries (ArduinoJson, ESP32SSDP, WebSockets).

And I had to add also a String-Method for "EspSigK::addDeltaValue()" and "EspSigK::sendDelta()". Without the extension, it was not possible to send a position.

To use the gateway, the following libraries have to be installed:

- NMEA2000 (via [ZIP-File](https://github.com/ttlappalainen/NMEA2000))
- NMEA2000_esp32 (via [ZIP-File](https://github.com/ttlappalainen/NMEA2000_esp32))
- ESP32SSDP (via [ZIP-File](https://github.com/luc-github/ESP32SSDP))
- WebSockets (fom Markus Sattler, via Library Manager)
- ArduinoJson (via Library Manager).

In the [main programm](https://github.com/AK-Homberger/NMEA2000-SignalK-Gateway/blob/main/NMEA2000-SignalK-Gateway/NMEA2000-SignalK-Gateway.ino), you have to change the WLAN information to allow the ESP32 to connect to you WLAN network.

```
const String hostname  = "NMEA2000-Gateway";    //Hostname for network discovery
const String ssid      = "ssid";     //SSID to connect to
const String ssidPass  = "password";  // Password for wifi
```

In setup() you can also define details regarding your local SignalK gateway:
```
  sigK.setPrintDebugSerial(true);       // Default false, causes debug messages to be printed to Serial (connecting etc)
  sigK.setPrintDeltaSerial(false);       // Default false, prints deltas to Serial.
  //sigK.setServerHost("192.168.0.20");    // Optional. Sets the ip of the SignalKServer to connect to. If not set we try to discover server with mDNS
  //sigK.setServerPort(80);                // If manually setting host, this sets the port for the signalK Server (default 80);

  //sigK.setServerToken("secret"); // if you have security enabled in node server, it wont accept deltas unles you auth
  // add a user via the admin console, and then run the "signalk-generate-token" script
  // included with signalk to generate the string. (or disable security)

  sigK.begin();                          // Start everything. Connect to wifi, setup services, etc...

```
Server discovery via MDNS is default. If the ESP32 can't find the SignalK server, you can set the hostname and port of the server.

If you do have security enabled on your SignalK server, you have to use "sigK.setServerToken("secret")" with the correct value generated on the gateway.

The gateway has many predefined conversions from NMEA2000 to SignalK. Feel free to add more. 

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
It is easy to add other PGNs as required.

You can also connect sensors (I2C, 1-Wire) to the ESP32. Then it is also possible to send sensor data to the SignalK server. See Examples of the [EspSigK](https://github.com/mxtommy/EspSigK) library for details on how to send.

Alternatively you can follow the tutorial part of the NMEA2000 tutorial regarding [SignalK](https://github.com/AK-Homberger/NMEA2000-Workshop/blob/main/Docs/BME280-3-SignalK.md) (currently only available in German language, but you can use Google translator).

This picture shows the data in te SignalK "Instrumentpanel WebApp" window.
 
![Intruments](https://github.com/AK-Homberger/NMEA2000-SignalK-Gateway/blob/main/Signalk-Instrumentpanel.png)

# 2. WLAN Gateway (all PGNs)
The second implementation shows how to build a gateway that sends all PGNs from NMEA2000 via WLAN to a SignalK server. The format is Yacht Devices format via UDP. TCP is also possible, but I recognised re-connect problems (restart of SignalK server necessary). But UDP is working fine.

For this solution, only these libraries have to be installed:
- NMEA2000 (via [ZIP-File](https://github.com/ttlappalainen/NMEA2000))
- NMEA2000_esp32 (via [ZIP-File](https://github.com/ttlappalainen/NMEA2000_esp32))

in the [Programm](https://github.com/AK-Homberger/NMEA2000-SignalK-Gateway/blob/main/NMEA2000-SignalK-YD-Gateway/NMEA2000-SignalK-YD-Gateway.ino) you have to set the details for your WLAN and also for the SignalK server.

```
const char* hostname  = "NMEA2000-Gateway";    // Hostname for network discovery
const char* ssid      = "ssid";                // SSID to connect to
const char* ssidPass  = "password";            // Password for wifi


// UPD broadcast to SignalK server
const char * udpAddress = "192.168.0.20"; // UDP broadcast address. Should be the SignalK server
const int udpPort = 4444;                 // YD UDP port
```

With "192.168.0.20" it will send UDP packets only to the gateway IP. Broadcast to all is possible with "192.168.0.255". But you have to adjust to your network address anyway in both cases.

With a broadcast you can also use many apps on tablets directly with NMEA2000 (e.g. [NMEAremote](https://www.zapfware.de/nmearemote/) from Zapfware). That is working even without a SignalK server!

With this solution, you will get directly all NMEA2000 PGNs in the SignalK server (or the tablet). 

For SignalK you have to define a new Data Connection in the SignalK server:

![YachtDevices](https://github.com/AK-Homberger/NMEA2000-SignalK-Gateway/blob/main/SignalK-YachtDevicesUDP.png)

That's all.

Many thanks to the Canboatjs team for the [CanId](https://github.com/canboat/canboatjs/blob/3f530b4d8f9237aec1757dce70e38262d631d8ec/lib/canId.js#L35) calculation. That was very helpful for my [implementation](https://github.com/AK-Homberger/NMEA2000-SignalK-Gateway/blob/3878ba35182d8258470d97c02fec8038486888a7/NMEA2000-SignalK-YD-Gateway/NMEA2000-SignalK-YD-Gateway.ino#L164).


# 3. USB-Serial connection to SignalK server (all PGNs)

If you prefer a cable connection between NMEA2000 bus and the SignalK server, you can use the same hardware together with the sketch [ActisenseListenerSender-ESP32.ino](https://github.com/AK-Homberger/NMEA2000-SignalK-Gateway/blob/main/ActisenseListenerSender-ESP32/ActisenseListenerSender-ESP32.ino).

Due to the fact that power is supplied through USB, you do not need the additional DC converter.

For this implementation, only these libraries have to be installed:
- NMEA2000 (via [ZIP-File](https://github.com/ttlappalainen/NMEA2000))
- NMEA2000_esp32 (via [ZIP-File](https://github.com/ttlappalainen/NMEA2000_esp32))
 
With this solution, you will get directly all NMEA2000 PGNs in the SignalK server. You have to define a new data connection within the server.

![source](https://github.com/AK-Homberger/NMEA2000-SignalK-Gateway/blob/main/SignalK-Actisense.png)

As COM port you have to select the COM port of the ESP32.

That's all.

##Parts
- U1 ESP32 [Link](https://www.amazon.de/AZDelivery-NodeMCU-Development-Nachfolgermodell-ESP8266/dp/B071P98VTG/ref=sxts_sxwds-bia-wc-drs3_0?__mk_de_DE=%C3%85M%C3%85%C5%BD%C3%95%C3%91&cv_ct_cx=ESP32&dchild=1&keywords=ESP32) 
- J5 SN65HVD230 [Link](https://eckstein-shop.de/Waveshare-SN65HVD230-CAN-Board-33V-ESD-protection)
- J2 D24V10F5 [Link](https://eckstein-shop.de/Pololu-5V-1A-Step-Down-Spannungsregler-D24V10F5)
- D1 Diode 1N4001 [Link](https://www.reichelt.com/de/en/rectifier-diode-do41-50-v-1-a-1n-4001-p1723.html?&nbc=1)
- J1 Connector 2-pin [Link](https://www.reichelt.de/de/en/2-pin-terminal-strip-spacing-5-08-akl-101-02-p36605.html?&nbc=1)
