#include <SPI.h>

class SPIPad {
public:
  SPIPad() {
    SPI.begin(SCK, MISO, MOSI);
    SPI.setClockDivider(spiFrequencyToClockDiv(80000UL));
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(LSBFIRST);
    //pinMode(MISO, INPUT_PULLUP);
  }
  void begin(uint8_t cs_pin) {
    pinMode(cs_pin, OUTPUT);
    digitalWrite(cs_pin, HIGH);
  }
  void updateIO(uint8_t cs_pin) {
    digitalWrite(cs_pin, LOW);
    delayMicroseconds(20);
    SPI.transfer(0x01);
    uint8_t controllerType = SPI.transfer(0x42);  //Тип контроллера
    SPI.transfer(0x00);

    //Длинна принимаемых данных с контроллера (по умолчанию 0)
    uint8_t dataLength = 0;
    switch (controllerType) {
      case 0x41:
        dataLength = 2;
        break;
      case 0x53:
      case 0x73:
        dataLength = 6;
        break;
      case 0x79:
        dataLength = 18;
        break;
    }

    //Получение данных с контроллера
    for (uint8_t i = 0; i < sizeof(controls); i++) controls[i] = 0;
    for (uint8_t i = 0; i < dataLength; i++) {
      if (i < sizeof(controls))
        if (i == 2 || i == 4) {
          controls[i] = 128 + SPI.transfer(0x00);
          if (controls[i] == -128) controls[i] = -127;
        } else if (i == 3 || i == 5) {
          controls[i] = 128 - SPI.transfer(0x00);
          if (controls[i] == -128) controls[i] = 127;
        } else controls[i] = ~SPI.transfer(0x00);
      else SPI.transfer(0x00);
    }

    digitalWrite(cs_pin, HIGH);
  }
  //Вывод состояния кнопок
  bool getButton(uint8_t num) {
    if (num < 16) return controls[num / 8] & (1 << (num % 8));
    return false;
  }
  //Вывод состояния осей
  int8_t getAxis(uint8_t num) {
    if (num < 4) return controls[num + 2];
    return 0;
  }
  //Вывод состояния DPAD
  int8_t getDpad(uint8_t num) {
    int8_t data = 0;
    if (num < 2) {
      if (controls[0] & (1 << (5 - num))) data++;
      if (controls[0] & (1 << (7 - num))) data--;
    }
    return data;
  }

private:
  int8_t controls[6] = { 0, 0, 0, 0, 0, 0 };
};