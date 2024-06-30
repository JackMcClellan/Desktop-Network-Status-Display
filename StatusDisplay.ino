/*
 Desktop Network Status Display

 This sketch connects to Uptime Kuma https://github.com/louislam/uptime-kuma
 using an Arduino Mega with an ethernet Module and WS2811 Pixels
 to show the current status of selected monitors with visual Pixel Colors.

 Circuit:
 * Ethernet Module attached to pins 10, 50, 51, 52
 * LED Pixel Strip attached to pins 3

 created 28 Mar 2024
 by Jack McClellan
 modified 30 June 2024
 by Jack McClellan

 */

#include <SPI.h>
#include <Ethernet.h>
#include "FastLED.h"

// Define Arduino Ethernet Values
IPAddress ip(192, 168, 1, 99); // (Static) IP address of the Arduino

// Define Uptime Server Values
IPAddress serverIP(192, 168, 1, 12);                        // (Static) IP address of Uptime Server
const int serverPort = 3001;                                // Uptime Server Port
const String monitorNumbers[] = {"4", "6", "1", "2", "10"}; // The monitor numbers found from Uptime Server. Ex. monitorNumbers[0] = "4"
const int refreshRate = 30;                                 // Approx. Time(sec) between refreshing status from Uptime Server

// Define LED Values
#define DATA_PIN 3            // The data pin of the LED Pixels
#define NUM_LEDS 5            // Number of LEDs on the current strand
const float brightness = .05; // Percentage for brightness of all LEDs (0-1)

// ************************************************************************
const byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED}; // MAC address of the ethernet card
CRGB leds[NUM_LEDS];                                     // LED Array for use in FastLED
EthernetClient client;
const int monitorCount = sizeof(monitorNumbers) / sizeof(monitorNumbers[0]);

void setup()
{
  // start the Ethernet connection:
  Ethernet.begin(mac, ip);
  // give the Ethernet shield time to initialize:
  delay(1000);

  // Initialize LEDs
  FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS);

  // Initialize Serial Monitor
  Serial.begin(9600);
  Serial.println("Beginning Monitoring on **" + String(monitorCount) + "** items approx. every " + String(refreshRate) + " seconds.");

  // Initialize LED test to insure all LEDs are working
  init_led_test();
}

/**
 * Runs only at board boot to make sure pixels are working and turns all LEDs off.
 */
void init_led_test()
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB(200, 200, 200); // set pixel white
    FastLED.show();
    delay(500);
  }
  delay(1000);

  // Blink all LEDs white for 2 seconds
  for (int i = 0; i < 2; i++)
  {
    LEDS.showColor(CRGB(0, 0, 0));
    delay(200);
    LEDS.showColor(CRGB(255 * brightness, 255 * brightness, 255 * brightness));
    delay(200);
  }

  // Turn all LEDs off
  LEDS.showColor(CRGB(0, 0, 0));
  delay(500);
}

/**
 * Sends a GET request to the server to retrieve the status of a specific monitor.
 *
 * @param monitorNumber the number of the monitor to retrieve the status for
 * @return the response from the server as a string, or an empty string if the connection failed
 */
String get_response_from_monitor(String monitorNumber)
{
  if (client.connect(serverIP, serverPort))
  {
    client.println("GET /api/badge/" + monitorNumber + "/status HTTP/1.1");
    client.println("Host: " + String(serverIP[0]) + "." + String(serverIP[1]) + "." + String(serverIP[2]) + "." + String(serverIP[3]));
    client.println("Connection: close");
    client.println();
  }
  else
  {
    // if you couldn't make a connection:
    Serial.println("Connection failed");
    return "";
  }

  // read the response from the server:
  char c;
  String response = "";
  while (client.connected() && !client.available())
    delay(1); // wait for data
  while (client.connected() || client.available())
  {
    if (client.available())
    {
      c = client.read();
      response += c;
    }
  }
  client.stop();

  return response;
}

/**
 * Finds the status from the given response string.
 *
 * @param response the response string containing status information
 * @return an integer representing the status found: 0 for "up", 1 for "down", 2 for "maintenance", -1 if status not found
 */
int find_status_from_response(String response)
{
  // Example Response: ... <title>Status: Up</title> ...
  int startTitleIdx = response.indexOf("<title>") + 7;
  int endTitleIdx = response.indexOf("</title>");
  if (startTitleIdx != -1 && endTitleIdx != -1)
  {
    String titleString = response.substring(startTitleIdx, endTitleIdx);
    int colonIdx = titleString.indexOf(":");
    if (colonIdx != -1)
    {
      String status = titleString.substring(colonIdx + 2); // colon and space
      status.toLowerCase();
      if (status == "up")
      {
        return 0;
      }
      else if (status == "down")
      {
        return 1;
      }
      else if (status == "maintenance")
      {
        return 2;
      }
    }
  }
  return -1; // return -1 if status not found
}

/**
 * Displays the status of each monitor based on the status array.
 *
 * @param statusArray an array of integers representing the status of each monitor
 */
void display_status(int statusArray[])
{
  // Set all lights to default off (No Color)
  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB(0, 0, 0); // set pixel off
  }

  for (int i = 0; i < monitorCount; i++)
  {
    const int currentStatus = statusArray[i];
    switch (currentStatus)
    {
    case 0:
      leds[i] = CRGB(0, 255 * brightness, 0); // set pixel green
      break;

    case 1:
      leds[i] = CRGB(255 * brightness, 0, 0); // set pixel red
      break;

    case 2:
      leds[i] = CRGB(0, 0, 255 * brightness); // set pixel blue
      break;

    default:
      Serial.println("Invalid expression for monitor number " + i);
      leds[i] = CRGB(255 * brightness, 225 * brightness, 0); // set pixel yellow
      break;
    }
  }
  FastLED.show();
}

void loop()
{
  int statusArray[monitorCount];
  Serial.println("Getting Status!\n");
  for (int i = 0; i < monitorCount; i++)
  {
    String currentMonitorNumber = monitorNumbers[i];
    String status = "";

    // Get Status for current monitor from Uptime Server
    String serverResponse = get_response_from_monitor(currentMonitorNumber);

    // Convert response from Server into status
    statusArray[i] = find_status_from_response(serverResponse);
  }
  // Update LED status colors
  display_status(statusArray);

  // Wait before sending next refresh requests
  delay(1000 * refreshRate);
}