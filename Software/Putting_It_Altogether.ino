#include <DNSServer.h> //Used by WiFiManager
#include <ESP8266WebServer.h>
#include <WiFiManager.h> //Creates an AP and a captive portal to login to an existing AP
#include <ESP8266WiFi.h> 
#include <ESP8266mDNS.h> //Resolves hostname from *.local to given IP Address
#include <WiFiClient.h>
#include <WiFiUdp.h> //needed for NTP client
#include <Ticker.h> //To perform non-blocking time based ops
#include <ESP8266Ping.h>
#include <TimeLib.h>
#include <FastLED.h>
#include <SPI.h>
#include <pwm.h>

//GPIO
//http://esp8266.github.io/Arduino/versions/2.0.0/doc/reference.html
//https://nodemcu.readthedocs.io/en/master/en/modules/gpio/
//https://nodemcu.readthedocs.io/en/master/en/modules/spi/

#define SPI_CLK 5
#define SPI_MOSI 7
#define STROBE 8 //Tied to SPI CS
#define PWMOUT 2
#define USRBTN 1

#define DATA_PIN 3 //seems to used for bootloader mode - double confirm
#define NUM_LEDS 6

CRGB leds[NUM_LEDS];
uint8_t max_bright = 255;
uint8_t thishue = 0;
uint8_t deltahue = 15;

#define timeadjust 28800 //GMT +8

//Function Prototypes
//void doHTTP_helloWorld(void); //Test page to check conenctivity
void sendNTPpacket(IPAddress& address);
void do_NTP(void);
void tick_NTP(void);
void do_HTTP(void);
void tick_Time(void);
void tick_LEDBrightness(void);
void tick_rainbow(void);

//Variables Declaration
WiFiServer server(80);
char hostString[16] = {0};

unsigned int localPort = 2390; // local port to listen for UDP packets
IPAddress timeServerIP; // time.google.com NTP server address
const char* ntpServerName = "time.google.com";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
WiFiUDP udp; // A UDP instance to let us send and receive packets over UDP

class currentTime {
  public:
  int hours, mins, secs;
};

currentTime NTP;
currentTime RTC; 

Ticker tickNTP; //declare a class
Ticker tickTime;
Ticker tickRainbow;
Ticker tickBrightness;


bool check_NTP; //time to check NTP?

const char* remote_host = "time.google.com";

bool internet_up = false; //Is internet working?

void setup() {
  // put your setup code here, to run once:
  Serial.begin(250000);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //Tries to connect to an AP using existing credentials from flash memory
  //If connection fails, creates an AP and waits for user input
  wifiManager.autoConnect("ESP8266_AP", "12345678");    
  //If you get here you have connected to the WiFi
  Serial.println("Connected...yeey :)");
  //Wait for 2 secs
  Serial.print("\nHang On.");
  for(int x = 0; x < 40; x++) {
    Serial.print(".");
    delay(50);
  }
  Serial.println("\n");

  /*
  //Set up mDNS responder:
  //First parameter is domain name - *.local
  //Second parameter is IP Address to resolve
  if (!MDNS.begin("IZTest")) {
    Serial.println("Error setting up MDNS responder!");
    while(1) { 
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  //Start TCP (HTTP) server
  server.begin();
  Serial.println("TCP server started\n");
  //Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);
  Serial.println("\n");
  */
  Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());

  Serial.println("Testing connection to NTP server..");
  while(!internet_up) {
    Serial.print("Pinging host ");
    Serial.println(remote_host);
    if(Ping.ping(remote_host)) {
      Serial.println("Success!!\n");
      internet_up = true;
    } else {
      Serial.println("Error :(\n");
      Serial.println("Trying again in 3 secs...");
      for(int y = 0; y < 3; y++)
        delay(1000);
    }
  }

  //SPI.setup(1, spi.MASTER, spi.CPOL_LOW, spi.CPHA_LOW, 24, 8, spi.HALFDUPLEX);
  //SPI.send(1, 0x000000, 0x000000); //dummy out
  SPI.begin();
  SPI.write16(0x000000);
  
  tickNTP.attach(60, tick_NTP);  
  tickTime.attach_ms(250, tick_Time);
  check_NTP = true;
  adjustTime(timeadjust);

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  tickRainbow.attach_ms(100, tick_rainbow);
  tickBrightness.attach(5, tick_LEDBrightness);
}



/*-----------------------------------------------------------------------------
 * Main Code
 *-----------------------------------------------------------------------------*/
void loop() {
  //put your main code here, to run repeatedly:
  //doHTTP_helloWorld(); //Remember to remove comment on actual function
  //do_HTTP(); //modified version on helloWorld
  if(check_NTP) {
    check_NTP = false;
    do_NTP();
  }
  //delay(1);
}



/*-----------------------------------------------------------------------------
 * Custom Functions
 *-----------------------------------------------------------------------------*/
void tick_NTP(void) { //change check_NTP flag
  check_NTP = true;
  return;
}

void tick_LEDBrightness(void) {
  //FastLED.show();   
  FastLED.setBrightness(max_bright);
}

