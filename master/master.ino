#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEClient.h>
#include <cstring>

#define BUTTON_SLAVE1 18
#define BUTTON_SLAVE2 19
#define BUTTON_SLAVE3 21

struct SlaveConfig {
  const char* name;
  const char* serviceUUID;
  const char* charUUID;
  int buttonPin;
};

SlaveConfig slaves[] = {
  {"ESP32_SLAVE_1", "7a3c9e10-5b2d-4f6a-9c1e-2d8b4a6f0e31", "7a3c9e11-5b2d-4f6a-9c1e-2d8b4a6f0e31", BUTTON_SLAVE1},
  {"ESP32_SLAVE_2", "8b4d9e20-6c3e-5a7b-8d2f-3e9c5b7a0f42", "8b4d9e21-6c3e-5a7b-8d2f-3e9c5b7a0f42", BUTTON_SLAVE2},
  {"ESP32_SLAVE_3", "d3f7a821-6b94-4c2e-a105-8e7d3f9b6240", "d3f7a822-6b94-4c2e-a105-8e7d3f9b6240", BUTTON_SLAVE3}
};

const int SLAVE_COUNT = 3;

bool ledStates[SLAVE_COUNT]            = {false, false, false};
bool previousButtons[SLAVE_COUNT]      = {HIGH, HIGH, HIGH};
unsigned long lastPressMs[SLAVE_COUNT] = {0, 0, 0};

BLEAdvertisedDevice* targetDevice = nullptr;
String desiredName = "";
String desiredUUID = "";

class ScanCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) override {
    bool nameMatch = advertisedDevice.haveName() &&
                     String(advertisedDevice.getName().c_str()) == desiredName;
    bool uuidMatch = advertisedDevice.isAdvertisingService(BLEUUID(desiredUUID.c_str()));

    if (nameMatch || uuidMatch) {
      Serial.print("Found device: ");
      Serial.println(desiredName);
      if (targetDevice != nullptr) delete targetDevice;
      targetDevice = new BLEAdvertisedDevice(advertisedDevice);
      BLEDevice::getScan()->stop();
    }
  }
};

ScanCallbacks scanCallbacks;

bool sendCommand(const SlaveConfig& slave, const char* command) {
  if (targetDevice != nullptr) {
    delete targetDevice;
    targetDevice = nullptr;
  }

  desiredName = slave.name;
  desiredUUID = slave.serviceUUID;

  Serial.println();
  Serial.print("Scanning for ");
  Serial.println(slave.name);

  BLEScan* scan = BLEDevice::getScan();
  scan->clearResults();
  scan->setAdvertisedDeviceCallbacks(&scanCallbacks, false);
  scan->setActiveScan(true);
  scan->setInterval(100);
  scan->setWindow(99);
  scan->start(5, false);

  if (targetDevice == nullptr) {
    Serial.print("ERROR: Device not found: ");
    Serial.println(slave.name);
    scan->clearResults();
    return false;
  }

  bool ok = false;
  BLEClient* client = BLEDevice::createClient();

  Serial.print("Connecting to ");
  Serial.println(slave.name);

  if (client->connect(targetDevice)) {
    BLERemoteService* service = client->getService(BLEUUID(slave.serviceUUID));
    if (service == nullptr) {
      Serial.println("ERROR: Service not found.");
    } else {
      BLERemoteCharacteristic* ch = service->getCharacteristic(BLEUUID(slave.charUUID));
      if (ch == nullptr) {
        Serial.println("ERROR: Characteristic not found.");
      } else if (!ch->canWrite()) {
        Serial.println("ERROR: Characteristic cannot be written.");
      } else {
        ch->writeValue((uint8_t*)command, strlen(command), true);
        Serial.print("Command sent to ");
        Serial.print(slave.name);
        Serial.print(": ");
        Serial.println(command);
        ok = true;
      }
    }
    client->disconnect();
  } else {
    Serial.println("ERROR: Connection failed.");
  }

  delete client;
  delete targetDevice;
  targetDevice = nullptr;
  scan->clearResults();
  delay(300);
  return ok;
}

void handleButton(int i) {
  bool current = digitalRead(slaves[i].buttonPin);

  if (previousButtons[i] == HIGH && current == LOW &&
      millis() - lastPressMs[i] > 300) {
    lastPressMs[i] = millis();

    const char* command = ledStates[i] ? "OFF" : "HELLO";

    Serial.println();
    Serial.print("Button GPIO ");
    Serial.print(slaves[i].buttonPin);
    Serial.print(" pressed: ");
    Serial.println(slaves[i].name);

    if (sendCommand(slaves[i], command)) {
      ledStates[i] = !ledStates[i];
    } else {
      Serial.println("Command failed. LED state unchanged.");
    }

    current = digitalRead(slaves[i].buttonPin);
  }

  previousButtons[i] = current;
}

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < SLAVE_COUNT; i++) {
    pinMode(slaves[i].buttonPin, INPUT_PULLUP);
  }
  delay(50);

  for (int i = 0; i < SLAVE_COUNT; i++) {
    bool s = digitalRead(slaves[i].buttonPin);
    Serial.print("GPIO ");
    Serial.print(slaves[i].buttonPin);
    Serial.println(s == HIGH ? " = HIGH (ok)" : " = LOW (STUCK! check wiring)");
    previousButtons[i] = s;
  }

  BLEDevice::init("ESP32_MASTER");

  Serial.println("==============================");
  Serial.println("ESP32 BLE MASTER READY");
  Serial.println("GPIO 18 -> Slave 1");
  Serial.println("GPIO 19 -> Slave 2");
  Serial.println("GPIO 21 -> Slave 3");
  Serial.println("1st press: HELLO (LED ON), 2nd press: OFF");
  Serial.println("==============================");
}

void loop() {
  for (int i = 0; i < SLAVE_COUNT; i++) handleButton(i);
  delay(10);
}
