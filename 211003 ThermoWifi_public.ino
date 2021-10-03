// ThermoWifi
//
// Interface between 300K thermometer and UDP via WLAN


#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

IPAddress remoteIP(255,255,255,255); // Broadcast UDP measurement packets
// IPAddress remoteIP(192,168,178,24); // Recipient of UDP measurement packets
const unsigned int remoteUdpPort = 6001;  // remote port to send to
const unsigned int localUdpPort  = 6002;  // local port to listen on

// WLAN access credentials
const char* MY_SSID = "xxx";
const char* MY_PWD =  "yyy";

WiFiUDP Udp;

void setup() {

  IPAddress ip(192, 168, 178, 60);
  IPAddress gw(192, 168, 178, 1);
  IPAddress dns(192, 168, 178, 1);
  IPAddress sub(255, 255, 255, 0);

  Serial.begin(9600);
  Serial.println();    
  Serial.println("=== ThermoWifi ===========================================================================");    
  Serial.print(F("SDK-Version: "));
  Serial.println(ESP.getSdkVersion());
  Serial.print(F("ESP8266 Chip-ID: "));
  Serial.println(ESP.getChipId());
  Serial.print(F("ESP8266 Speed in MHz: "));  
  Serial.println(ESP.getCpuFreqMHz());
  Serial.print(F("Free Heap Size in Bytes: "));
  Serial.println(ESP.getFreeHeap());
  Serial.println(F(""));
  
  WiFi.config(ip, gw, sub, dns);
  WiFi.begin(MY_SSID, MY_PWD);
  
  while (WiFi.status() != WL_CONNECTED) //not connected, waiting to connect
    {
      delay(500);
      Serial.print(".");
    }
  
  Serial.println("\n\nWLAN connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print(F("Signal strength: "));
  Serial.println(WiFi.RSSI());
  
  Udp.begin(localUdpPort);
  
  Serial.println("UDP connection ready.");

  // Switch to UART2 (pin16/D8: TXD2, pin7/D7: RXD2)

  delay(500);
  Serial.swap();
  delay(500);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  
  }

void loop() {
  
  char readout[8];
  byte i;

  char  bcd[4];
  boolean negative, hightemp;
  float temp;
  
  digitalWrite(LED_BUILTIN, LOW);
  Serial.print("A");  // trigger thermometer to send all encoded data
  digitalWrite(LED_BUILTIN, HIGH);

  if (Serial.available() > 0){
    digitalWrite(LED_BUILTIN, LOW);
    i = 0;
    while (Serial.available() > 0) {
      if (i<8) readout[i] = Serial.read();
      i++;
    } 
  
    Udp.beginPacket(remoteIP, remoteUdpPort);
    Udp.write("RAW:");
    Udp.write(readout,i);
    Udp.endPacket();

/*
    // Testvalues:
    readout[2] = 0x04;
    readout[3] = 0x13;
    readout[4] = 0x89;
*/
    if (i ==8 && readout[0]==0x02 && readout[7]==0x03) {
      negative = readout[2] & 2;   // 0: temperature above zero, 1: temperatures below zero
      hightemp = readout[2] & 4;   // 0: 3+1 digits, 1: 4 digits
  
      bcd[0] = (readout[3] >> 4);
      bcd[1] = (readout[3] & 15);
      bcd[2] = (readout[4] >> 4);
      bcd[3] = (readout[4] & 15);
      
      Udp.beginPacket(remoteIP, remoteUdpPort);
      Udp.write("#01M");
      if (negative) Udp.write("-");
      if (bcd[0] != 0xB) Udp.write(bcd[0] + 48); // omit digit if it's empty (0xB)
      if (bcd[1] != 0xB) Udp.write(bcd[1] + 48); // omit digit if it's empty (0xB)
      Udp.write(bcd[2] + 48);
      if (!hightemp) Udp.write(".");
      Udp.write(bcd[3] + 48);
      Udp.write("<");
      Udp.endPacket();
    }
    digitalWrite(LED_BUILTIN, HIGH);
  }
    
  delay (1000);
}
