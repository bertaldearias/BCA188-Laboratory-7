#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#define SLAVE_NAME   "ESP32_SLAVE_2"
#define LED_PIN      4
#define SERVICE_UUID "8b4d9e20-6c3e-5a7b-8d2f-3e9c5b7a0f42"
#define CHAR_UUID    "8b4d9e21-6c3e-5a7b-8d2f-3e9c5b7a0f42"

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* s) override {
    Serial.println("Master connected to Slave 2");
  }
  void onDisconnect(BLEServer* s) override {
    Serial.println("Master disconnected");
    BLEDevice::startAdvertising();
    Serial.println("Advertising again...");
  }
};

class CommandCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* c) override {
    String command = String(c->getValue().c_str());
    command.trim();
    Serial.print("Command received: ");
    Serial.println(command);

    if (command == "HELLO") {
      digitalWrite(LED_PIN, HIGH);
      Serial.println("Slave 2 LED ON - HELLO received");
    } else if (command == "OFF") {
      digitalWrite(LED_PIN, LOW);
      Serial.println("Slave 2 LED OFF");
    } else {
      Serial.println("Unknown command");
    }
  }
};

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

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
  Serial.println("ESP32_SLAVE_2 READY");
  Serial.println("Waiting for Master commands...");
  Serial.println("==============================");
}

void loop() {
  delay(100);
}
