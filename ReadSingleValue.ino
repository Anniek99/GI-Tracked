// GI Tracked v2 (with SD card logging)
// Records 3 mic channels + thermistor + RTC timestamp to microSD

#include <SPI.h>
#include "SdFat.h"
#include <Wire.h>
#include "RTClib.h"

// SD card setup (Adafruit microSD BFF)
#define SD_FAT_TYPE 3
#define SD_CS_PIN TX   
SdFat SD;
File dataFile;

// Sensor pins
const int mic1Pin = A0;
const int mic2Pin = A1;
const int mic3Pin = A2;
const int thermPin = A3;

// RTC
RTC_DS3231 rtc;

// Filename buffer
char filename[30];

// Flush counter - saves to card periodically so data
// survives if the battery dies mid-night
int sampleCount = 0;

void setup() {
  Serial.begin(57600);
  delay(2000);

  Serial.println("=== GI Tracked v2 starting ===");

  // ---- RTC setup ----
  Wire.setPins(SDA, SCL);
  Wire.begin();

  while (!rtc.begin()) {
    Serial.println("Couldn't find RTC, retrying...");
    delay(1000);
  }
  Serial.println("RTC found");

  // Set RTC to compile time if it lost power
if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

  DateTime now = rtc.now();
  Serial.print("RTC time: ");
  Serial.print(now.year()); Serial.print("-");
  Serial.print(now.month()); Serial.print("-");
  Serial.print(now.day()); Serial.print(" ");
  Serial.print(now.hour()); Serial.print(":");
  Serial.println(now.minute());

  // ---- SD card setup ----
  Serial.print("Initializing SD card...");

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("");
    Serial.println("=============================");
    Serial.println("SD CARD INIT FAILED!");
    Serial.println("Check:");
    Serial.println("  - Is a card inserted? Push until it clicks");
    Serial.println("  - Is card formatted as FAT32?");
    Serial.println("  - Is BFF seated properly on QT Py?");
    Serial.println("=============================");
    // Blink forever so know it failed
    while (1) {
      delay(1000);
      Serial.println("SD FAILED - insert card and reset");
    }
  }
  Serial.println("SD card OK!");

  // ---- Create filename from date/time ----
  // Format: YYYYMMDD_HHMM.csv
  sprintf(filename, "%04d%02d%02d_%02d%02d.csv",
          now.year(), now.month(), now.day(),
          now.hour(), now.minute());

  Serial.print("Opening file: ");
  Serial.println(filename);

  dataFile = SD.open(filename, FILE_WRITE);

  if (!dataFile) {
    Serial.println("=============================");
    Serial.println("FAILED TO OPEN FILE!");
    Serial.println("=============================");
    while (1) {
      delay(1000);
      Serial.println("FILE FAILED - check SD card and reset");
    }
  }

  Serial.println("File opened, recording started!");
  Serial.println("Unplug USB when done. Data is safe.");
  Serial.println("================================");
}

void loop() {
  // Read all sensors
  int mic1 = analogRead(mic1Pin);
  int mic2 = analogRead(mic2Pin);
  int mic3 = analogRead(mic3Pin);
  float thermVoltage = analogRead(thermPin) * (3.3 / 4096.0);
  DateTime now = rtc.now();

  // Build CSV line
  // descColon,smallInt,ascColon,thermistorVoltage,unixTimestamp
  dataFile.print(mic1);
  dataFile.print(',');
  dataFile.print(mic2);
  dataFile.print(',');
  dataFile.print(mic3);
  dataFile.print(',');
  dataFile.print(thermVoltage);
  dataFile.print(',');
  dataFile.println(now.unixtime());

  // Also print to serial for debugging
  Serial.print(mic1);
  Serial.print(',');
  Serial.print(mic2);
  Serial.print(',');
  Serial.print(mic3);
  Serial.print(',');
  Serial.print(thermVoltage);
  Serial.print(',');
  Serial.println(now.unixtime());

  sampleCount++;

  // Flush to SD card every 200 samples (~10 seconds)
 

  if (sampleCount >= 200) {
    dataFile.flush();
    sampleCount = 0;
  }

  delay(50);  // ~20 samples per second
}
