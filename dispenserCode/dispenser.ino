// https://projecthub.arduino.cc/Isaac100/getting-started-with-the-hc-sr04-ultrasonic-sensor-7cabe1

#include <ESP32Servo.h>

#define trigPin 5
#define echoPin 17

#define speedOfSound 0.034

#define averageDivisionValue 10 //the sum of the distance value will be divived by this constant to get the average distance

float distance;
long duration;
int averageDistance = 0;
int servoPosition = 0;
Servo servoValve;

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  servoValve.attach(19); 
  
  Serial.begin(9600);
}

void loop() {

  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  duration = pulseIn(echoPin, HIGH);
  distance = (float)duration * speedOfSound/2;

  Serial.print("Distance (cm): ");
  Serial.println(distance);
  averageDistance = 0;

  for (servoPosition = 0; servoPosition <= 180; servoPosition += 1) {
    servoValve.write(servoPosition);
    delay(15);
  }

  for (servoPosition = 180; servoPosition >= 0; servoPosition -= 1) {
    servoValve.write(servoPosition);
    delay(15);

  }
}