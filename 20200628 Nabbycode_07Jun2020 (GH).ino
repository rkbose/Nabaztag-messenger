//
// ESP32 controller for Nabby Nabaztag with MP304a MP3 chip by RKB
// Board used: ESP32Dev module 115200
//  (COM4 on ACER, right-usb-poort; not all usb-cables are OK)
//

#define VERSION  "7Jun2020a"
#include <HardwareSerial.h>
#include <MP3Player_KT403A-Raj.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <AsyncUDP.h>
#include <ESPmDNS.h>
#include "freertos/timers.h"
#include "Wire.h"    //used for I2C communication with IO expander MCP23017

// Help text
String helpText = "---- Nabby commands ----\n"
                  "h:  display help\n"
                  "?:  display info\n"
                  "d:  doorbell\n"
                  "q:  quick down call\n"
                  "ix: left ear back, goto position x -> speed = -200\n"
                  "mx: left ear forward, goto position x -> speed = 200\n"
                  "jx:  right ear forward, goto position x -> speed= -200\n"
                  "lx:  right ear back, goto position x -> speed= 200\n"
                  "k:  stop both ears\n"
                  "sx: specify musiq\n"
                  "ax: specify file in folder mp3\n"
                  "v-: decrease volume\n"
                  "v+: increase volume\n"
                  "9:  get Volume\n"
                  "ex: set equalizer 1..5 (default = 2) 0(Normal) 1(Pop) 2(Rock) 3(Jazz) 4(Classic) 5(Bass)\n"
                  "n:  play the next song\n"
                  "p:  pause the MP3 player\n"
                  "r:  resume the MP3 player\n"
                  "5:  play the previous song\n"
                  "6:  play loop for all the songs\n"
                  "-------------------------\n";

// Left and Right motor pins and PWM definitions
#define MOTOR_L_IN1  27   // D27 is Left ear motor IN1
#define MOTOR_L_IN2  14   // D14 is Left ear motor IN2
#define MOTOR_R_IN1  12   // D12 is Right ear motor IN1
#define MOTOR_R_IN2  13   // D13 is Right ear motor IN2
#define PWM_LED_D23     0  // PWM channel LED on D23
#define PWM_MOTOR_L_IN1 1  // PWM channel Left ear motor IN1
#define PWM_MOTOR_L_IN2 2  // PWM channel Left ear motor IN2
#define PWM_MOTOR_R_IN1 3  // PWM channel Right ear motor IN1
#define PWM_MOTOR_R_IN2 4  // PWM channel Right ear motor IN2

// Left and Right Encoder pins
#define ENC_L  32  // left ear encoder input
#define ENC_R  33  // right ear encoder input

// setting PWM properties for heartbeat LED and PWM for left&right ear motors
const int freq = 5000;       // in Hz
const int pwmChannel0 = 0;   // PWM channels: 0 to 15
const int resolution = 8;    // Resolution in bits: 8, 10, 12, 15

// Heartbeat LD and Nabby head button
#define LED_D23   23         // heartbeat LED on pin 23
#define NabbysButton 25      // Nabbys head button on pin 25

// volatiles for LED blinking and PWM channel
// volatile int toggle = 1;    // used for slow flashing led
volatile int prev_secs = 0; // previous time readin from millis() for Heartbeat led
volatile int prev_secs_cLeds = 0; // prev. time reading from millis() for colour leds
volatile int PWM = 0;       // temp variable for duty cycle of LED
volatile int pwmsign = 20;  // incremental step for PWM of LED
volatile byte colourLow = 1;   // value for colour LED on PORT A
volatile byte colourHigh = 1;   // value for colour LED on PORT B
volatile int cledArrayindex = 0;  // index used in the const array with cled colours
volatile int LedNotificationCount = 0; // TRUE if Belly LED's are ON

// left Ear globals and definitions
volatile unsigned long leftEarLastTriggerTime = 0;
volatile unsigned long leftEarTriggerTime = 0;
volatile unsigned int leftEarState = 0;    // states in statemachine
volatile unsigned int leftEarTargetPosition = 0;
volatile unsigned int leftEarPosition = 0;
volatile boolean leftEarPositionKnown = false;
#define LEFT_EAR_STATE_INIT     0  // Initialize state
#define LEFT_EAR_STATE_RUNNING  1  // running state: trying to maintain position

