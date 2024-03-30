/*
 This sketch connects to Uptime Kuma https://github.com/louislam/uptime-kuma
 using an Arduino Mega with ethernet and Pixel strip
 to show the current status of monitors from Uptime Kuma with visual Pixel Colors

 Circuit:
 * Ethernet Module attached to pins 10, 50, 51, 52
 * LED Pixel Strip attached to pins 3

 created 28 Mar 2024
 by Jack McClellan
 modified 30 Mar 2024
 by Jack McClellan

 */

#include <SPI.h>
#include <Ethernet.h>
#include "FastLED.h"

// Enter a MAC address and IP address for your controller below.
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 99);  // IP address of the Arduino

//Define the monitors to check status of
IPAddress serverIP(192, 168, 1, 12);  // IP address of Uptime Kuma

String monitorNumbers[] = {"6", "7", "8", "12"};
int monitorCount = sizeof(monitorNumbers) / sizeof(monitorNumbers[0]);

// Define LED Data
#define DATA_PIN 3 // for LED strand
#define NUM_LEDS 5
CRGB leds[NUM_LEDS];


// ******************************************************** //
EthernetClient client;

void setup() {
  // start the Ethernet connection:
  Ethernet.begin(mac, ip);
  // give the Ethernet shield a second to initialize:
  delay(1000);
  // Initialize LEDs 
  FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS);
  
  Serial.begin(9600);
  Serial.println("Begining Monitoring");
  Serial.println(monitorCount);

  // Initialize LED test to insure all LEDs are working
  init_led_test();
}

String get_status_response(String monitorNumber){
  if (client.connect(serverIP, 3001)) {
    client.println("GET /api/badge/" + monitorNumber + "/status HTTP/1.1");
    client.println("Host: " + String(serverIP[0]) + "." + String(serverIP[1]) + "." + String(serverIP[2]) + "." + String(serverIP[3]));
    client.println("Connection: close");
    client.println();
  } else {
    // if you couldn't make a connection:
    Serial.println("Connection failed **" + monitorNumber + "**");
    return "";
  }

  // read the response from the server:
  char c;
  String fullResponse = "";
  while (client.connected() && !client.available()) delay(1);  // wait for data
  while (client.connected() || client.available()) {
    if (client.available()) {
      c = client.read();
      fullResponse += c;
    }
  }
  client.stop();

  return fullResponse;
}

String get_status(String response){
  int startIdx = response.indexOf("<title>") + 7;
  int endIdx = response.indexOf("</title>");
  if (startIdx != -1 && endIdx != -1) {
    String statusFull = response.substring(startIdx, endIdx);
    int colonIndex = statusFull.indexOf(":");
    if (colonIndex != -1) {
      String status = statusFull.substring(colonIndex + 2);
      return status;
    }
  }
  return "";  // return empty string if status extraction fails
}

void init_led_test() //runs at board boot to make sure pixels are working
{
  LEDS.showColor(CRGB(255, 0, 0)); //turn all pixels on red
   delay(1000);
   LEDS.showColor(CRGB(0, 255, 0)); //turn all pixels on green
   delay(1000);
   LEDS.showColor(CRGB(0, 0, 255)); //turn all pixels on blue
   delay(1000);
   LEDS.showColor(CRGB(0, 0, 0)); //turn all pixels off
}

void show_status_led(String statusArray[], int statusArrayCount) {
  for (int i = 0; i < statusArrayCount; i++) {
    const char* currentStatus = statusArray[i].c_str();
    if (strcmp(currentStatus, "Up") == 0) {
      leds[i] = CRGB(0, 255, 0); //turn current pixel green
    } else if (strcmp(currentStatus, "Down") == 0) {
      leds[i] = CRGB(255, 0, 0); //turn current pixel red
    } else if (strcmp(currentStatus, "Maintenance") == 0) {
      leds[i] = CRGB(0, 0, 255); //turn current pixel blue
    } else if (strcmp(currentStatus, "Failed") == 0) {
      leds[i] = CRGB(255, 255, 0); //turn current pixel yellow
    } else {
      leds[i] = CRGB(255, 255, 255); //turn current pixel white
      Serial.println("Invalid status");
    }
  }
  FastLED.show();
}

void loop() {
  String statusArray[monitorCount];
  for (int i = 0; i < monitorCount; i++) { 
    String currentMonitorNumber = monitorNumbers[i];
    String status = "";
    
    // Get Status for current monitor from Uptime Server
    String serverResponse = get_status_response(currentMonitorNumber);
    
    if (serverResponse != ""){
      // Convert response from Server into status
      status = get_status(serverResponse);
      if (status == ""){
        status = "Failed";
        }
    }
    else {
      status = "Failed";
    }
    
    Serial.println("**" + currentMonitorNumber + "** " + status);
    statusArray[i] = status;
  }
  // Update LED status colors
  show_status_led(statusArray, sizeof(statusArray) / sizeof(statusArray[0]));
  
  // wait 15 seconds before sending the next request:
  Serial.println();
  delay(15000);
}
