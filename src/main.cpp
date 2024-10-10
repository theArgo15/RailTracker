/*
  Complete project details: https://RandomNerdTutorials.com/esp32-https-requests/
  Based on the BasicHTTPSClient.ino example found at Examples > BasicHttpsClient
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include "TLC5947.h"
#include "time.h"

// Replace with your network credentials
const char *ssid = "Snozzberries";
const char *password = "pinkpotato003";

const char *ntpServer = "pool.ntp.org";

// TLC5947 stuff
int DEVICES = 1; //  set Amount of TLC5947 Boards

int led = 24 * DEVICES; //  this set Amount of LEDs per Board

int wait = 100; //  set time between channel control

const int DATA = 26;
const int CLOCK = 25;
const int BLANK = 33;
const int LATCH = 32;

TLC5947 tlc(DEVICES, CLOCK, DATA, LATCH, BLANK);

const int ZERO_B = 0;   //  zero brightness
const int MAX_B = 4095; //  100% brightness
int a = MAX_B;          //  current brightness

// structure of stop location important variables
struct stopLocationStruct
{
  float lat;
  float lon;
  String name;
};

// initialize an array of all blue and green line stops
stopLocationStruct stopLocations[] = {
    {44.854277, -93.238877, "Mall of America Station"},
    {44.855876, -93.231499, "30th Ave Station"},
    {44.856369, -93.226485, "Bloomington Central Station"},
    {44.859536, -93.223109, "American Blvd Station"},
    {44.874119, -93.224068, "Terminal 2 Station"},
    {44.88077, -93.204922, "Terminal 1 Station"},
    {44.893222, -93.198084, "Fort Snelling Station"},
    {44.90279, -93.202266, "VA Medical Center Station"},
    {44.912429, -93.210163, "50th St Minnehaha Station"},
    {44.920758, -93.219847, "46th St Station"},
    {44.934655, -93.229449, "38th St Station"},
    {44.94836, -93.238864, "Lake St - Midtown Station"},
    {44.962525, -93.247027, "Franklin Ave Station"},
    {44.968406, -93.251029, "Cedar Riverside Station"},
    {44.975101, -93.259722, "U.S. Bank Stadium Station"},
    {44.976863, -93.265879, "Government Plaza Station"},
    {44.978597, -93.269919, "Nicollet Mall Station"},
    {44.980177, -93.273202, "Warehouse District Hennepin Ave Station"},
    {44.983045, -93.277453, "Target Field Station Platform 1"},
    {44.983543, -93.278703, "Target Field Station Platform 2'"}};

unsigned long epochTime;
String jsonBuffer;
String serverName = "https://svc.metrotransit.org/nextrip/vehicles";
String routeIDs[] = {
    "901",
    "902",
};
int stopCount = 6;
// www.howsmyssl.com root certificate authority, to verify the server
// change it to your server root CA
const char *rootCACertificate = "  -----BEGIN CERTIFICATE-----\n"
                                "MIIF3jCCA8agAwIBAgIQAf1tMPyjylGoG7xkDjUDLTANBgkqhkiG9w0BAQwFADCB\n"
                                "iDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0pl\n"
                                "cnNleSBDaXR5MR4wHAYDVQQKExVUaGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNV\n"
                                "BAMTJVVTRVJUcnVzdCBSU0EgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMTAw\n"
                                "MjAxMDAwMDAwWhcNMzgwMTE4MjM1OTU5WjCBiDELMAkGA1UEBhMCVVMxEzARBgNV\n"
                                "BAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0plcnNleSBDaXR5MR4wHAYDVQQKExVU\n"
                                "aGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNVBAMTJVVTRVJUcnVzdCBSU0EgQ2Vy\n"
                                "dGlmaWNhdGlvbiBBdXRob3JpdHkwggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIK\n"
                                "AoICAQCAEmUXNg7D2wiz0KxXDXbtzSfTTK1Qg2HiqiBNCS1kCdzOiZ/MPans9s/B\n"
                                "3PHTsdZ7NygRK0faOca8Ohm0X6a9fZ2jY0K2dvKpOyuR+OJv0OwWIJAJPuLodMkY\n"
                                "tJHUYmTbf6MG8YgYapAiPLz+E/CHFHv25B+O1ORRxhFnRghRy4YUVD+8M/5+bJz/\n"
                                "Fp0YvVGONaanZshyZ9shZrHUm3gDwFA66Mzw3LyeTP6vBZY1H1dat//O+T23LLb2\n"
                                "VN3I5xI6Ta5MirdcmrS3ID3KfyI0rn47aGYBROcBTkZTmzNg95S+UzeQc0PzMsNT\n"
                                "79uq/nROacdrjGCT3sTHDN/hMq7MkztReJVni+49Vv4M0GkPGw/zJSZrM233bkf6\n"
                                "c0Plfg6lZrEpfDKEY1WJxA3Bk1QwGROs0303p+tdOmw1XNtB1xLaqUkL39iAigmT\n"
                                "Yo61Zs8liM2EuLE/pDkP2QKe6xJMlXzzawWpXhaDzLhn4ugTncxbgtNMs+1b/97l\n"
                                "c6wjOy0AvzVVdAlJ2ElYGn+SNuZRkg7zJn0cTRe8yexDJtC/QV9AqURE9JnnV4ee\n"
                                "UB9XVKg+/XRjL7FQZQnmWEIuQxpMtPAlR1n6BB6T1CZGSlCBst6+eLf8ZxXhyVeE\n"
                                "Hg9j1uliutZfVS7qXMYoCAQlObgOK6nyTJccBz8NUvXt7y+CDwIDAQABo0IwQDAd\n"
                                "BgNVHQ4EFgQUU3m/WqorSs9UgOHYm8Cd8rIDZsswDgYDVR0PAQH/BAQDAgEGMA8G\n"
                                "A1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQEMBQADggIBAFzUfA3P9wF9QZllDHPF\n"
                                "Up/L+M+ZBn8b2kMVn54CVVeWFPFSPCeHlCjtHzoBN6J2/FNQwISbxmtOuowhT6KO\n"
                                "VWKR82kV2LyI48SqC/3vqOlLVSoGIG1VeCkZ7l8wXEskEVX/JJpuXior7gtNn3/3\n"
                                "ATiUFJVDBwn7YKnuHKsSjKCaXqeYalltiz8I+8jRRa8YFWSQEg9zKC7F4iRO/Fjs\n"
                                "8PRF/iKz6y+O0tlFYQXBl2+odnKPi4w2r78NBc5xjeambx9spnFixdjQg3IM8WcR\n"
                                "iQycE0xyNN+81XHfqnHd4blsjDwSXWXavVcStkNr/+XeTWYRUc+ZruwXtuhxkYze\n"
                                "Sf7dNXGiFSeUHM9h4ya7b6NnJSFd5t0dCy5oGzuCr+yDZ4XUmFF0sbmZgIn/f3gZ\n"
                                "XHlKYC6SQK5MNyosycdiyA5d9zZbyuAlJQG03RoHnHcAP9Dc1ew91Pq7P8yF1m9/\n"
                                "qS3fuQL39ZeatTXaw2ewh0qpKJ4jjv9cJ2vhsE/zB+4ALtRZh8tSQZXq9EfX7mRB\n"
                                "VXyNWQKV3WKdwrnuWih0hKWbt5DHDAff9Yk2dDLWKMGwsAvgnEzDHNb842m1R0aB\n"
                                "L6KCq9NjRHDEjf8tM7qtj3u1cIiuPhnPQCjY/MiQu12ZIvVS5ljFH4gxQ+6IHdfG\n"
                                "jjxDah2nGN59PRbxYvnKkKj9\n"
                                "-----END CERTIFICATE-----\n";

void setup()
{
  Serial.begin(115200);
  Serial.println();
  // Initialize Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());

  tlc.begin(); //  initialize TLC5947 library
  tlc.enable();
  for (int i = 0; i < 2; i++)
  {
    for (int i = 0; i < led; i++)
    {
      tlc.setPWM(i, a);
      delay(wait);
      tlc.write();
      Serial.print(i);
      Serial.print(" = ");
      Serial.println(a);
    }
    a = (a > ZERO_B) ? ZERO_B : MAX_B;
  }
}

// function for API calls
String apiCall(const char *serverName)
{
  String payload;
  WiFiClientSecure *client = new WiFiClientSecure;
  if (client)
  {
    // set secure client with certificate
    client->setCACert(rootCACertificate);
    // create an HTTPClient instance
    HTTPClient https;

    // Initializing an HTTPS communication using the secure client
    Serial.print("[HTTPS] begin...\n");
    if (https.begin(*client, serverName))
    { // HTTPS
      Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header
      int httpCode = https.GET();
      // httpCode will be negative on error
      if (httpCode > 0)
      {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
        {
          // print server response payload
          payload = https.getString();
          https.end();
          return payload;
        }
        else
        {
          Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }
        https.end();
      }
    }
    else
    {
      Serial.printf("[HTTPS] Unable to connect\n");
    }
  }
}

// function to get current time from NTP server
unsigned long getTime()
{
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return (0);
  }
  time(&now);
  return now;
}

// function to do the distance formula between two coordinates and convert the answer into feet
float coordDistance(float lat1, float lon1, float lat2, float lon2)
{
  const float meterPerDegree = 111195;
  // cosine of the latitude in the interested area. Used to determine how many meters per degree of longitude based on the latitude (shorter by the poles).
  // Since the latitudes are similar, we will treat this as a constant. That means the code only works in Minneapolis!
  const float coslat = .70810068;
  float dlat = lat1 - lat2;
  float dlon = (lon1 - lon2) * coslat;
  return sqrt(dlat * dlat + dlon * dlon) * meterPerDegree;
}

void loop()
{
  epochTime = getTime();
  Serial.println(epochTime);

  Serial.println(coordDistance(44.854277, -93.238877, 44.983543, -93.278703));
  // for (int thisStop = 0; thisStop < stopCount; thisStop++)
  // {
  //   String extendedServerName = (serverName + stopIDs[thisStop]);
  //   jsonBuffer = apiCall(extendedServerName.c_str());
  //   JSONVar myObject = JSON.parse(jsonBuffer);
  //   // JSON.typeof(jsonVar) can be used to get the type of the var
  //   if (JSON.typeof(myObject) == "undefined")
  //   {
  //     Serial.println("Parsing input failed!");
  //     return;
  //   }
  //   delay(1000);
  //   Serial.print("Departures: ");
  //   Serial.println(myObject["departures"][0]);
  //   int departureTime = int(myObject["departures"][0]["departure_time"]);
  //   Serial.print("epochTime is :");
  //   Serial.println(epochTime);
  //   Serial.print("relative time is:");
  //   Serial.println(departureTime - epochTime);
  // if (departureTime - epochTime < 300)
  // {
  //   analogWrite(pins[thisStop], 255);
  // }
  // else
  // {
  //   analogWrite(pins[thisStop], 0);
  // }
  // }
  //     Serial.print("JSON object = ");
  // Serial.println(myObject);
  // Serial.print("Departures: ");
  // Serial.println(myObject["departures"]);

  Serial.println();
  Serial.println("Waiting 1min before the next round...");
  delay(60000);
}