void tick_rainbow(void) {
  fill_rainbow(leds, NUM_LEDS, thishue, deltahue); 
  FastLED.show(); //blocking ooperation anyways
}

void tick_Time(void)
{
    // print the hour, minute and second:
    Serial.print("The time now is ");       // UTC is the time at Greenwich Meridian (GMT)
    if ( hour() < 10 ) {
      Serial.print('0');
    }
    Serial.print(hour());
    Serial.print(':');
    
    if (minute() < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.print(minute());
    Serial.print(':');

    if (second() < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println(second()); // print the second
    //Serial.print("\n");
    Serial.print(dayShortStr(weekday()));
    Serial.print(" ");
    Serial.print(monthStr(month()));
    Serial.print(" ");
    Serial.print(year()); 
    Serial.print("\n");
    Serial.println();
    
    //add SPI code here
    
}

void do_NTP(void) { //update internal time
  tickTime.detach();
  //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP); 
  Serial.print("\n");
  Serial.print("Sending NTP packet to "); Serial.print(timeServerIP); Serial.println("...");
  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  Serial.println("Waiting for reply...");
  int cb = udp.parsePacket();
  unsigned long packettime = millis();
  while(!cb)
  {
    cb = udp.parsePacket();
    if((millis() - packettime) >= 400)
    {
      Serial.println("Timeout.");
      tickTime.attach(1, tick_Time);
      return;
    }
  }
  
  //int cb = udp.parsePacket();
  /*if (!cb) {
    Serial.println("no packet yet");
  }*/
  //else {
    Serial.print("Reply received, length=");
    Serial.println(cb);
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = " );
    Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // print Unix time:
    Serial.println(epoch);
    setTime(epoch);
    adjustTime(timeadjust);
    Serial.print("\n");
    // print the hour, minute and second:
    Serial.print("The time now is ");       // UTC is the time at Greenwich Meridian (GMT)
    if ( hour() < 10 ) {
      Serial.print('0');
    }
    Serial.print(hour());
    Serial.print(':');
    
    if (minute() < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.print(minute());
    Serial.print(':');

    if (second() < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println(second()); // print the second
    Serial.print("\n");
  tickTime.attach_ms(250, tick_Time);
  return;
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress& address) {
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
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
  Serial.print("\n");
  return;
}

/* void doHTTP_helloWorld(void) { //Test page to test connectivity
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  Serial.println("");
  Serial.println("New client");

  // Wait for data from client to become available
  while(client.connected() && !client.available()){
    delay(1);
  }
  
  // Read the first line of HTTP request
  String req = client.readStringUntil('\r');
  
  // First line of HTTP request looks like "GET /path HTTP/1.1"
  // Retrieve the "/path" part by finding the spaces
  int addr_start = req.indexOf(' ');
  int addr_end = req.indexOf(' ', addr_start + 1);
  if (addr_start == -1 || addr_end == -1) {
    Serial.print("Invalid request: ");
    Serial.println(req);
    return;
  }
  req = req.substring(addr_start + 1, addr_end);
  Serial.print("Request: ");
  Serial.println(req);
  client.flush();
  
  String s;
  if (req == "/")
  {
    IPAddress ip = WiFi.localIP();
    String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
    s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 at ";
    s += ipStr;
    s += "</html>\r\n\r\n";
    Serial.println("Sending 200");
  }
  else
  {
    s = "HTTP/1.1 404 Not Found\r\n\r\n";
    Serial.println("Sending 404");
  }
  client.print(s);
  Serial.println("Done with client");
  return;
} */

void do_HTTP(void) { //Test page to test connectivity
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  Serial.println("");
  Serial.println("New client");

  // Wait for data from client to become available
  while(client.connected() && !client.available()){
    delay(1);
  }
  
  // Read the first line of HTTP request
  String req = client.readStringUntil('\r');
  
  // First line of HTTP request looks like "GET /path HTTP/1.1"
  // Retrieve the "/path" part by finding the spaces
  int addr_start = req.indexOf(' ');
  int addr_end = req.indexOf(' ', addr_start + 1);
  if (addr_start == -1 || addr_end == -1) {
    Serial.print("Invalid request: ");
    Serial.println(req);
    return;
  }
  req = req.substring(addr_start + 1, addr_end);
  Serial.print("Request: ");
  Serial.println(req);
  client.flush();
  
  String s;
  if (req == "/")
  {
    IPAddress ip = WiFi.localIP();
    String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
    s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html><meta http-equiv='refresh' content='3'>The UTC time now is ";
    if(hour() < 10)    s += "0";  s += String(hour()) + ":"; 
    if(minute() < 10)  s += "0";  s += String(minute()) + ":"; 
    if(second() < 10)  s += "0";  s += String(second());
    s += "</html>\r\n\r\n";
    Serial.println("Sending 200");
  }
  else
  {
    s = "HTTP/1.1 404 Not Found\r\n\r\n";
    Serial.println("Sending 404");
  }
  client.print(s);
  Serial.println("Done with client");
  return;
}
