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
#include "secrets.h"
#include <vector>

// Replace with your network credentials
const char *ssid = SECRET_SSID;
const char *password = SECRET_PASS;

const char *ntpServer = "pool.ntp.org";

// TLC5947 stuff
const int DEVICES = 2; //  set Amount of TLC5947 Boards

const int led = 24 * DEVICES; //  this set Amount of LEDs per Board

const int wait = 100; //  set time between channel control

const int DATA = 32;
const int CLOCK = 33;
const int BLANK = 25;
const int LATCH = 26;

TLC5947 tlc(DEVICES, CLOCK, DATA, LATCH, BLANK);

const int ZERO_B = 0;   //  zero brightness
const int MAX_B = 1000; //  100% brightness is 4095
const int MID_B = 250;  //  10% brightness
int a = MAX_B;          //  current brightness

const int indicatorLED = 42;

// structure of stop location important variables
struct stopLocationStruct
{
  float lat;
  float lon;
  String name;
  int LEDindex;
};

// initialize a vector of all blue line stops
std::vector<stopLocationStruct> blueLineStopLocations = {
    {44.854277, -93.238877, "Mall of America Station", 0},
    {44.855876, -93.231499, "30th Ave Station", 1},
    {44.856369, -93.226485, "Bloomington Central Station", 2},
    {44.859536, -93.223109, "American Blvd Station", 3},
    {44.874119, -93.224068, "Terminal 2 Station", 4},
    {44.88077, -93.204922, "Terminal 1 Station", 5},
    {44.893222, -93.198084, "Fort Snelling Station", 6},
    {44.90279, -93.202266, "VA Medical Center Station", 7},
    {44.912429, -93.210163, "50th St Minnehaha Station", 8},
    {44.920758, -93.219847, "46th St Station", 9},
    {44.934655, -93.229449, "38th St Station", 10},
    {44.94836, -93.238864, "Lake St - Midtown Station", 11},
    {44.962525, -93.247027, "Franklin Ave Station", 12},
    {44.968406, -93.251029, "Cedar Riverside Station", 13},
    {44.975101, -93.259722, "U.S. Bank Stadium Station", 14},
    {44.976863, -93.265879, "Government Plaza Station", 15},
    {44.978597, -93.269919, "Nicollet Mall Station", 16},
    {44.980177, -93.273202, "Warehouse District Hennepin Ave Station", 17},
    {44.983045, -93.277453, "Target Field Station Platform 1", 18},
};
// initialize vector of all green line stops
std::vector<stopLocationStruct> greenLineStopLocations = {
    {44.948226, -93.08672, "Union Depot Station", 19},
    {44.946164, -93.092297, "Central Station", 20},
    {44.950663, -93.097492, "10th St Station", 21},
    {44.954047, -93.097459, "Robert St Station", 22},
    {44.955728, -93.105179, "Capitol / Rice St Station", 23},
    {44.955776, -93.117045, "Western Ave Station", 24},
    {44.955741, -93.127216, "Dale St Station", 25},
    {44.95575, -93.137323, "Victoria St Station", 26},
    {44.955739, -93.147502, "Lexington Pkwy Station", 27},
    {44.955718, -93.157728, "Hamline Ave Station", 28},
    {44.955696, -93.167894, "Snelling Ave Station", 29},
    {44.956409, -93.17872, "Fairview Ave Station", 30},
    {44.963091, -93.195441, "Raymond Ave Station", 31},
    {44.967838, -93.207226, "Westgate Station", 32},
    {44.971754, -93.215229, "Prospect Park Station", 33},
    {44.974799, -93.222881, "Stadium Village Station", 34},
    {44.973645, -93.231064, "East Bank Station", 35},
    {44.971941, -93.246128, "West Bank Station", 36},
    {44.97496, -93.259658, "U.S. Bank Stadium Station", 37},
    {44.976796, -93.265899, "Government Plaza Station", 38},
    {44.978541, -93.26996, "Nicollet Mall Station", 39},
    {44.980074, -93.273244, "Warehouse District Hennepin Ave Station", 40},
    {44.982905, -93.277396, "Target Field Station Platform 1", 41},
};

// vector of vectors?
std::vector<std::vector<stopLocationStruct>> allLineStopLocations = {blueLineStopLocations, greenLineStopLocations};

unsigned long epochTime;
String jsonBuffer;
String serverName = "https://svc.metrotransit.org/nextrip/vehicles/";
String routeIDs[] = {
    "901", // Blue line
    "902", // Green line
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
    delay(1000);
    a = (a > ZERO_B) ? ZERO_B : MAX_B;
  }
}

