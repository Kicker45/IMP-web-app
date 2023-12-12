#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer* pServer = NULL;
BLECharacteristic* pSensorCharacteristic = NULL;
BLECharacteristic* pLedCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;
int speed = 2;
void updateSpeed (int newspeed);

const int redLEDPin = 12;
const int oraLEDPin = 13;
const int greLEDPin = 14;
const int redRGBPin = 17;
const int bluRGBPin = 25;
const int greRGBPin = 26;

#define SERVICE_UUID "bb85eff0-6d74-4f63-8085-71aa869df5f5"
#define SENSOR_CHARACTERISTIC_UUID "bb85eff1-6d74-4f63-8085-71aa869df5f5"
#define LED_CHARACTERISTIC_UUID "bb85eff2-6d74-4f63-8085-71aa869df5f5"

class ProgramQueue {
  int* queue;
  int front;
  int rear;
  int capacity;

public:
  ProgramQueue(int size) {
    queue = new int[size];
    capacity = size;
    front = rear = -1;
  }

  ~ProgramQueue() {
    delete[] queue;
  }

  bool isEmpty() {
    return front == -1;
  }

  bool isFull() {
    return (front == 0 && rear == capacity - 1) || front == rear + 1;
  }

  void enqueue(int programNumber) {
    if (isFull()) {
      Serial.println("Queue is full. Unable to enqueue.");
    } else {
      if (front == -1) front = 0;
      rear = (rear + 1) % capacity;
      queue[rear] = programNumber;
    }
  }

  int dequeue() {
    if (isEmpty()) {
      Serial.println("Queue is empty. Unable to dequeue.");
      return -1;
    } else {
      int programNumber = queue[front];
      if (front == rear) {
        front = rear = -1;
      } else {
        front = (front + 1) % capacity;
      }
      return programNumber;
    }
  }
};

ProgramQueue programQueue(10);  // Set the queue size as needed

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }
};

class MyCharacteristicCallbacks : public BLECharacteristicCallbacks {
public:
  void
  onWrite(BLECharacteristic* pCharacteristic) {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
      Serial.print("Characteristic event, written: ");
      Serial.println(static_cast<int>(value[0]));  // Print the integer value

      int receivedValue = static_cast<int>(value[0]);
      if (receivedValue >= 5 && receivedValue <= 7) {
        // Handle speed commands
        updateSpeed(receivedValue);
      } else {
        // Handle other commands
        programQueue.enqueue(receivedValue);
      }
    }
  }
};

void setup() {
  Serial.begin(115200);
  pinMode(redLEDPin, OUTPUT);
  pinMode(oraLEDPin, OUTPUT);
  pinMode(greLEDPin, OUTPUT);
  pinMode(redRGBPin, OUTPUT);
  pinMode(greRGBPin, OUTPUT);
  pinMode(bluRGBPin, OUTPUT);

  BLEDevice::init("ESP32");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);

  pSensorCharacteristic = pService->createCharacteristic(
    SENSOR_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_INDICATE);

  pLedCharacteristic = pService->createCharacteristic(
    LED_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_WRITE);

  pLedCharacteristic->setCallbacks(new MyCharacteristicCallbacks());

  pSensorCharacteristic->addDescriptor(new BLE2902());
  pLedCharacteristic->addDescriptor(new BLE2902());

  pService->start();

  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
  if (deviceConnected) {
    int programNumber = programQueue.dequeue();
    if (programNumber != -1) {
      executeProgram(programNumber);
    }

    pSensorCharacteristic->setValue(String((int)(value / 10)).c_str());
    pSensorCharacteristic->notify();
    value++;
    Serial.print("New value notified: ");
    Serial.println(value);
    delay(100);
  }

  if (!deviceConnected && oldDeviceConnected) {
    Serial.println("Device disconnected.");
    delay(500);
    pServer->startAdvertising();
    Serial.println("Start advertising");
    oldDeviceConnected = deviceConnected;
  }

  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
    Serial.println("Device Connected");
  }
}

void executeProgram(int programNumber) {
  switch (programNumber) {
    case 0:
      DisableAll();
      break;
    case 1:
      RGBCycle();
      DisableAll();
      break;
    case 2:
      SemaphoreCycle();
      DisableAll();
      break;
    case 3:
      RGBColorCycle();
      DisableAll();
      break;
    case 4:
      PoliceSiren();
      DisableAll();
      break;
    case 10:
      LightAll();
      break;
    default: break;
  }
}


void updateSpeed(int newspeed) {
  switch (newspeed) {
    case 5:
      speed = 4;
      break;
    case 6:
      speed = 2;
      break;
    case 7:
      speed = 1;
      break;
    default:
      speed = 1;
      break;
  }
}

