/*
*  By - KUMARAN K
**/
#include <SoftwareSerial.h>
#include <LiquidCrystal.h>

SoftwareSerial gsmSerial(2, 3); // RX on pin 2, TX on pin 3
SoftwareSerial gpsSerial(10, 11);

LiquidCrystal lcd(4, 5, 6, 7, 8, 9);

#define xPin A0
#define yPin A1
#define zPin A2

int xSample = 0;
int ySample = 0;
int zSample = 0;

#define numSamples 10
#define minAcceleration -50
#define maxAcceleration 50

int sampleIndex = 0;
int gpsStatus = 0;

float latitude = 0;
float longitude = 0;
String speed = "";
String gpsString = "";
char *gpsTestString = "$GPRMC";

void initializeModule(String command, char *response, int delayTime) {
  while (1) {
    Serial.println(command);
    gsmSerial.println(command);
    delay(100);

    while (gsmSerial.available() > 0) {
      if (gsmSerial.find(response)) {
        Serial.println(response);
        delay(delayTime);
        return;
      } else {
        Serial.println("Error");
      }
    }
    delay(delayTime);
  }
}

void setup() {
  gsmSerial.begin(9600);
  Serial.begin(9600);
  lcd.begin(16, 2);

  lcd.print("Accident Alert  ");
  lcd.setCursor(0, 1);
  lcd.print("     System     ");
  delay(2000);
  lcd.clear();
  lcd.print("Initializing");
  lcd.setCursor(0, 1);
  lcd.print("Please Wait...");
  delay(1000);

  Serial.println("Initializing....");
  initializeModule("AT", "OK", 1000);
  initializeModule("ATE1", "OK", 1000);
  initializeModule("AT+CPIN?", "READY", 1000);
  initializeModule("AT+CMGF=1", "OK", 1000);
  initializeModule("AT+CNMI=2,2,0,0,0", "OK", 1000);
  Serial.println("Initialized Successfully");
  lcd.clear();
  lcd.print("Initialized");
  lcd.setCursor(0, 1);
  lcd.print("Successfully");
  delay(2000);
  lcd.clear();
  lcd.print("Calibrating ");
  lcd.setCursor(0, 1);
  lcd.print("Accelerometer");

  for (int i = 0; i < numSamples; i++) {
    xSample += analogRead(xPin);
    ySample += analogRead(yPin);
    zSample += analogRead(zPin);
  }

  xSample /= numSamples;
  ySample /= numSamples;
  zSample /= numSamples;

  Serial.println(xSample);
  Serial.println(ySample);
  Serial.println(zSample);
  delay(1000);

  lcd.clear();
  lcd.print("Waiting For GPS");
  lcd.setCursor(0, 1);
  lcd.print("     Signal    ");
  delay(2000);
  gpsSerial.begin(9600);
  getGpsData();
  displayCoordinates();
  delay(2000);
  lcd.clear();
  lcd.print("GPS is Ready");
  delay(1000);
  lcd.clear();
  lcd.print("System Ready");
  Serial.println("System Ready..");
}

void loop() {
  int xValue = analogRead(xPin);
  int yValue = analogRead(yPin);
  int zValue = analogRead(zPin);

  int xAcceleration = xSample - xValue;
  int yAcceleration = ySample - yValue;
  int zAcceleration = zSample - zValue;

  Serial.print("x=");
  Serial.println(xAcceleration);
  Serial.print("y=");
  Serial.println(yAcceleration);
  Serial.print("z=");
  Serial.println(zAcceleration);

  if (xAcceleration < minAcceleration || xAcceleration > maxAcceleration ||
      yAcceleration < minAcceleration || yAcceleration > maxAcceleration ||
      zAcceleration < minAcceleration || zAcceleration > maxAcceleration) {
    getGpsData();
    displayCoordinates();
    lcd.clear();
    lcd.print("Sending SMS ");
    Serial.println("Sending SMS");
    sendSms();
    Serial.println("SMS Sent");
    delay(2000);
    lcd.clear();
    lcd.print("System Ready");
  }
}