// function to light up an indicator LED
void raiseLED(int LEDpin)
{
  int brightness = 100;
  tlc.setPWM(LEDpin, brightness);
  delay(wait);
  tlc.write();
}
// function to blink an indicator LED
void blinkLED(int LEDpin)
{
  int brightness = 1000;
  for (int i = 0; i < 2; i++)
  {

    tlc.setPWM(LEDpin, brightness);
    delay(wait);
    tlc.write();
    brightness = ZERO_B;
  }
}
// function for API calls
String apiCall(const char *serverName)
{
  String payload;
  WiFiClientSecure client;
  {
    // set secure client with certificate
    client.setCACert(rootCACertificate);
    // create an HTTPClient instance
    HTTPClient https;

    // Initializing an HTTPS communication using the secure client
    Serial.print("[HTTPS] begin...\n");
    if (https.begin(client, serverName))
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
          blinkLED(indicatorLED);
          https.end();
          return payload;
        }
        else
        {
          Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
          raiseLED(indicatorLED);
        }
        https.end();
      }
    }
    else
    {
      Serial.printf("[HTTPS] Unable to connect\n");
      raiseLED(indicatorLED);
    }
  }
  return "";
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
  // return sqrt(dlat * dlat + dlon * dlon) * meterPerDegree; //use this line to get actual distances
  return (dlat * dlat + dlon * dlon) * meterPerDegree; // sqrt is computationally expensive, so return distance squared instead
}

// function to set LEDs to be off
void clearLEDS(int color) // 0 for blue line, 1 for green
{
  for (int i = 0; i < allLineStopLocations.at(color).size(); i++)
  {
    tlc.setPWM(allLineStopLocations.at(color).at(i).LEDindex, ZERO_B);
  }
}

void loop()
{

  // Serial.println(coordDistance(44.854277, -93.238877, 44.983543, -93.278703));
  // loop of all lines of interest
  for (int k = 0; k < 2; k++)
  {
    std::vector<stopLocationStruct> stopLocations = allLineStopLocations.at(k);
    String extendedServerName = (serverName + routeIDs[k]);
    jsonBuffer = apiCall(extendedServerName.c_str());
    JSONVar trainsJson = JSON.parse(jsonBuffer);
    // JSON.typeof(jsonVar) can be used to get the type of the var
    if (JSON.typeof(trainsJson) == "undefined")
    {
      Serial.println("Parsing input failed!");
      return;
    }
    clearLEDS(k);
    // loop for all trains on line
    for (int i = 0; i < trainsJson.length(); i++)
    {
      float minDistance = 100000000; // needs to be arbitarily high
      String nearestStop;
      int nearestStopLEDindex;
      String directionOfMotion;
      // cast the lat and long to float from the JSON
      float trainLat = (double)trainsJson[i]["latitude"];
      float trainLon = (double)trainsJson[i]["longitude"];
      // loop of all stops on line
      for (const stopLocationStruct &stopLocation : stopLocations)
      {
        directionOfMotion = JSON.stringify(trainsJson[i]["direction"]);

        float currentDistance = coordDistance(trainLat, trainLon, stopLocation.lat, stopLocation.lon);
        // Serial.println(currentDistance);
        if (currentDistance < minDistance)
        {
          minDistance = currentDistance;
          nearestStop = stopLocation.name;
          nearestStopLEDindex = stopLocation.LEDindex;
        }
      }
      // minDistance = sqrt(minDistance); //Uncomment if actual distance needed
      Serial.print("The ");
      Serial.print(trainsJson[i]["direction"]);
      Serial.print(" ");
      Serial.print(trainsJson[i]["trip_id"]);
      Serial.print(" train is ");
      Serial.print(minDistance);
      Serial.print(" meters away from ");
      Serial.println(nearestStop);
      Serial.print("Light up LED ");
      Serial.println(nearestStopLEDindex);
      if (directionOfMotion == "\"SB\"" || directionOfMotion == "\"EB\"")
      {
        Serial.println("Southbound or Eastbound");
        // Do not show trains going out of the city
        tlc.setPWM(nearestStopLEDindex, ZERO_B);
      }
      else
      {
        tlc.setPWM(nearestStopLEDindex, MAX_B);
      }
    }
    tlc.write();
  }
  Serial.println();
  Serial.println("Waiting 45 seconds before the next round...");
  delay(45000);
}