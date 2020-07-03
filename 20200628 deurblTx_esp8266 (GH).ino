//
// WiFi doorbell to signal Nabby (NabzTag ESP32) by RKB
//  ESP8266 Joyit module (small 8 pin module),
//  Use Board: Generic ESP8266 module
//     (on ACER: COM115200, no-usb cable, com3. Don't forget to put switch in prgrm mode)
//

#define VERSION  "7Jun2020b"

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include "WiFiUdp.h"

const char* ssid = "SSID OF NETWORK";
const char* password =  "WPA CODE OF NETWORK";
volatile int prev_secs = 0; // previous seconds: used for timing of ping to Nabby
volatile int packetRcvdCounter = 6; // counter, decremented on non rcvd pings to reset board

WiFiUDP Udp;

// buffers for receiving and sending data
char rxBuffer[UDP_TX_PACKET_MAX_SIZE + 1]; //buffer to hold incoming packet,
char  NabbyDoorbellCmd[] = "Nabby:d.";       // string to send to Nabby: sound doorbell
char  NabbyGetInfoCmd[] = "Nabby:?:";       // string to send to Nabby: GetInfo

void scanNetworks() {
  int numberOfNetworks = WiFi.scanNetworks();
  Serial.print("\nNumber of networks found: ");
  Serial.println(numberOfNetworks);
}

void connectToNetwork() {
  WiFi.begin(ssid, password);
  Serial.print("Establishing connection to WiFi: ");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.print("\nConnected to network: ");
  Serial.print(WiFi.SSID());
  Serial.print("; ");
  Serial.println(WiFi.localIP());
}

void setup() {
  pinMode(2, INPUT);  //DIO-2 is input for doorbell button
  Serial.begin(115200);
  delay(100);
  scanNetworks();
  connectToNetwork();

  Udp.begin(1234);  // receive udp packets on port 1234

  if (MDNS.begin("doorbell sender")) {
    Serial.println("mDNS responder started");
  }
  Serial.println("Sending mDNS query");
  int n = MDNS.queryService("mydoorbell", "udp", 1234); // Send query for mydoorbell services
  Serial.println("mDNS query done");
  if (n == 0) {
    Serial.println("no mDNS services found");
  } else Serial.printf("Found 'mydoorbell' services: %d\n", n);
  Serial.println(VERSION);
  Serial.printf("ChipID: ESP_%06X\n", ESP.getChipId());
  packetRcvdCounter = 6; // initially wait 6*10 seconds for Nabbies response before timeout
}

void loop() {
  int secs = millis();
  int inputVal = digitalRead(2);  // read doorbell button

  //  printf("digital in = %d\n", inputVal);
  if (inputVal == LOW) {
    Serial.println("\nSending doorbell cmd to Nabby");
    Udp.beginPacket(MDNS.IP(0), 1234);  // send udp packet to doorbell receiver
    Udp.write(NabbyDoorbellCmd);
    Udp.endPacket();
    delay(500);
  } else {
    if (packetRcvdCounter <= 3) {
      Serial.println("\nChecking Nabby");   // Check if Nabby is available: sent ping command
      Udp.beginPacket(MDNS.IP(0), 1234);  // send udp packet to doorbell receiver
      Udp.write(NabbyGetInfoCmd);
      Udp.endPacket();
      delay(500);
    }
  }

#define xSeconds 10000 // in mSecs
  if (secs > (prev_secs + xSeconds)) { // every x seconds
    prev_secs = secs;
    Serial.printf("x seconds passed  packetRcvdCounter = %d\n", packetRcvdCounter);
    if (packetRcvdCounter > 0 ) packetRcvdCounter = packetRcvdCounter - 1;
  }

  // check for received packets
  int packetSize = Udp.parsePacket();
  if (packetSize) {   // packet received
    packetRcvdCounter = 6; // reset counter to check reception, max 1 minute without reception
    int n = Udp.read(rxBuffer, UDP_TX_PACKET_MAX_SIZE);
    rxBuffer[n] = 0;
    Serial.print("Nabby's response:");
    Serial.println(rxBuffer);
  }

  if (packetRcvdCounter == 0 ) {
    Serial.printf("Restarting board\n");
    ESP.restart(); // restart board
  }
  delay(200);
}
