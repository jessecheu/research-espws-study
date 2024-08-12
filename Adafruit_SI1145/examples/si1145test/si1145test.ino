/*************************************************** 
  This is a library for the Si1145 UV/IR/Visible Light Sensor

  Designed specifically to work with the Si1145 sensor in the
  adafruit shop
  ----> https://www.adafruit.com/products/1777

  These sensors use I2C to communicate, 2 pins are required to  
  interface
  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
 ****************************************************/

#include <Wire.h>
#include "Adafruit_SI1145.h"

/**
 * Adapted from BLE Client Example
 */

#include "BLEDevice.h"
#include <driver/rtc_io.h>

// Define UUIDs:
static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
static BLEUUID  charUUID_1("beb5483e-36e1-4688-b7f5-ea07361b26a8");

// Some variables to keep track on device connected
static boolean doConnect = false;
static boolean connected = false;

// Define pointer for the BLE connection
static BLEAdvertisedDevice* myDevice;
BLERemoteCharacteristic* pRemoteChar_1;
BLEClient*  pClient;

// Sleep logic
#define SLEEP_TIME 10 * 1000000 // 10 seconds
void deepSleep() {
  esp_sleep_enable_timer_wakeup(SLEEP_TIME);
  gpio_deep_sleep_hold_dis();
  esp_deep_sleep_start();
}

// Callback function that is called whenever a client is connected or disconnected
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    deepSleep(); // go to sleep
  }
};

// Function that is run whenever the server is connected
bool connectToServer() {
  Serial.println(myDevice->getAddress().toString().c_str());

  pClient  = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the remove BLE Server.
  pClient->connect(myDevice);

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    pClient->disconnect();
    return false;
  }

  connected = true;
  pRemoteChar_1 = pRemoteService->getCharacteristic(charUUID_1);
  if(pRemoteChar_1 == nullptr) {
    pClient-> disconnect();
    return false;
  }
  return true;
}

// Scan for BLE servers and find the first one that advertises the service we are looking for.
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  //Called for each advertising BLE server.
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
    }
  }
};

Adafruit_SI1145 uv = Adafruit_SI1145();

void setup() {
  Serial.begin(9600);
  
  Serial.println("Adafruit SI1145 test");
  
  if (! uv.begin()) {
    Serial.println("Didn't find Si1145");
    while (1);
  }

  Serial.println("OK!");

  //END LIGHT SENSOR

    BLEDevice::init("Your number here");
  Serial.begin(115200);

  // Turn LED off on esp for sleep
  pinMode(LED_BUILTIN, OUTPUT);
  gpio_hold_dis(GPIO_NUM_2);
  digitalWrite(LED_BUILTIN, LOW);
  gpio_hold_en(GPIO_NUM_2);

  // Scan for server
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}

void loop() {
  Serial.println("===================");
  Serial.print("Vis: "); Serial.println(uv.readVisible());
  Serial.print("IR: "); Serial.println(uv.readIR());
  
  // Uncomment if you have an IR LED attached to LED pin!
  //Serial.print("Prox: "); Serial.println(uv.readProx());

  float UVindex = uv.readUV();
  // the index is multiplied by 100 so to get the
  // integer index, divide by 100!
  UVindex /= 100.0;  
  Serial.print("UV: ");  Serial.println(UVindex);

  delay(1000);

  //END LIGHT CODE

  Serial.println("Attempting to data to the server...");
  //START BLE CODE

    if (doConnect == true) {
    connectToServer();
  }

  if (connected) {
    // Read from sensors

    // Format readings into: "Name,sensor,sensor_reading,sensor1,sensor1_readingg"
    // Example
    Serial.println("Sending data to the server...");
    String sensor_readings = "Berk, example, 1.0";
    // Can use .concat() to append values to string
    sensor_readings.concat(",light,");
    sensor_readings.concat(UVindex);
    Serial.println(sensor_readings);

    std::string str = sensor_readings.c_str();
    pClient->setMTU(512);

    pRemoteChar_1->writeValue((uint8_t*)str.c_str(), str.length());

    delay(5000);
    deepSleep();
  }

  //END BLE CODE
}