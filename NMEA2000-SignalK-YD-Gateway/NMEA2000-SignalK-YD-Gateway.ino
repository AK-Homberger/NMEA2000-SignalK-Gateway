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

// Read PGNs from NMEA2000-Bus and send as Yacht Devices format to SignalK server (UDP)
// Version 0.1, 10.02.2021, AK-Homberger

#define ESP32_CAN_TX_PIN GPIO_NUM_5  // Set CAN TX port to 5 
#define ESP32_CAN_RX_PIN GPIO_NUM_4  // Set CAN RX port to 4

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <NMEA2000_CAN.h>   // This will automatically choose right CAN library and create suitable NMEA2000 object
#include <N2kMessages.h>
#include <time.h>
#include <sys/time.h>

#define Max_YD_Message_Size 500

int NodeAddress;            // To store last Node Address
Preferences preferences;    // Nonvolatile storage on ESP32 - To store LastDeviceAddress

const char* hostname  = "NMEA2000-Gateway";    // Hostname for network discovery
const char* ssid      = "ssid";                // SSID to connect to
const char* ssidPass  = "password";            // Password for wifi


// UPD broadcast to SignalK server
const char * udpAddress = "192.168.0.20"; // UDP broadcast address. Should be the SignalK server
const int udpPort = 4444;                 // YD UDP port


// Create UDP instance
WiFiUDP udp;

uint16_t DaysSince1970 = 0;
double SecondsSinceMidnight = 0;

char YD_msg[Max_YD_Message_Size] = "";


//*****************************************************************************
void setup() {
  uint8_t chipid[6];
  uint32_t id = 0;
  int i = 0;

  // Init USB serial port
  Serial.begin(115200);
  delay(10);

  // Init WiFi connection
  Serial.println("Start WLAN");
  WiFi.begin(ssid, ssidPass);
  WiFi.setHostname("NMEA2000-Gateway");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    i++;
    if (i > 20 ) ESP.restart();
  }

  IPAddress IP = WiFi.localIP();
  Serial.println("");
  Serial.print("AP IP address: ");
  Serial.println(IP);


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
  NMEA2000.SetMsgHandler(MyHandleNMEA2000Msg);

  NMEA2000.Open();
}


//*****************************************************************************
void HandleGNSS(const tN2kMsg & N2kMsg) {

  unsigned char SID;
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

  if ( ParseN2kGNSS(N2kMsg, SID, DaysSince1970, SecondsSinceMidnight, Latitude, Longitude, Altitude,
                    GNSStype, GNSSmethod, nSatellites, HDOP, PDOP, GeoidalSeparation,
                    nReferenceStations, ReferenceStationType, ReferenceSationID, AgeOfCorrection) ) {
  }
}


// 16:29:27.082 R 09F8017F 50 C3 B8 13 47 D8 2B C6

//*****************************************************************************
void N2kToYD_Can(const tN2kMsg &msg, char *MsgBuf) {
  int i, len;
  uint32_t canId = 0;
  char time_str[20];
  char Byte[5];
  unsigned int PF;
  unsigned long MyTime = 0;
  time_t rawtime;
  struct tm  ts;

  len = msg.DataLen;
  if (len > 134) len = 134;

  canId = msg.Source & 0xff;
  PF = (msg.PGN >> 8) & 0xff;

  if (PF < 240) {
    canId = (canId | ((msg.Destination & 0xff) << 8));
    canId = (canId | (msg.PGN << 8));
  } else {
    canId = (canId | msg.PGN << 8);
  }

  canId = (canId | msg.Priority << 26);

  MyTime = (DaysSince1970 * 3600 * 24) + SecondsSinceMidnight;  // Create time from GNSS time

  rawtime = MyTime; // Create time from NMEA 2000 time (UTC)
  ts = *localtime(&rawtime);
  strftime(time_str, sizeof(time_str), "%T.000", &ts); // Create time string

  snprintf(MsgBuf, 25, "%s R %0.8x", time_str, canId); // Set time and canID

  for (i = 0; i < len; i++) {
    snprintf(Byte, 4, " %0.2x", msg.Data[i]);   // Add data fields
    strcat(MsgBuf, Byte);
  }
}


//*****************************************************************************
void MyHandleNMEA2000Msg(const tN2kMsg &N2kMsg) {

  if (N2kMsg.PGN == 129029L) HandleGNSS(N2kMsg);   // Just to get time

  N2kToYD_Can(N2kMsg, YD_msg);            // Create YD message from PGN

  udp.beginPacket(udpAddress, udpPort);   // Send to UDP
  udp.println(YD_msg);
  udp.endPacket();

  // Serial.println(YD_msg);
}


//*****************************************************************************
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

  // Dummy to empty input buffer to avoid board to stuck with e.g. NMEA Reader
  if ( Serial.available() ) {
    Serial.read();
  }
}
