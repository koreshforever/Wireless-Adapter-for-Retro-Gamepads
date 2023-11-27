#include "esp32-hal-uart.h"
#include "HardwareSerial.h"

class N64pad {
public:
  N64pad(HardwareSerial &serial) {
    ser = &serial;
  }

  //Запуск последовательного порта
  void begin() {
    ser->begin(2500000, SERIAL_6N1);  //расчётная скорость порта ровно 2 мегабод/сек, но китайский геймпад на ней не работает
  }

  //Запрос значений с геймпада
  void requestControls() {
    while (ser->available()) ser->read();
    uint8_t message[] = {
      0x20,
      0x20,
      0x20,
      0x20,
      0x20,
      0x20,
      0x20,
      0x3E,
      0x00
    };
    ser->write(message, sizeof(message));
    ser->flush();
    delay(1);

    controls[0] = controls[1] = controls[2] = controls[3] = 0;

    for (uint8_t i = 0; i < 9; i++) {
      if (!ser->available()) return;
      ser->read();
    }

    for (uint8_t i = 0; i < 4; i++) {
      for (uint8_t j = 0; j < 8; j++) {
        if (!ser->available()) return;
        uint8_t val = ser->read();
        if (val & (1 << 2 | 1 << 3)) controls[i] |= (1 << (7 - j));
      }
    }

    while (ser->available()) ser->read();
    if (controls[2] == -128) controls[2] = -127;
    if (controls[3] == -128) controls[3] = -127;
  }

  //Вывод значений осей (0 - X, 1 - Y)
  int8_t getAxis(uint8_t num) {
    if (num > 1) return 0;
    return controls[2 + num];
  }

  //Вывод состояния кнопки
  bool getButton(uint8_t num) {
    if (num > 15) return false;
    return controls[num / 8] & (1 << (num % 8));
  }
  //Вывод состояния DPAD
  int8_t getDpad(uint8_t num) {
    int8_t data = 0;
    if (num == 0) {
      if (controls[0] & 1) data++;
      if (controls[0] & (1 << 1)) data--;
    }
    if (num == 1) {
      if (controls[0] & (1 << 3)) data++;
      if (controls[0] & (1 << 2)) data--;
    }
    return data;
  }
private:
  HardwareSerial *ser = nullptr;
  int8_t controls[4] = { 0, 0, 0, 0 };
};