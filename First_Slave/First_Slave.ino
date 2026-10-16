#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

// ---- Must match the Master's slaves[0] entry ----
#define SLAVE_NAME   "ESP32_SLAVE_1"
#define SERVICE_UUID "7a3c9e10-5b2d-4f6a-9c1e-2d8b4a6f0e31"
#define CHAR_UUID    "7a3c9e11-5b2d-4f6a-9c1e-2d8b4a6f0e31"
// -------------------------------------------------

#define LED_PIN 4

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* s) override {
    Serial.println("Master connected to Slave 1");
  }
  void onDisconnect(BLEServer* s) override {
    Serial.println("Master disconnected");
    BLEDevice::startAdvertising();   // so the Master can connect again
    Serial.println("Advertising again...");
  }
};

class CommandCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* c) override {
    String command = String(c->getValue().c_str());
    command.trim();

    Serial.print("Command received: ");
    Serial.println(command);

    if (command == "HELLO" || command == "ON") {
      digitalWrite(LED_PIN, HIGH);
      Serial.println("Slave 1 LED ON");
    } else if (command == "OFF") {
      digitalWrite(LED_PIN, LOW);
      Serial.println("Slave 1 LED OFF");
    } else {
      Serial.println("Unknown command");
    }
  }
};

void setup() {
  Serial.begin(115200);
  delay(2000);   // lets the Serial Monitor connect

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Quick LED check at boot: 2 blinks
  for (int i = 0; i < 2; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }

  BLEDevice::init(SLAVE_NAME);
  BLEServer* server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  BLEService* service = server->createService(BLEUUID(SERVICE_UUID));
  BLECharacteristic* ch = service->createCharacteristic(
    BLEUUID(CHAR_UUID),
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
  );
  ch->setCallbacks(new CommandCallbacks());
  ch->setValue("OFF");
  service->start();

  BLEAdvertising* adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(SERVICE_UUID);
  adv->setScanResponse(true);
  BLEDevice::startAdvertising();

  Serial.println("==============================");
  Serial.println("ESP32_SLAVE_1 READY");
  Serial.println("Waiting for Master commands...");
  Serial.println("==============================");
}

void loop() {
  delay(100);
}