// right Ear globals and definitions
volatile unsigned long rightEarLastTriggerTime = 0;
volatile unsigned long rightEarTriggerTime = 0;
volatile unsigned int rightEarState = 0;    // states in statemachine
volatile unsigned int rightEarTargetPosition = 0;
volatile unsigned int rightEarPosition = 0;
volatile boolean rightEarPositionKnown = false;
#define RIGHT_EAR_STATE_INIT     0  // Initialize state
#define RIGHT_EAR_STATE_RUNNING  1  // running state: trying to maintain position

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

// initialize timers
hw_timer_t   *leftEarTimer = NULL;     // timer used for leftEar statemachine
hw_timer_t   *rightEarTimer = NULL;     // timer used for rightEar statemachine

// buffers for receiving and sending data over UDP
//#define UDP_TX_PACKET_MAX_SIZE  250
//char packetBuffer[UDP_TX_PACKET_MAX_SIZE + 1]; //buffer to hold incoming packet,
//char  ReplyBuffer[] = "Thanks, Nabby\r\n";       // a string to send back
AsyncUDP udp;

WiFiMulti wifiMulti;

String IpAddress2String(const IPAddress& ipAddress) // convert IP addr to string
{
  return String(ipAddress[0]) + String(".") + \
         String(ipAddress[1]) + String(".") + \
         String(ipAddress[2]) + String(".") + \
         String(ipAddress[3])  ;
}

