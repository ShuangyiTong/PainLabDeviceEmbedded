#include <WiFiNINA.h>

#include "wpa_cred.h"
#include "device_descriptor.h"

int status = WL_IDLE_STATUS;             // the Wi-Fi radio's status
int ledState = LOW;                       //ledState used to set the LED
unsigned long previousMicrosInfo = 0;     //will store last time Wi-Fi information was updated
unsigned long previousMicrosReport = 0;
const unsigned long reportInterval = 30000;            // interval at which to update the board information
const unsigned long collectInterval = 1000;            // interval at which to collect analog signal
byte buffer[4000]; // micros (4 bytes) + uint16_t (2 bytes) * 3 = 10 bytes per frame
uint16_t bufferPointer = 4; // four bytes are for total byte counts
uint16_t maxbuffer = 2004; // 4 + 10 * 200 = 1004, that's 450 frames
unsigned long consecutiveDroppedFrames = 0;
unsigned long MAXDROPPEDFRAMES = 33; // at least 33 * 450 = 14850 frames dropped
bool allowSend = true;

int Reset = 2;

IPAddress server(192, 168, 0, 125); // painlab server
WiFiClient client;

uint32_t reverseWordEndianness(uint32_t src) { // use -O3 will get rid of the call stack
  uint32_t dest = 0;
  asm ("REV %0, %1"
    : "=r" (dest)
    : "r" (src));

  return dest;
}


void connectToWifi() {
  // attempt to connect to Wi-Fi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to network: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);
  
    // wait 10 seconds for connection:
    delay(5000);
  }

  // you're connected now, so print out the data:
  Serial.println("You're connected to the network");
  Serial.println("---------------------------------------");
}

void sendDeviceDescriptor() {
  uint32_t dataLength = strlen(descriptor);
  uint32_t dataLengthBE = reverseWordEndianness(dataLength);
  client.write(reinterpret_cast<byte*>(&dataLengthBE), 4);
  size_t writtenLen = client.write(descriptor, dataLength); // assume utf-8 works
  allowSend = false;
  Serial.print("descriptor bytes sent: ");
  Serial.println(writtenLen);
}

void connectToPainlabControlPanel() {
  while (1) {
    if (client.connect(server, 8124)) {
      // connect to pain lab control panel
      Serial.println("Connected to control panel");
      break;
    }
    Serial.println("Can't connect to pain lab control panel");
    delay(5000);
  }

  Serial.println("Sending descriptor");
  sendDeviceDescriptor();
}

void setup() {
  analogReadResolution(12); // padded to fit uint16_t

  // pull high for reset pin
  digitalWrite(Reset, HIGH);
  pinMode(Reset, OUTPUT);

  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  // while (!Serial);

  // set the LED as output
  pinMode(LED_BUILTIN, OUTPUT);

  connectToWifi();
  connectToPainlabControlPanel();
  Serial.println("setup completed");
}

void printNetworkInfo() {
  Serial.println("Board Information:");
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print your network's SSID:
  Serial.println();
  Serial.println("Network Information:");
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.println(rssi);
  Serial.println("---------------------------------------");
}

#define COPY_AND_ADVANCE(data) \
  {                                                       \
    memcpy(buffer + bufferPointer, reinterpret_cast<byte*>(&data), sizeof(data));   \
    bufferPointer += sizeof(data);\
  }

void readAndPushFrameIntoBuffer() {
  if (bufferPointer > maxbuffer) {
    Serial.print("Frame dropped, allowSend: ");
    Serial.println(allowSend);
    bufferPointer = 4;

    consecutiveDroppedFrames++;
    if (consecutiveDroppedFrames > MAXDROPPEDFRAMES) {
      Serial.print("Resetting");
      digitalWrite(Reset, LOW);
    }
  }
  unsigned long ts = micros();
  // Remember to change ADC clock to at least DIV64
  uint16_t a0_reading = analogRead(A0); // GSR
  uint16_t a2_reading = analogRead(A2); // Heart Rate
  uint16_t a7_reading = analogRead(A7); // Myoware muscle sensor

  COPY_AND_ADVANCE(ts);
  COPY_AND_ADVANCE(a0_reading);
  COPY_AND_ADVANCE(a2_reading);
  COPY_AND_ADVANCE(a7_reading);
}

void checkOK() {
  //Serial.println("reading reply");
  uint8_t simplifiedReply = client.read();
  //Serial.println(simplifiedReply);
  if (simplifiedReply == 1) {
    allowSend = true;
  }
  else if (simplifiedReply != 255) {
    Serial.println(simplifiedReply);
  }
}

void loop() {
 if (!allowSend) {
    checkOK();
 }
 
 unsigned long currentMicrosInfo = micros();

 // check if the time after the last update is bigger the interval
 if (currentMicrosInfo - previousMicrosInfo >= collectInterval) {
   previousMicrosInfo = currentMicrosInfo;
   readAndPushFrameIntoBuffer();
 }
 
 //Serial.println(currentMicrosInfo - previousMicrosReport);
 if (allowSend && bufferPointer > 4) {
    if (currentMicrosInfo - previousMicrosReport >= reportInterval) {
      previousMicrosReport = currentMicrosInfo;

      *(reinterpret_cast<uint32_t*>(buffer)) = reverseWordEndianness(bufferPointer - 4); // write four BE bytes to buffer header
      
      client.write(buffer, bufferPointer); // ~3.5ms
      consecutiveDroppedFrames = 0;
      bufferPointer = 4;
      allowSend = false;

      // if the LED is off turn it on and vice-versa:
      if (ledState == LOW) {
        ledState = HIGH;
      } else {
        ledState = LOW;
      }

      // set the LED with the ledState of the variable:
      digitalWrite(LED_BUILTIN, ledState);
    }
 }

}