void DisableAll() {
  digitalWrite(redLEDPin, LOW);
  digitalWrite(oraLEDPin, LOW);
  digitalWrite(greLEDPin, LOW);
  analogWrite(redRGBPin, 0);
  analogWrite(greRGBPin, 0);
  analogWrite(bluRGBPin, 0);
}

void RGBCycle() {
  float RGBDelay = speed * 10;
  for (int i = 0; i < 255; i++) {
    setColourRgb(i, 255 - i, 0);
    delay(RGBDelay);
  }
  for (int i = 0; i < 255; i++) {
    setColourRgb(255 - i, 0, i);
    delay(RGBDelay);
  }
  for (int i = 0; i < 255; i++) {
    setColourRgb(0, i, 255 - i);
    delay(RGBDelay);
  }
}

void setColourRgb(unsigned int red, unsigned int green, unsigned int blue) {
  analogWrite(redRGBPin, red);
  analogWrite(greRGBPin, green);
  analogWrite(bluRGBPin, blue);
}

void SemaphoreCycle() {
  digitalWrite(redLEDPin, HIGH);
  digitalWrite(oraLEDPin, LOW);
  digitalWrite(greLEDPin, LOW);
  delay(speed * 3000);  // 3 seconds
  digitalWrite(redLEDPin, HIGH);
  digitalWrite(oraLEDPin, HIGH);
  digitalWrite(greLEDPin, LOW);
  delay(speed * 1000);  // 1 second
  digitalWrite(redLEDPin, LOW);
  digitalWrite(oraLEDPin, LOW);
  digitalWrite(greLEDPin, HIGH);
  delay(speed * 4000);  // 4 seconds
  digitalWrite(redLEDPin, LOW);
  digitalWrite(oraLEDPin, HIGH);
  digitalWrite(greLEDPin, LOW);
  delay(speed * 1000);  // 1 seconds
  digitalWrite(redLEDPin, HIGH);
  digitalWrite(oraLEDPin, LOW);
  digitalWrite(greLEDPin, LOW);
  delay(speed * 2000);  // 2 seconds
}

void RGBColorCycle() {
  float cycleTime = speed * 1000;  // Initial cycle time (1 seconds)
  float whiteTime = speed * 2000;  // Time to display white color (2 seconds)

  while (true) {
    // Red
    analogWrite(redRGBPin, 255);
    analogWrite(greRGBPin, 0);
    analogWrite(bluRGBPin, 0);
    delay(cycleTime);

    // Yellow (combination of Red and Green)
    analogWrite(redRGBPin, 255);
    analogWrite(greRGBPin, 255);
    analogWrite(bluRGBPin, 0);
    delay(cycleTime);

    // Green
    analogWrite(redRGBPin, 0);
    analogWrite(greRGBPin, 255);
    analogWrite(bluRGBPin, 0);
    delay(cycleTime);

    // Cyan (combination of Green and Blue)
    analogWrite(redRGBPin, 0);
    analogWrite(greRGBPin, 255);
    analogWrite(bluRGBPin, 255);
    delay(cycleTime);

    // Blue
    analogWrite(redRGBPin, 0);
    analogWrite(greRGBPin, 0);
    analogWrite(bluRGBPin, 255);
    delay(cycleTime);

    // Magenta (combination of Red and Blue)
    analogWrite(redRGBPin, 255);
    analogWrite(greRGBPin, 0);
    analogWrite(bluRGBPin, 255);
    delay(cycleTime);


    // Shorten the cycle time for the next iteration
    cycleTime /= 2;

    // Break out of the loop if the cycle time is too short
    if (cycleTime < 10) {
      // White
      analogWrite(redRGBPin, 255);
      analogWrite(greRGBPin, 255);
      analogWrite(bluRGBPin, 255);
      delay(whiteTime);
      break;
    }
  }
}

void PoliceSiren() {
  float BasicDelay = speed * 250;
  for (int i = 0; i < 11; i++) {
    analogWrite(bluRGBPin, 255);
    digitalWrite(redLEDPin, LOW);
    delay(BasicDelay);
    analogWrite(bluRGBPin, 0);
    digitalWrite(redLEDPin, HIGH);
    delay(BasicDelay);
  }
}

void LightAll() {
  digitalWrite(redLEDPin, HIGH);
  digitalWrite(oraLEDPin, HIGH);
  digitalWrite(greLEDPin, HIGH);
  analogWrite(redRGBPin, 255);
  analogWrite(greRGBPin, 255);
  analogWrite(bluRGBPin, 255);
}