void connectWifi() {
  WiFi.mode(WIFI_STA); // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.disconnect();
  delay(100);

  int n = WiFi.scanNetworks();   // WiFi.scanNetworks will return the number of networks found
  Serial.println("scan done");
  if (n == 0) {
    Serial.println("no networks found");
  } else {
    Serial.printf("Nr networks found: %d\n", n);
  }
  //  wifiMulti.addAP("SSID OF NETWORK", "WPA CODE FOR YOUR NETWORK");
  wifiMulti.addAP("SSID OF NETWORK", "WPA CODE FOR YOUR NETWORK");
  wifiMulti.addAP("SSID OF NETWORK", "WPA CODE FOR YOUR NETWORK");

  Serial.print("Connecting Wifi -");
  while (wifiMulti.run() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to network");

  Serial.print("WiFi connected: ");
  Serial.print(WiFi.SSID());
  Serial.print(";   IP addr: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("Nabby")) {
    MDNS.addService("mydoorbell", "udp", 1234); // Announce service on port x
    Serial.println("MDNS responder started");
  }
  Serial.printf("UDP server on port %d\n", 1234);
}

String parseNabbyCmd(String cmdString) {
  char cmdChar1 = 0;
  char cmdChar2 = 0;
  String returnString = "Nabby did not understand";

  if (cmdString.startsWith("Nabby:") and cmdString.length() > 7) {
    cmdChar1 = cmdString.charAt(6);
    cmdChar2 = cmdString.charAt(7);

    switch (cmdChar1) {
      case 'p':
        PlayPause();
        returnString = "Pause the MP3 player";
        break;
      case 'r':
        PlayResume();
        returnString = "Resume the MP3 player";
        break;
      case 'n':
        PlayNext();
        returnString = "Play the next song";
        break;
      case '5':
        PlayPrevious();
        returnString = "Play the previous song";
        break;
      case '6':
        PlayLoop();
        returnString = "Play loop for all the songs";
        break;
      case 'v':
        if (cmdChar2 == '+') {
          IncreaseVolume();
          returnString = "Increase volume [v+]";
        }
        if (cmdChar2 == '-') {
          DecreaseVolume();
          returnString = "Decrease volume [v-]";
        }
        break;
      case '9':
        GetVolume();
        returnString = "Get Volume";
        break;
      case 's':
        SpecifyMusicPlay(cmdChar2 - '0');
        returnString = ("Specify music");
        break;
      case 'a':
        SpecifyMusicInMp3(cmdChar2 - '0');
        returnString = ("Specify music from MP3 folder");
        break;
      case 'e':
        SetEqualizer(cmdChar2 - '0');
        returnString = "Set Equalizer";
        break;
      case 'i':  // left ear left
        leftEarGoto(cmdChar2 - '0', -200);
        returnString = "leftEarGoto left";
        break;
      case 'm':    //left ear right
        leftEarGoto(cmdChar2 - '0', 200);
        returnString = "leftEarGoto right";
        break;
      case 'j':    // right ear left
        rightEarGoto(cmdChar2 - '0', -200);
        returnString = "rightEarGoto left";
        break;
      case 'l':   // right ear right
        rightEarGoto(cmdChar2 - '0', 200);
        returnString = "Right ear Speed 200";
        break;
      case 'k': // stop both ears
        leftEarSetSpeed(1);
        rightEarSetSpeed(1);
        returnString = "Stop both ears";
        break;
      case 'd': // doorbell
        SpecifyMusicInMp3(1);
        LedNotificationCount = 30;
        leftEarGoto(9, 200);
        rightEarGoto(9, -200);
        //        delay(10000);
        //        leftEarGoto(3, 200);
        //        rightEarGoto(3, -200);
        returnString = "Doorbell sounded";
        break;
      case 'q': // come Quickly downstairs
        SpecifyMusicInMp3(2);
        returnString = "Quick down sounded";
        break;
      case '?': // return information
        returnString = "Hello, I am Nabby\n" + String(VERSION) + "\nSSID, IP addr: " +
                       WiFi.SSID() + "; " + IpAddress2String(WiFi.localIP()) + "\n";
        break;
      case 'h': // return help text
        returnString = helpText;
        break;
      default:
        returnString = "Nabby did not understand. Use 'h' for help";
        break;
    }
  } else {
    returnString = "Nabby did not understand. Use 'h' for help";
  }
  return (returnString);
}

void handleUdp() {
  if (udp.listen(1234)) {
    udp.onPacket([](AsyncUDPPacket packet) {
      String myString = (const char*)packet.data();
      String reply = parseNabbyCmd(myString);
      packet.print(reply);
    });
  }
}

void leftEarSetSpeed(int speed) {
  if (speed > 0) {
    if (speed > 255) speed = 255;
    ledcWrite(PWM_MOTOR_L_IN1, speed);
    ledcWrite(PWM_MOTOR_L_IN2, 0);
//    Serial.printf("L-speed positive: %d \n", speed);
  }
  if (speed < 0) {
    if (speed < -255) speed = -255;
    ledcWrite(PWM_MOTOR_L_IN1, 0);
    ledcWrite(PWM_MOTOR_L_IN2, -1 * speed);
//    Serial.printf("L-speed negative: %d \n", -1 * speed);
  }
}

void IRAM_ATTR leftEarEncoderInterrupt() {
  // Debounch the encoder signal. Note Nabby's encoders output a triangle, not a square wave. This causes multiple interrupts.
  // The timer interrupts will mask the encoder interrupts for a short period. Better would have been a hardware schmitt trigger
  detachInterrupt(digitalPinToInterrupt(ENC_L));  // disable interrupt for debouncing
  timerAlarmWrite(leftEarTimer, 500000, false);  // true: reload, false: trigger one time
  timerWrite(leftEarTimer, 0);     // reset timer
  timerAlarmEnable(leftEarTimer);

  leftEarTriggerTime = millis();

  if ((leftEarState == LEFT_EAR_STATE_RUNNING) and (leftEarTriggerTime - leftEarLastTriggerTime) > 1300) {
    leftEarPositionKnown = true;
    leftEarPosition = 0;
  }
  leftEarPosition++;
  //  Serial.printf("leftEarInt,  prev.state: %d, pos: %d, known: %d,  time: %d\n", leftEarState, leftEarPosition, leftEarPositionKnown, (leftEarTriggerTime - leftEarLastTriggerTime));
  leftEarLastTriggerTime = leftEarTriggerTime;
  if (leftEarPositionKnown and (leftEarPosition == leftEarTargetPosition)) {
    leftEarSetSpeed(1);
    leftEarState = LEFT_EAR_STATE_INIT;
    leftEarPositionKnown = false;
  }
  else leftEarState = LEFT_EAR_STATE_RUNNING;
}

void IRAM_ATTR leftEarTimerInterrupt() {
  //  Serial.println("leftEarTimerInt");
  attachInterrupt(digitalPinToInterrupt(ENC_L), leftEarEncoderInterrupt, RISING); //RISING, FALLING, HIGH
}

void leftEarInitialize() {
  pinMode(ENC_L, INPUT);  //A 10K external resistor is used as pull down. Enabling internal pullup would create divider
  leftEarState = LEFT_EAR_STATE_INIT;
  leftEarTargetPosition = 0;
  leftEarPosition = 0;
  leftEarPositionKnown = false;
  leftEarLastTriggerTime = millis();
  leftEarTimer = timerBegin(0, 80, true);     // initialize timer to timer 0; prescaler 80 (= 1Mhz); and TRUE (=cnt up)
  timerAttachInterrupt(leftEarTimer, &leftEarTimerInterrupt, true);  // interrupt function. TRUE (= edge type)
  attachInterrupt(digitalPinToInterrupt(ENC_L), leftEarEncoderInterrupt, RISING); //RISING, FALLING, HIGH, LOW
}

void leftEarGoto(unsigned int targetPosition, int speed) {
  portENTER_CRITICAL(&mux);
  leftEarTargetPosition = targetPosition;
  portEXIT_CRITICAL(&mux);
  leftEarSetSpeed(speed);
}

void rightEarSetSpeed(int speed) {
  if (speed > 0) {
    if (speed > 255) speed = 255;
    ledcWrite(PWM_MOTOR_R_IN1, speed);
    ledcWrite(PWM_MOTOR_R_IN2, 0);
//    Serial.printf("R-speed positive: %d \n", speed);
  }
  if (speed < 0) {
    if (speed < -255) speed = -255;
    ledcWrite(PWM_MOTOR_R_IN1, 0);
    ledcWrite(PWM_MOTOR_R_IN2, -1 * speed);
//    Serial.printf("R-speed negative: %d \n", -1 * speed);
  }
}

void IRAM_ATTR rightEarEncoderInterrupt() {
  // Debounch the encoder signal. Note Nabby's encoders output a triangle, not a square wave. This causes multiple interrupts.
  // The timer interrupts will mask the encoder interrupts for a short period. Better would have been a hardware schmitt trigger
  detachInterrupt(digitalPinToInterrupt(ENC_R));  // disable interrupt for debouncing
  timerAlarmWrite(rightEarTimer, 500000, false);  // true: reload, false: trigger one time
  timerWrite(rightEarTimer, 0);     // reset timer
  timerAlarmEnable(rightEarTimer);
  rightEarTriggerTime = millis();
  if ((rightEarState == RIGHT_EAR_STATE_RUNNING) and (rightEarTriggerTime - rightEarLastTriggerTime) > 1300) {
    rightEarPositionKnown = true;
    rightEarPosition = 0;
  }
  rightEarPosition++;
  //  Serial.printf("rightEarInt,  prev.state: %d, pos: %d, known: %d,  time: %d\n", rightEarState, rightEarPosition, rightEarPositionKnown, (rightEarTriggerTime - rightEarLastTriggerTime));
  rightEarLastTriggerTime = rightEarTriggerTime;
  if (rightEarPositionKnown and (rightEarPosition == rightEarTargetPosition)) {
    rightEarSetSpeed(1);
    rightEarState = RIGHT_EAR_STATE_INIT;
    rightEarPositionKnown = false;
  }
  else rightEarState = RIGHT_EAR_STATE_RUNNING;
}

void IRAM_ATTR rightEarTimerInterrupt() {
  //  Serial.println("rightEarTimerInt");
  attachInterrupt(digitalPinToInterrupt(ENC_R), rightEarEncoderInterrupt, RISING); //RISING, FALLING, HIGH
}

void rightEarInitialize() {
  pinMode(ENC_R, INPUT);  //A 10K external resistor is used as pull down. Enabling internal pullup would create divider
  rightEarState = RIGHT_EAR_STATE_INIT;
  rightEarTargetPosition = 0;
  rightEarPosition = 0;
  rightEarPositionKnown = false;
  rightEarLastTriggerTime = millis();
  rightEarTimer = timerBegin(1, 80, true);     // initialize timer to timer 1; prescaler 80 (= 1Mhz); and TRUE (=cnt up)
  timerAttachInterrupt(rightEarTimer, &rightEarTimerInterrupt, true);  // interrupt function. TRUE (= edge type)
  attachInterrupt(digitalPinToInterrupt(ENC_R), rightEarEncoderInterrupt, RISING); //RISING, FALLING, HIGH, LOW
}

void rightEarGoto(unsigned int targetPosition, int speed) {
  portENTER_CRITICAL(&mux);
  rightEarTargetPosition = targetPosition;
  portEXIT_CRITICAL(&mux);
  rightEarSetSpeed(speed);
}

int mappingHeartbeat(int x) { //optional non linear heartbeat LED light intensity
  //return (x*x*x/(255^3));
  return (x);
}

const byte cledLowArray [] = {   //LED sequence to be played (for low byte on IO expander: GPA)
  // A0, A1, A2: G,R,B - Belly
  // A3, A4, A5: G, R, B - LEFT
  // A6, A7: G, R - MID
  B00001000, // 0  LG
  B01000010, // 1  MG
  B00000000, // 2  RG
  // B00000010, // 3 OFF
  B00010000, // 4  LR
  B10000010, // 5  MR
  B00000000, // 6  RR
  // B00000010, // 7 OFF
  B00100000, // 8  LB
  B00000010, // 9  MB
  B00000000 // 10  RB
  // B00000010  // 11  OFF
};

const byte cledHighArray [] = {    //LED sequence to be played (for High byte on IO expander: GPB)
  // B0: B - MID
  // B1, B2, B3: G,R,B - RIGHT
  // B4, B5, B6: G,R,B - TOP
  B00100000, // 0   LG
  B00100000, // 1   MG
  B00100010, // 2   RG
  //  B00100000, // 3   OFF
  B00100000, // 4   LR
  B00100000, // 5   MR
  B00100100, // 6   RR
  // B00100000, // 7   OFF
  B00100000, // 8   LB
  B00100001, // 9   MB
  B00101000 // 10   RB
  // B00100000 // 11  OFF
};

void blinkLeds(void) {  //time based routine for LED blinking and Nabbys button polling
  int secs = millis();

  if (secs > (prev_secs + 15)) { // each 35 mSec the heartbeat Led changes brightness
    prev_secs = secs;
    PWM = PWM + pwmsign;
    if (PWM > 150) {
      pwmsign = -2;
      PWM = 150;
    }
    if (PWM < 0) {
      pwmsign = 2;
      PWM = 0;
    }
    ledcWrite(PWM_LED_D23, mappingHeartbeat(PWM));  // heartbeat LED
  }

  if ((LedNotificationCount > 0) and (secs > (prev_secs_cLeds + 350))) { // each 250 mSec the Leds change
    prev_secs_cLeds = secs;
    LedNotificationCount--;
    if (LedNotificationCount < 0) LedNotificationCount = 0;

    cledArrayindex++;
    if (cledArrayindex > sizeof(cledLowArray) - 1) cledArrayindex = 0;
    colourLow = cledLowArray[cledArrayindex];
    colourHigh = cledHighArray[cledArrayindex];
    portMCP23017(colourLow, colourHigh);

    if (digitalRead(NabbysButton) == 0) {
      PlayPause();
      LedNotificationCount = 0;
    }
  }
  if (LedNotificationCount == 0) {
    portMCP23017(0, 0);
    leftEarSetSpeed(1);
    rightEarSetSpeed(1);
  }

  /*    if (toggle == 1) {
        //    digitalWrite (2, LOW); // blink onboard ESP32-LED: make off
        portMCP23017(0xff, colour);
        toggle = 2;
      }
      else {
        //    digitalWrite (2, HIGH); // blink onboard ESP32-LED: make ON
        portMCP23017(0, colour);
        toggle = 1;
      }
  */
}

void initMCP23017() {
  Wire.begin(21, 22);
  // set IO pins to outputs
  Wire.beginTransmission(0x20); // with three address pins @gnd, address is 0x20
  Wire.write(0x00); // IODIRA register
  Wire.write(0x00); // set all ports on port A to outputs
  Wire.endTransmission();
  Wire.beginTransmission(0x20); // with three address pins @gnd, address is 0x20
  Wire.write(0x01); // IODIRB register
  Wire.write(0x00); // set all ports on port B to outputs
  Wire.endTransmission();
}

void portMCP23017(byte valueA, byte valueB) {
  Wire.beginTransmission(0x20);
  Wire.write(0x12); // GPIOA
  Wire.write(valueA);
  Wire.endTransmission();
  Wire.beginTransmission(0x20);
  Wire.write(0x13); // GPIOB
  Wire.write(valueB);
  Wire.endTransmission();
}

void setup() {
  Serial.begin(115200);  // debug interface
  Serial2.begin(9600, SERIAL_8N1, 16, 17);  // MP3 interface
  Serial.print("Initializing Nabby: ");
  Serial.println(VERSION);

  pinMode(2, OUTPUT); // enable on-board led
  pinMode(LED_D23, OUTPUT); // enable external LED on D23
  pinMode(NabbysButton, INPUT_PULLUP); // Nabbys head button

  // initialize I2C bus for MCP23017 IO expander
  initMCP23017();

  // make motordriver pins output
  pinMode(MOTOR_L_IN1, OUTPUT);
  pinMode(MOTOR_L_IN2, OUTPUT);
  pinMode(MOTOR_R_IN1, OUTPUT);
  pinMode(MOTOR_R_IN2, OUTPUT);

  // initialize PWM channels for led and motorpins
  ledcSetup(PWM_LED_D23, freq, resolution);
  ledcAttachPin(LED_D23, PWM_LED_D23); // attach the channel to the GPIO2 to be controlled

  ledcSetup(PWM_MOTOR_L_IN1, freq, resolution);
  ledcAttachPin(MOTOR_L_IN1, PWM_MOTOR_L_IN1); // attach the channel to the GPIO

  ledcSetup(PWM_MOTOR_L_IN2, freq, resolution);
  ledcAttachPin(MOTOR_L_IN2, PWM_MOTOR_L_IN2); // attach the channel to the GPIO

  ledcSetup(PWM_MOTOR_R_IN1, freq, resolution);
  ledcAttachPin(MOTOR_R_IN1, PWM_MOTOR_R_IN1); // attach the channel to the GPIO

  ledcSetup(PWM_MOTOR_R_IN2, freq, resolution);
  ledcAttachPin(MOTOR_R_IN2, PWM_MOTOR_R_IN2); // attach the channel to the GPIO

  //  timerAlarmWrite(leftEarTimer, 5000000, false);  // TRUE = reload
  //  timerAlarmEnable(leftEarTimer);
  leftEarInitialize();
  rightEarInitialize();

  connectWifi();  // connect to WiFi access point
  //  delay(100);

  SelectPlayerDevice(0x02);     // Select SD card as the player device.
  SetVolume(25);                // Set the volume, the range is 0x00 to 0x1E.
  SetEqualizer(2);              // 0(Normal)   1(Pop)   2(Rock)    3(Jazz)   4(Classic)   5(Bass)
  leftEarGoto(3, -200);
  rightEarGoto(3, 200);
  delay(7000);
  Serial.println("end of setup()");
}

void loop() {
  char recvChar = 0;
  char recvChar2 = 0;

  blinkLeds();

  if (Serial2.available()) {     // check if MP3 player replied
    char recvMp3Char = Serial2.read();
 //   Serial.printf("Received: %d    0x%x\n", recvMp3Char, recvMp3Char);
  }

  if (Serial.available()) {     // check for cmd's via console port
    recvChar = Serial.read();
    if (Serial.available()) recvChar2 = Serial.read();
    String tempString = "Nabby:";
    tempString += recvChar;
    tempString += recvChar2;
    Serial.println(parseNabbyCmd(tempString));
    Serial.flush();
  }
  handleUdp();  // handle and parse commands received via UDP
  delay(100);
}
