#include <WiFi.h>
#include <WebServer.h>
#include "time.h"

// Ultrasonic sensor
#define trigPin 4
#define echoPin 2

#define speedOfSound 0.034

float distance;
long duration;

// Servo motor
#define servoPin 15

#define frequency 50
#define resolution 16
#define minDuty 1638
#define maxDuty 8192

int dutyCycle;

// Time and Network settings
const char* ssid = "";
const char* password = "";
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;
struct tm timeinfo;

// Time when the valve will open
int feedingHours[4] = {8, 12, 16, 20};
int feedingMinutes[4] = {0, 0, 0, 0}; 

// these variable ensure that the valve will open only one time
int lastFedHour = -1;
int lastFedMinute = -1;

// Web server setup
WebServer server(80);

void setup() {
  Serial.begin(9600);
  
  // Set ultrasonic sensor pin
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Servo motor setting
  ledcAttach(servoPin, frequency, 16);

  // Connect to Wi-Fi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  // Getting time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Initialize web server routes
  server.on("/", handleRoot);
  server.on("/set-time", handleSetTime);
  server.begin();

  Serial.println("HTTP server started.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  distance = getDistance();
  
  // Handle web server requests
  server.handleClient();

  // Get current time
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }

  // Check if it's time to open the servo
  checkFeedingTime();
}

void openValve() {
  // set the servo to 0 degree
  dutyCycle = map(0, 0, 180, minDuty, maxDuty);
  ledcWrite(servoPin, dutyCycle);  
  delay(1000);
  
  // set the servo to 55 degree
  dutyCycle = map(55, 0, 180, minDuty, maxDuty);
  ledcWrite(servoPin, dutyCycle);
  delay(1000);

  // set the servo to 0 degree
  dutyCycle = map(0, 0, 180, minDuty, maxDuty);
  ledcWrite(servoPin, dutyCycle);  
  delay(1000);
}

void checkFeedingTime() {
  for (int i = 0; i < 4; i++) {
    // Check if it's feeding time and ensure the servo hasn't already been triggered for this minute
    if (timeinfo.tm_hour == feedingHours[i] && timeinfo.tm_min == feedingMinutes[i]) {
      if (lastFedHour != timeinfo.tm_hour || lastFedMinute != timeinfo.tm_min) {
        openValve();
        
        // Update the last feeding time to prevent re-opening
        lastFedHour = timeinfo.tm_hour;
        lastFedMinute = timeinfo.tm_min;
      }
    }
  }
}


float getDistance() {
  // clear the trig pin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = (float)duration * speedOfSound / 2;
  
  if (distance > 19) {
    distance = 19;
  } else if (distance < 0) {
    distance = 0;
  }

  return distance;
}

void handleRoot() {
  String html = "<html><body>";
  html += "<h1>Food dispenser</h1>";
  html += "<h3>Current feeding times (HH:MM):</h3>";
  
  for (int i = 0; i < 4; i++) {
    html += "<p>Feeding time " + String(i + 1) + "-> " + String(feedingHours[i]) + ":" + String(feedingMinutes[i]) + "</p>";
  }
  
  html += "<form action='/set-time' method='get'>";
  html += "<h3>Set Feeding Time (HH:MM):</h3>";
  
  for (int i = 0; i < 4; i++) {
    html += "<input type='number' name='hour" + String(i) + "' min='0' max='23' value='" + String(feedingHours[i]) + "'>";
    html += ":<input type='number' name='minute" + String(i) + "' min='0' max='59' value='" + String(feedingMinutes[i]) + "'><br><br>";
  }
  
  html += "<input type='submit' value='Set Feeding Times'>";
  html += "</form>";
  
  html += "<h2>Food Level: " + String(distance) + " cm</h2>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleSetTime() {
  for (int i = 0; i < 4; i++) {
    if (server.hasArg("hour" + String(i))) {
      feedingHours[i] = server.arg("hour" + String(i)).toInt();
    }
    if (server.hasArg("minute" + String(i))) {
      feedingMinutes[i] = server.arg("minute" + String(i)).toInt();
    }
  }
  server.sendHeader("Location", "/");
  server.send(303); // Redirect to home page
}
