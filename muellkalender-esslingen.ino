#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>  // Include this library for secure connections

// Wi-Fi credentials
const char* ssid = "SSID";
const char* password = "KEY";

// NTP Client setup
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600 * 1); // Berlin timezone (UTC+1)

// OLED display dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C // I2C address of the OLED display
#define OLED_SDA 14         // D6
#define OLED_SCL 12         // D5

Adafruit_SSD1306 *display;

// Link to ics calendar file. Get your link here https://www.awb-es.de/abfuhr/abfuhrtermine/abfuhrtermine-suchen.html
const char* apiUrl = "https://api.abfall.io/?kh=DaA02103019b46345f1998698563DaAd&t=ics&s=3701";

// Set DNS server to Google's DNS (8.8.8.8)
IPAddress dns1(8, 8, 8, 8);

// LED Pin (define an Alarm LED)
#define LED_PIN 4  // D2 pin on ESP8266 (GPIO4)

void setup() {
  // Initialize Serial
  Serial.begin(1000000); // Set baud rate to 1000000 (Ensure Serial Monitor matches)

  // Set DNS server
  WiFi.config(INADDR_NONE, INADDR_NONE, dns1); // Manually set DNS

  // Initialize OLED
  display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display->begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display->clearDisplay();
  display->setTextSize(1);
  display->setTextColor(SSD1306_WHITE);
  display->setCursor(0, 0);
  display->println("Starting...");
  display->display();

  // Connect to Wi-Fi
  display->println("Connecting to WiFi...");
  display->display();
  WiFi.begin(ssid, password);
  
  // Wait for Wi-Fi connection
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    attempts++;
    Serial.print(".");
    if (attempts >= 20) {
      Serial.println("WiFi connection failed");
      display->println("WiFi failed");
      display->display();
      return; // Stop setup if Wi-Fi doesn't connect
    }
  }
  Serial.println("WiFi connected");
  display->println("WiFi connected");
  display->display();

  // Initialize time client
  timeClient.begin();

  // Set LED pin as output
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  // Update time client
  timeClient.update();

  // Get current date and time
  String currentDate = getCurrentDate();  // In YYYYMMDD format
  String tomorrowDate = getTomorrowDate();  // In YYYYMMDD format

  // Fetch and display events
  String event = fetchEvents(currentDate, tomorrowDate);

  // Display date and event
  display->clearDisplay();
  display->setCursor(0, 0);
  display->println(getFormattedDate());  // Display date in dd.mm.yyyy hh:mm format
  display->println("");  // Empty line under the date

  if (event != "") {
    display->println(event);
    digitalWrite(LED_PIN, HIGH);  // Turn on LED if there are events
  } else {
    display->println("No events today.");
    digitalWrite(LED_PIN, LOW);  // Turn off LED if no events
  }

  display->display();
  delay(60000); // Update every minute
}

String getCurrentDate() {
  time_t rawTime = timeClient.getEpochTime();
  struct tm* timeInfo = gmtime(&rawTime);

  char dateBuffer[9];  // Length for "YYYYMMDD"
  strftime(dateBuffer, sizeof(dateBuffer), "%Y%m%d", timeInfo);  // Format as YYYYMMDD
  return String(dateBuffer);
}

String getTomorrowDate() {
  time_t rawTime = timeClient.getEpochTime() + 86400; // Add 1 day
  struct tm* timeInfo = gmtime(&rawTime);
  char dateBuffer[9];  // Length for "YYYYMMDD"
  strftime(dateBuffer, sizeof(dateBuffer), "%Y%m%d", timeInfo);  // Format as YYYYMMDD
  return String(dateBuffer);
}

String getFormattedDate() {
  time_t rawTime = timeClient.getEpochTime();
  struct tm* timeInfo = gmtime(&rawTime);

  char dateBuffer[11];  // Length for "dd.mm.yyyy"
  char timeBuffer[6];   // Length for "hh:mm"

  // Format date as dd.mm.yyyy
  strftime(dateBuffer, sizeof(dateBuffer), "%d.%m.%Y", timeInfo);

  // Format time as hh:mm
  strftime(timeBuffer, sizeof(timeBuffer), "%H:%M", timeInfo);

  // Combine date and time in the desired format
  String formattedDate = String(dateBuffer) + " " + String(timeBuffer);
  return formattedDate;
}

String fetchEvents(String currentDate, String tomorrowDate) {
  if (WiFi.status() != WL_CONNECTED) {
    return "No WiFi";
  }

  // DNS Resolution Check
  IPAddress ip;
  if (!WiFi.hostByName("api.abfall.io", ip)) {
    Serial.println("DNS resolution failed");
    return "DNS resolution failed";
  } else {
    Serial.println("DNS resolution succeeded");
    Serial.println(ip);
  }

  Serial.println("Starting HTTP request...");

  WiFiClientSecure client;  // Use secure client for HTTPS
  HTTPClient http;
  http.begin(client, apiUrl);  // Begin HTTP request to the API using HTTPS
  http.setUserAgent("Mozilla/5.0 (X11; Linux x86_64; rv:128.0) Gecko/20100101 Firefox/128.0");

  // Disabling SSL certificate validation for testing
  client.setInsecure();  // This disables certificate validation for testing

  int httpCode = http.GET(); // Perform the GET request

  // Check HTTP response code
  if (httpCode != HTTP_CODE_OK) {
    Serial.print("HTTP Error: ");
    Serial.println(httpCode);
    http.end();
    return "Error fetching events";
  }

  // Print the raw response for debugging
  String payload = http.getString();
  Serial.println("Raw API Response: ");
  Serial.println(payload);
  
  http.end();

  // Parse the iCalendar .ics format manually
  return parseICS(payload, currentDate, tomorrowDate);
}

String parseICS(String icsData, String currentDate, String tomorrowDate) {
  String eventSummary = "";

  // Look for all BEGIN:VEVENT and END:VEVENT blocks in the .ics data
  int eventStart = 0;
  while ((eventStart = icsData.indexOf("BEGIN:VEVENT", eventStart)) != -1) {
    int eventEnd = icsData.indexOf("END:VEVENT", eventStart);
    if (eventEnd == -1) break; // Error if there's no END:VEVENT

    String event = icsData.substring(eventStart, eventEnd);
    String eventDate = extractDate(event);

    // Extract the SUMMARY field of the event
    String summary = extractSummary(event);

    // Convert special characters
    summary = convertSpecialChars(summary);

    // Check if event date matches today or tomorrow
    if (eventDate == currentDate || eventDate == tomorrowDate) {
      if (eventSummary != "") {
        eventSummary += "\n";
      }
      eventSummary += summary;
    }

    // Move to next event
    eventStart = eventEnd;
  }

  return eventSummary.isEmpty() ? "No events today." : eventSummary;
}

String extractDate(String event) {
  int startIndex = event.indexOf("DTSTART;VALUE=DATE:") + strlen("DTSTART;VALUE=DATE:");
  int endIndex = event.indexOf("\r\n", startIndex);
  return event.substring(startIndex, endIndex);
}

String extractSummary(String event) {
  int startIndex = event.indexOf("SUMMARY:") + strlen("SUMMARY:");
  int endIndex = event.indexOf("\r\n", startIndex);
  return event.substring(startIndex, endIndex);
}

// Function to replace special characters with ASCII equivalents
String convertSpecialChars(String text) {
  text.replace("ü", "ue");
  text.replace("ö", "oe");
  text.replace("ä", "ae");
  text.replace("ß", "ss");
  text.replace("é", "e");
  text.replace("è", "e");
  text.replace("à", "a");
  // Add more replacements as needed
  return text;
}