void gpsEvent() {
  gpsString = "";

  while (1) {
    while (gpsSerial.available() > 0) {
      char inChar = (char)gpsSerial.read();
      gpsString += inChar;
      sampleIndex++;

      if (sampleIndex < 7) {
        if (gpsString[sampleIndex - 1] != gpsTestString[sampleIndex - 1]) {
          sampleIndex = 0;
          gpsString = "";
        }
      }

      if (inChar == '\r') {
        if (sampleIndex > 60) {
          gpsStatus = 1;
          break;
        } else {
          sampleIndex = 0;
        }
      }
    }

    if (gpsStatus)
      break;
  }
}

void getGpsData() {
  lcd.clear();
  lcd.print("Getting GPS Data");
  lcd.setCursor(0, 1);
  lcd.print("Please Wait.....");
  gpsStatus = 0;
  int x = 0;

  while (gpsStatus == 0) {
    gpsEvent();
    int strLength = sampleIndex;
    convertCoordinateToDecimal();
    sampleIndex = 0;
    x = 0;
    strLength = 0;
  }
}

void displayCoordinates() {
  lcd.clear();
  lcd.print("Lat:");
  lcd.print(latitude);
  lcd.setCursor(0, 1);
  lcd.print("Lon:");
  lcd.print(longitude);

  Serial.print("Latitude:");
  Serial.println(latitude);
  Serial.print("Longitude:");
  Serial.println(longitude);
  Serial.print("Speed(in knots)=");
  Serial.println(speed);
  delay(2000);
  lcd.clear();
  lcd.print("Speed(Knots):");
  lcd.setCursor(0, 1);
  lcd.print(speed);
}

void convertCoordinateToDecimal() {
  String latDegree = "";
  for (sampleIndex = 20; sampleIndex <= 21; sampleIndex++)
    latDegree += gpsString[sampleIndex];

  String latMinute = "";
  for (sampleIndex = 22; sampleIndex <= 28; sampleIndex++)
    latMinute += gpsString[sampleIndex];

  String lonDegree = "";
  for (sampleIndex = 32; sampleIndex <= 34; sampleIndex++)
    lonDegree += gpsString[sampleIndex];

  String lonMinute = "";
  for (sampleIndex = 35; sampleIndex <= 41; sampleIndex++)
    lonMinute += gpsString[sampleIndex];

  speed = "";
  for (sampleIndex = 45; sampleIndex < 48; sampleIndex++)
    speed += gpsString[sampleIndex];

  float minute = latMinute.toFloat();
  minute = minute / 60;
  float degree = latDegree.toFloat();
  latitude = degree + minute;

  minute = lonMinute.toFloat();
  minute = minute / 60;
  degree = lonDegree.toFloat();
  longitude = degree + minute;
}

void sendSms() {
  gsmSerial.println("AT");
  delay(500);
  printSerialData();

  gsmSerial.println("AT+CMGF=1");
  delay(500);
  printSerialData();

  gsmSerial.print("AT+CMGS=");
  gsmSerial.print('"');
  gsmSerial.print("9345734565"); // Mobile number for SMS alert
  gsmSerial.println('"');
  delay(500);
  printSerialData();

  gsmSerial.print("Latitude:");
  gsmSerial.println(latitude);
  delay(500);
  printSerialData();

  gsmSerial.print("Longitude:");
  gsmSerial.println(longitude);
  delay(500);
  printSerialData();

  gsmSerial.print("Speed:");
  gsmSerial.print(speed);
  gsmSerial.println("Knots");
  delay(500);
  printSerialData();

  gsmSerial.print("http://maps.google.com/maps?&z=15&mrt=yp&t=k&q=");
  gsmSerial.print(latitude, 6);
  gsmSerial.print("+");
  gsmSerial.print(longitude, 6);
  gsmSerial.write(26);
  delay(2000);
  printSerialData();
}

void printSerialData() {
  while (gsmSerial.available() > 0) {
    Serial.print(gsmSerial.read());
  }
}
