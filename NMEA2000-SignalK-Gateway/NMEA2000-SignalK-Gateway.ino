/*
  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

// Read PGNs from NMEA2000-Bus and send as SignalK to server
// Version 0.1, 06.02.2021, AK-Homberger

#define ESP32_CAN_TX_PIN GPIO_NUM_5  // Set CAN TX port to 5 
#define ESP32_CAN_RX_PIN GPIO_NUM_4  // Set CAN RX port to 4

#include <Arduino.h>
#include <Preferences.h>
#include <NMEA2000_CAN.h>   // This will automatically choose right CAN library and create suitable NMEA2000 object
#include <N2kMessages.h>

#include "EspSigK.h"        // For SignalK handling

int NodeAddress;            // To store last Node Address
Preferences preferences;    // Nonvolatile storage on ESP32 - To store LastDeviceAddress

const String hostname  = "NMEA2000-Gateway";    //Hostname for network discovery
const String ssid      = "ssid";     //SSID to connect to
const String ssidPass  = "password";  // Password for wifi

EspSigK sigK(hostname, ssid, ssidPass); // create the object

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


//*****************************************************************************
void setup() {
  uint8_t chipid[6];
  uint32_t id = 0;
  int i = 0;

  // Init USB serial port
  Serial.begin(115200);
  delay(10);

  sigK.setPrintDebugSerial(true);       // Default false, causes debug messages to be printed to Serial (connecting etc)
  sigK.setPrintDeltaSerial(false);       // Default false, prints deltas to Serial.
  //sigK.setServerHost("192.168.0.20");    // Optional. Sets the ip of the SignalKServer to connect to. If not set we try to discover server with mDNS
  //sigK.setServerPort(80);                // If manually setting host, this sets the port for the signalK Server (default 80);

  //sigK.setServerToken("secret"); // if you have security enabled in node server, it wont accept deltas unles you auth
  // add a user via the admin console, and then run the "signalk-generate-token" script
  // included with signalk to generate the string. (or disable security)

  sigK.begin();                          // Start everything. Connect to wifi, setup services, etc...

  // Reserve enough buffer for sending all messages.
  NMEA2000.SetN2kCANMsgBufSize(8);
  NMEA2000.SetN2kCANReceiveFrameBufSize(150);
  NMEA2000.SetN2kCANSendFrameBufSize(150);

  // Generate unique number from chip id
  esp_efuse_read_mac(chipid);
  for (i = 0; i < 6; i++) id += (chipid[i] << (7 * i));

  // Set product information
  NMEA2000.SetProductInformation("1", // Manufacturer's Model serial code
                                 100, // Manufacturer's product code
                                 "NMEA Reader",  // Manufacturer's Model ID
                                 "1.0.2.25 (2019-07-07)",  // Manufacturer's Software version code
                                 "1.0.2.0 (2019-07-07)" // Manufacturer's Model version
                                );
  // Set device information
  NMEA2000.SetDeviceInformation(id,  // Unique number. Use e.g. Serial number.
                                131, // Device function=NMEA 2000 to Analog Gateway. See codes on http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                25,  // Device class=Inter/Intranetwork Device. See codes on  http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                2046 // Just choosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
                               );

  preferences.begin("nvs", false);                          // Open nonvolatile storage (nvs)
  NodeAddress = preferences.getInt("LastNodeAddress", 34);  // Read stored last NodeAddress, default 34
  preferences.end();
  Serial.printf("NodeAddress=%d\n", NodeAddress);

  // If you also want to see all traffic on the bus use N2km_ListenAndNode instead of N2km_NodeOnly below
  NMEA2000.SetMode(tNMEA2000::N2km_ListenOnly, NodeAddress);
  NMEA2000.ExtendReceiveMessages(ReceiveMessages);
  NMEA2000.SetMsgHandler(MyHandleNMEA2000Msg);

  NMEA2000.Open();
  delay(200);
}


//*****************************************************************************
void MyHandleNMEA2000Msg(const tN2kMsg &N2kMsg) {

  switch (N2kMsg.PGN) {
    case 127250L: HandleHeading(N2kMsg);
    case 128259L: HandleBoatSpeed(N2kMsg);
    case 128267L: HandleDepth(N2kMsg);
    case 129025L: HandlePosition(N2kMsg);
    case 129026L: HandleCOG_SOG(N2kMsg);
    case 129029L: HandleGNSS(N2kMsg);
    case 130306L: HandleWind(N2kMsg);
    case 128275L: HandleLog(N2kMsg);
    case 130310L: HandleWaterTemp(N2kMsg);
    case 127245L: HandleRudder(N2kMsg);
  }
}


//*****************************************************************************
void HandleHeading(const tN2kMsg &N2kMsg) {
  unsigned char SID;
  tN2kHeadingReference ref;
  double Deviation = 0;
  double Variation;
  double Heading;

  if ( ParseN2kHeading(N2kMsg, SID, Heading, Deviation, Variation, ref) ) {
    sigK.sendDelta("navigation.headingTrue", Heading + Variation + Deviation);    
  }
}


//*****************************************************************************
void HandleBoatSpeed(const tN2kMsg &N2kMsg) {
  unsigned char SID;
  double WaterReferenced;
  double GroundReferenced;
  tN2kSpeedWaterReferenceType SWRT;

  if ( ParseN2kBoatSpeed(N2kMsg, SID, WaterReferenced, GroundReferenced, SWRT) ) {
    sigK.sendDelta("navigation.speedThroughWater", WaterReferenced);
    sigK.sendDelta("navigation.speedOverGround", GroundReferenced);    
  }
}


//*****************************************************************************
void HandleDepth(const tN2kMsg &N2kMsg) {
  unsigned char SID;
  double DepthBelowTransducer;
  double Offset;
  double Range;
  double WaterDepth;

  if ( ParseN2kWaterDepth(N2kMsg, SID, DepthBelowTransducer, Offset, Range) ) {
    WaterDepth = DepthBelowTransducer + Offset;
    sigK.sendDelta("environment.depth.belowTransducer", DepthBelowTransducer);
    sigK.sendDelta("environment.depth.belowSurface", WaterDepth);    
  }
}


//*****************************************************************************
void HandlePosition(const tN2kMsg &N2kMsg) {
  double Latitude;
  double Longitude;
  char buf[100];

  if ( ParseN2kPGN129025(N2kMsg, Latitude, Longitude) ) {
    snprintf(buf, sizeof(buf), "{\"altitude\":%f,\"latitude\":%f,\"longitude\":%f}", 0.0 , Latitude, Longitude);
    sigK.sendDelta("navigation.position", String(buf));        
  }
}


//*****************************************************************************
void HandleCOG_SOG(const tN2kMsg &N2kMsg) {
  unsigned char SID;
  tN2kHeadingReference ref;
  double COG;
  double SOG;

  if ( ParseN2kPGN129026(N2kMsg, SID, ref, COG, SOG) ) {
    sigK.sendDelta("navigation.courseOverGroundTrue", COG);
    sigK.sendDelta("navigation.speedOverGround", SOG);    
  }
}


//*****************************************************************************
void HandleWind(const tN2kMsg &N2kMsg) {
  unsigned char SID;
  tN2kWindReference WindReference;

  double WindAngle, WindSpeed;

  if ( ParseN2kWindSpeed(N2kMsg, SID, WindSpeed, WindAngle, WindReference) ) {
    if ( WindReference == N2kWind_Apparent ) {
      sigK.sendDelta("environment.wind.angleApparent", WindAngle);
      sigK.sendDelta("environment.wind.speedApparent", WindSpeed);      
    }
    else if ( WindReference == N2kWind_True_boat ) {
      sigK.sendDelta("environment.wind.angleTrueGround", WindAngle);
      sigK.sendDelta("environment.wind.speedTrue", WindSpeed);      
    }
    else if ( WindReference == N2kWind_True_water ) {
      sigK.sendDelta("environment.wind.angleTrueWater", WindAngle);
      sigK.sendDelta("environment.wind.speedTrue", WindSpeed);      
    }
  }
}


//*****************************************************************************
void HandleLog(const tN2kMsg & N2kMsg) {

  uint16_t DaysSince1970;
  double SecondsSinceMidnight;
  uint32_t Log;
  uint32_t TripLog;

  if ( ParseN2kDistanceLog(N2kMsg, DaysSince1970, SecondsSinceMidnight, Log, TripLog) ) {
    sigK.sendDelta("navigation.trip.log", (int)TripLog);
    sigK.sendDelta("navigation.log", (int)Log);
  }
}


//*****************************************************************************
void HandleWaterTemp(const tN2kMsg & N2kMsg) {

  unsigned char SID;
  double OutsideAmbientAirTemperature;
  double AtmosphericPressure;
  double WaterTemperature;

  if ( ParseN2kPGN130310(N2kMsg, SID, WaterTemperature, OutsideAmbientAirTemperature, AtmosphericPressure) ) {
    // sigK.sendDelta("environment.outside.temperature", OutsideAmbientAirTemperature);
    // sigK.sendDelta("environment.outside.pressure", AtmosphericPressure);
    sigK.sendDelta("environment.water.temperature", WaterTemperature);    
  }
}


//*****************************************************************************
void HandleRudder(const tN2kMsg & N2kMsg) {

  double RudderPosition;
  unsigned char Instance;
  tN2kRudderDirectionOrder RudderDirectionOrder;
  double AngleOrder;

  if ( ParseN2kRudder(N2kMsg, RudderPosition, Instance, RudderDirectionOrder, AngleOrder) ) {
    sigK.sendDelta("steering.rudderAngle", RudderPosition);    
  }
}


//*****************************************************************************
void HandleGNSS(const tN2kMsg & N2kMsg) {

  unsigned char SID;
  uint16_t DaysSince1970;
  double SecondsSinceMidnight;
  double Latitude;
  double Longitude;
  double Altitude;
  tN2kGNSStype GNSStype;
  tN2kGNSSmethod GNSSmethod;
  unsigned char nSatellites;
  double HDOP;
  double PDOP;
  double GeoidalSeparation;
  unsigned char nReferenceStations;
  tN2kGNSStype ReferenceStationType;
  uint16_t ReferenceSationID;
  double AgeOfCorrection;
  char buf[100];

  if ( ParseN2kGNSS(N2kMsg, SID, DaysSince1970, SecondsSinceMidnight, Latitude, Longitude, Altitude, GNSStype, GNSSmethod,
                    nSatellites, HDOP, PDOP, GeoidalSeparation,
                    nReferenceStations, ReferenceStationType, ReferenceSationID, AgeOfCorrection) ) {
    
    sigK.addDeltaValue("navigation.gnss.type", GNSStype);
    sigK.addDeltaValue("navigation.gnss.horizontalDilution", HDOP);
    sigK.addDeltaValue("navigation.gnss.positionDilution", PDOP);
    
    sigK.sendDelta();
    
    sigK.addDeltaValue("navigation.gnss.satellites", nSatellites);
    sigK.addDeltaValue("navigation.gnss.geoidalSeparation", GeoidalSeparation);
    sigK.addDeltaValue("navigation.gnss.differentialAge", AgeOfCorrection);

    sigK.sendDelta();
    
    sigK.addDeltaValue("navigation.gnss.differentialReference", ReferenceSationID);
    snprintf(buf, sizeof(buf), "{\"altitude\":%f,\"latitude\":%f,\"longitude\":%f}", Altitude , Latitude, Longitude);
    sigK.addDeltaValue("navigation.position", String(buf));
    
    sigK.sendDelta();
  }
}


//*****************************************************************************
// Function to check if SourceAddress has changed (due to address conflict on bus)

void CheckSourceAddressChange() {
  int SourceAddress = NMEA2000.GetN2kSource();

  if (SourceAddress != NodeAddress) { // Save potentially changed Source Address to NVS memory
    preferences.begin("nvs", false);
    preferences.putInt("LastNodeAddress", SourceAddress);
    preferences.end();
    Serial.printf("Address Change: New Address=%d\n", SourceAddress);
  }
}


//*****************************************************************************
void loop() {

  NMEA2000.ParseMessages();

  CheckSourceAddressChange();

  sigK.handle();

  // Dummy to empty input buffer to avoid board to stuck with e.g. NMEA Reader
  if ( Serial.available() ) {
    Serial.read();
  }
}
