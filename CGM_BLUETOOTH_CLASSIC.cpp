#include <Arduino.h>
#include <BluetoothSerial.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

BluetoothSerial SerialBT;
int glucoseLevel;
float sensor_noise;

const int pattern_senario = 4;
const int patterns[pattern_senario][8] = {
  {150, 150, 150, 150, 150, 150, 150, 150},           // senario Pattern 1: constant glucose readings 
  {140, 145, 150, 155, 160, 165, 170, 175},           // senario Pattern 2: Gradual increase in glucose levels
  {145, 140, 150, 155, 160, 155, 150, 145},           // senario Pattern 3: round randomness readings
  {150, 200, 180, 170, 160, 150, 140, 130}            // senario Pattern 4: spiked readings  
};

int patternIndex;
const int *pattern = NULL;

int previousGlucoseLevel;
bool significantChangeFlag = false;

unsigned long lastSentTime = 0;
const unsigned long interval = 10000; // 10 seconds interval in milliseconds

void handleBluetoothTask(void *parameter) {
  while (1) {
    if (SerialBT.connected()) {
      // Check if data is available to receive
      if (SerialBT.available()) {
        // Read the received data
        String receivedData = SerialBT.readStringUntil('\n');
        // Process the received data here if needed
        Serial.println("Received data: " + receivedData);


        
      }
    }

    vTaskDelay(pdMS_TO_TICKS(1000)); // 1-second delay between each iteration
  }
}

void generateGlucoseTask(void *parameter) {
  while (1) {
    if (SerialBT.connected()) {
      // Check if 5 minutes have elapsed since the last data sending
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

        SerialBT.print(glucoseLevel);         // Readings transmitted into Bluetooth communication

        lastSentTime = millis(); // Update the last sent time to the current time
      }
    }

    vTaskDelay(pdMS_TO_TICKS(1000)); // 1-second delay between each iteration
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(921600);
  SerialBT.begin("CGM Sensor"); // Micro-controller sensor name appears for paired device

  delay(15000);

  xTaskCreatePinnedToCore(
    handleBluetoothTask,        // Function to run on this task
    "BluetoothTask",            // Name of the task
    4096,                       // Stack size (words, not bytes)
    NULL,                       // Task parameter
    1,                          // Priority (0 is the lowest)
    NULL,                       // Task handle
    0                           // Core to run the task on (0 or 1)
  );

  xTaskCreatePinnedToCore(
    generateGlucoseTask,        // Function to run on this task
    "GlucoseTask",              // Name of the task
    4096,                       // Stack size (words, not bytes)
    NULL,                       // Task parameter
    1,                          // Priority (0 is the lowest)
    NULL,                       // Task handle
    0                           // Core to run the task on (0 or 1)
  );
}

void loop() {
}
