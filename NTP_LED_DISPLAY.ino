// Code written to display current time with initial time sync via NTP
// NTP Code from Time_NTP.pde example from Time library
// 
// Author: Charles Dunbar

#include <ht1632c.h>
#include <Time.h> 
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>

// Need to use port names of ATMEGA2560, not Arduino 2560
// See http://arduino.cc/en/Hacking/PinMapping2560
// PortC maps to these arduino pins below
// ht1632c dotmatrix = ht1632c(&PORTC, 30, 31, 32, 33, GEOM_32x16, 3);

// Set up LED matrix
ht1632c led = ht1632c(&PORTC, 7, 6, 5, 4, GEOM_32x16, 3);
// Set up ethernet/timezone
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; 
IPAddress timeServer(216, 228, 192, 69); // nist-time-server.eoni.com
const int timeZone = -7;  // Pacific Daylight Time (USA)
EthernetUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets
// Variables used for printing the time
String timeString;
char stringBuf[50];

void setup () {  
  Serial.begin(9600);
  Serial.println("NTP LED Display Demo");
  led.clear();
  led.pwm(1);
  timeString = String ();
    
  if (Ethernet.begin(mac) == 0) {
    // no point in carrying on, so do nothing forevermore:
    while (1) {
      Serial.println("Failed to configure Ethernet using DHCP");
      delay(10000);
    }
  }
  
  Serial.print("IP number assigned by DHCP is ");
  Serial.println(Ethernet.localIP());
  Udp.begin(localPort);
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
}

time_t prevDisplay = 0; // when the digital clock was displayed

void loop() {  
  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) { //update the display only if time has changed
      prevDisplay = now();
      led.centertext(1, "The time is:", RED);
      digitalClockDisplay();
      led.centertext(8, stringBuf, GREEN);  
    }
  }
}


// Digital clock display of the time
// Build a string, then convert it back to a charArray
void digitalClockDisplay() {
  timeString = "";
  timeString += hour();
  printDigits(minute());
  printDigits(second());
  timeString += " ";
  timeString += day();
  timeString += " ";
  timeString += month();
  timeString += " ";
  timeString += year();
  timeString.toCharArray(stringBuf, 50);
}

// Utility for digital clock display: prints preceding colon and leading 0
void printDigits(int digits){
  timeString += ":";
  if(digits < 10)
    timeString += '0';
  timeString += digits;
}


/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:                 
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}



