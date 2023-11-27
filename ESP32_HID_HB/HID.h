#include "HIDReportD.h"

#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
NimBLECharacteristic* HIDinput[3];
NimBLECharacteristic* BATTindicator;

bool connected = false;

class ConnectionCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) {
    pServer->updateConnParams(desc->conn_handle, 6, 7, 0, 600);
    connected = true;
  }
  void onDisconnect(NimBLEServer* pServer) {
    connected = false;
  }
};

void taskServer(void*) {
  NimBLEDevice::init("ESP32 Gamepad adapter");
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);  //УСТАНОВКА МОЩНОСТИ

  //Безопасность устройства
  NimBLEDevice::setSecurityAuth(true, true, true);
  NimBLEDevice::setSecurityPasskey(SIX_DIGIT_PIN);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);

  NimBLEServer* pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ConnectionCallbacks());

  NimBLEHIDDevice* hid = new NimBLEHIDDevice(pServer);

  HIDinput[0] = hid->inputReport(1);
  HIDinput[1] = hid->inputReport(2);
  HIDinput[2] = hid->inputReport(3);

  hid->manufacturer()->setValue("Espressif");

  NimBLEService* pService = pServer->getServiceByUUID("180A");  // Service - Device information

  BLECharacteristic* pCharacteristic_Model_Number = pService->createCharacteristic(
    "2A24",  // Characteristic - Model Number String - 0x2A24
    NIMBLE_PROPERTY::READ);
  pCharacteristic_Model_Number->setValue("1.0.0");

  BLECharacteristic* pCharacteristic_Software_Revision = pService->createCharacteristic(
    "2A28",  // Characteristic - Software Revision String - 0x2A28
    NIMBLE_PROPERTY::READ);
  pCharacteristic_Software_Revision->setValue("1.0.0");

  BLECharacteristic* pCharacteristic_Serial_Number = pService->createCharacteristic(
    "2A25",  // Characteristic - Serial Number String - 0x2A25
    NIMBLE_PROPERTY::READ);
  pCharacteristic_Serial_Number->setValue("4839971075");

  BLECharacteristic* pCharacteristic_Firmware_Revision = pService->createCharacteristic(
    "2A26",  // Characteristic - Firmware Revision String - 0x2A26
    NIMBLE_PROPERTY::READ);
  pCharacteristic_Firmware_Revision->setValue("1.0.0");

  BLECharacteristic* pCharacteristic_Hardware_Revision = pService->createCharacteristic(
    "2A27",  // Characteristic - Hardware Revision String - 0x2A27
    NIMBLE_PROPERTY::READ);
  pCharacteristic_Hardware_Revision->setValue("1.0.0");

#define VENDOR_ID 0x6D04
#define PRODUCT_ID 0x18C2

  hid->pnp(0x01, VENDOR_ID, PRODUCT_ID, 0x0110);
  hid->hidInfo(0x00, 0x01);

  hid->reportMap((uint8_t*)ReportDescriptor, sizeof(ReportDescriptor));
  hid->startServices();

  NimBLEAdvertising* pAdvertising = pServer->getAdvertising();
  pAdvertising->setAppearance(HID_GAMEPAD);
  pAdvertising->addServiceUUID(hid->hidService()->getUUID());
  pAdvertising->start();

  BATTindicator = hid->batteryLevel();
  uint8_t bat_lev = 50;
  BATTindicator->setValue(&bat_lev, 1);

  vTaskDelay(portMAX_DELAY);
}


//ПОЛОЖЕНИЯ DPAD
#define DPAD_CENTERED 0
#define DPAD_UP 1
#define DPAD_UP_RIGHT 2
#define DPAD_RIGHT 3
#define DPAD_DOWN_RIGHT 4
#define DPAD_DOWN 5
#define DPAD_DOWN_LEFT 6
#define DPAD_LEFT 7
#define DPAD_UP_LEFT 8