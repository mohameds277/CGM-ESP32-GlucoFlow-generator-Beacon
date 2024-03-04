#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;

int glucoseLevel;
float sensor_noise;

const int pattern_senario = 4;
const int patterns[pattern_senario][8] = {
  {150, 150, 150, 150, 150, 150, 150, 150},           // Scenario Pattern 1: constant glucose readings 
  {140, 145, 150, 155, 160, 165, 170, 175},           // Scenario Pattern 2: Gradual increase in glucose levels
  {145, 140, 150, 155, 160, 155, 150, 145},           // Scenario Pattern 3: round randomness readings
  {150, 200, 180, 170, 160, 150, 140, 130}            // Scenario Pattern 4: spiked readings  
};

int patternIndex;
const int *pattern = NULL;

unsigned long lastSentTime = 0;
const unsigned long interval = 10000; // 10 seconds interval in milliseconds

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      // Do nothing on connect event
    }

    void onDisconnect(BLEServer* pServer) {
      // Do nothing on disconnect event
    }
};

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(921600);

  // Create the BLE Device
  BLEDevice::init("CGM Sensor");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService* pService = pServer->createService(BLEUUID((uint16_t)0x180F));

  // Create the BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      BLEUUID((uint16_t)0x2A19),
                      BLECharacteristic::PROPERTY_READ
                    );

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(pService->getUUID());
  pAdvertising->start();

  delay(15000);
}

void loop() {
  if (millis() - lastSentTime >= interval) {
    patternIndex = random(pattern_senario);
    pattern = patterns[patternIndex];

    glucoseLevel = pattern[millis() % 8];

    sensor_noise = random(-5, 5) / 10.0;
    glucoseLevel = glucoseLevel + (glucoseLevel * sensor_noise);

    // Hypoglycemia ALERT
    if (glucoseLevel < 70) {
      Serial.println("Hypoglycemia ALERT!");
    }
    // Hyperglycemia ALERT
    else if (glucoseLevel > 200) {
      Serial.println("Hyperglycemia ALERT!");
    }

    Serial.print("Glucose Level: ");
    Serial.print(glucoseLevel);           // Readings printed in serial monitor 
    Serial.println(" mg/dL");

    // Update BLE characteristic value with glucose level
    pCharacteristic->setValue(glucoseLevel);
    pCharacteristic->notify();

    lastSentTime = millis(); // Update the last sent time to the current time
  }

  delay(1000); // 1-second delay between each iteration
}
