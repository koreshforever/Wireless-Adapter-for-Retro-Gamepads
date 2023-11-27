//6-значный PIN-код
#define SIX_DIGIT_PIN 461659


#include "HID.h"
#include "N64_UART.h"
#include "genesisPad.h"
#include "PSX_SPI.h"
#include "LEDs.h"

//Геймпады
N64pad N64(Serial2);
GenPad GPad = GenPad(13, 12, 14, 27, 26, 32, 25);
SPIPad PSX = SPIPad();

//Кнопка
#define MAIN_BUTTON 15

//Датчик заряда
#define BATT_PIN 36

//Светодиоды
#define LEDS_PIN 0


//#define SYNC_PERIOD 20000UL  //ЧАСТОТА ОТПРАВКИ РЕПОРТА 50 Гц
//#define SYNC_PERIOD 16666UL  //ЧАСТОТА ОТПРАВКИ РЕПОРТА 60 Гц
//#define SYNC_PERIOD 10000UL  //ЧАСТОТА ОТПРАВКИ РЕПОРТА 100 Гц
#define SYNC_PERIOD 8333UL  //ЧАСТОТА ОТПРАВКИ РЕПОРТА 120 Гц


//Захват таймера
unsigned long microCap = micros();

//Выбор на мультиплексоре
#define MUX_PIN 2
bool mux_sel = false;
#define CS1_PIN 5
#define CS2_PIN 4
//Функции управления мультиплексором
void muxFlip() {
  mux_sel = !mux_sel;
  digitalWrite(MUX_PIN, mux_sel);
}
inline bool getMux() {
  return mux_sel;
}
inline uint8_t currentCS() {
  if (mux_sel) return CS2_PIN;
  else return CS1_PIN;
}

//Работа с датчиком заряда
uint8_t battLevel[] = { 255, 255 };
//Половина напряжения минимума и максимума в милливольтах
#define BATT_MIN 1550
#define BATT_FULL 2050
#define BATT_DELTA 15

uint8_t battPercent = 100;
uint16_t readBattLevel() {
  uint32_t currentLevel = analogReadMilliVolts(BATT_PIN);
  if (currentLevel >= BATT_FULL + BATT_DELTA) battPercent = 100;
  else if (currentLevel <= BATT_MIN) battPercent = 1;
  else {
    uint8_t checkPercent;
    if (currentLevel > (BATT_MIN + BATT_DELTA)) {
      checkPercent = map(currentLevel - BATT_DELTA, BATT_MIN, BATT_FULL, 1, 100);
      if (checkPercent > battPercent) battPercent = checkPercent;
    }
    checkPercent = map(currentLevel, BATT_MIN, BATT_FULL, 1, 100);
    if (checkPercent < battPercent) battPercent = checkPercent;
  }
  return battPercent;
}
#define BATT_CHECK_PERIOD 10000  //Период проверки уровня батареи
bool batt_notif = false;         //Уведомление о заряде передано
unsigned long battCheckMillis = millis();
//Сама проверка уровня заряда
void checkBattLevel() {
  unsigned long battMilDelta = millis() - battCheckMillis;
  if (battMilDelta > BATT_CHECK_PERIOD) {
    while (battMilDelta > BATT_CHECK_PERIOD) {
      battCheckMillis += BATT_CHECK_PERIOD;
      battMilDelta = millis() - battCheckMillis;
    }
    battLevel[1] = battLevel[0];
    battLevel[0] = readBattLevel();
    if (battLevel[0] != battLevel[1]) {
      BATTindicator->setValue(&battLevel[0], 1);
      updateColor(battLevel[0]);
      batt_notif = false;
    }
  }
  if (connected) {
    if (!batt_notif) {
      BATTindicator->notify();
      batt_notif = true;
    }
  } else batt_notif = false;
}


void setup() {
  //Кнопочка
  pinMode(MAIN_BUTTON, INPUT_PULLUP);
  //Последовательный порт
  Serial.begin(115200);
  while (!digitalRead(MAIN_BUTTON))
    ;
  Serial.println("\nESP32 Gamepad adapter");
  Serial.print("Bluetooth PIN-code: ");
  Serial.println(SIX_DIGIT_PIN);
  //Геймпады N64
  N64.begin();
  //Геймпады Sega Genesis
  pinMode(MUX_PIN, OUTPUT);
  digitalWrite(MUX_PIN, mux_sel);
  GPad.begin(true);
  delay(5);
  mux_sel = !mux_sel;
  digitalWrite(MUX_PIN, mux_sel);
  GPad.begin(true);
  //Геймпады PSX
  PSX.begin(CS1_PIN);
  PSX.begin(CS2_PIN);
  //Светодиоды
  FastLED.addLeds<NEOPIXEL, LEDS_PIN>(pixels, sizeof(pixels));

  //Bluetooth
  xTaskCreate(taskServer, "server", 20000, NULL, 5, NULL);
}

//Массив выходных значений
uint8_t inputValues[7] = { 0, 0, 0, 0, 0, 0, 0 };
uint8_t keyboardValues[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

//ГЛАВНЫЙ ЦИКЛ
void loop() {
  unsigned long microDelta = micros() - microCap;
  while (microDelta < SYNC_PERIOD) {
    microDelta = micros() - microCap;
  }
  while (microDelta >= SYNC_PERIOD) {
    microCap += SYNC_PERIOD;
    microDelta = micros() - microCap;
  }

  if (connected) {
    hidReport();  //Если подключены то соберём и отправим HID репорт
    showLEDs(true);
  } else {
    showLEDs(false);
  }
  checkBattLevel();
}

//Проверка состояния кнопки
bool button_sate = true;
void checkButton() {
  bool currentState = digitalRead(MAIN_BUTTON);
  if (currentState != button_sate) {
    button_sate = currentState;
    if (currentState == true) {
      keyboardValues[2] = 0x3A;

      delay(20);
      bool mainPressed = false;
      uint32_t doubleTime = millis();
      while ((millis() - doubleTime) < 180) {
        if (!digitalRead(MAIN_BUTTON)) {
          mainPressed = true;
          break;
        }
      }
      delay(20);
      doubleTime = millis();
      while (((millis() - doubleTime) < 180) && mainPressed) {
        if (digitalRead(MAIN_BUTTON)) {
          keyboardValues[2] = 0x2A;
          break;
        }
      }

      HIDinput[2]->setValue(keyboardValues, sizeof(keyboardValues));
      HIDinput[2]->notify();
      delay(50);
      keyboardValues[2] = 0x00;
      HIDinput[2]->setValue(keyboardValues, sizeof(keyboardValues));
      HIDinput[2]->notify();
    }
  }
}

//Главная функция репорта
void hidReport() {
  if (connected) {

    refreshValues();

    HIDinput[getMux()]->setValue(inputValues, sizeof(inputValues));
    HIDinput[getMux()]->notify();
    muxFlip();

    checkButton();
  }
}

//ОБНОВИТЬ ЗНАЧЕНИЯ ГЕЙМПАДОВ
void refreshValues() {
  for (uint8_t i = 0; i < sizeof(inputValues); i++) inputValues[i] = 0;
  N64.requestControls();
  PSX.updateIO(currentCS());
  GPad.updateControls();

  //Посчитаем DPAD
  int8_t outDpad[] = { 0, 0 };
  outDpad[0] += N64.getDpad(0);
  outDpad[0] += PSX.getDpad(0);
  outDpad[0] += GPad.getAxis(0);
  outDpad[1] += N64.getDpad(1);
  outDpad[1] += PSX.getDpad(1);
  outDpad[1] += GPad.getAxis(1);
  //Выведем DPAD
  if (outDpad[1] > 0) {
    if (outDpad[0] < 0) inputValues[6] = DPAD_UP_LEFT;
    else if (!outDpad[0]) inputValues[6] = DPAD_UP;
    else inputValues[6] = DPAD_UP_RIGHT;
  } else if (!outDpad[1]) {
    if (outDpad[0] < 0) inputValues[6] = DPAD_LEFT;
    else if (!outDpad[0]) inputValues[6] = DPAD_CENTERED;
    else inputValues[6] = DPAD_RIGHT;
  } else {
    if (outDpad[0] < 0) inputValues[6] = DPAD_DOWN_LEFT;
    else if (!outDpad[0]) inputValues[6] = DPAD_DOWN;
    else inputValues[6] = DPAD_DOWN_RIGHT;
  }

  //Посчитаем главные оси
  int16_t mainAxes[] = { 0, 0 };
  mainAxes[0] += N64.getAxis(0);
  mainAxes[1] -= N64.getAxis(1);
  mainAxes[0] += PSX.getAxis(2);
  mainAxes[1] -= PSX.getAxis(3);
  if (mainAxes[0] < -127) mainAxes[0] = -127;
  if (mainAxes[1] < -127) mainAxes[1] = -127;
  if (mainAxes[0] > 127) mainAxes[0] = 127;
  if (mainAxes[1] > 127) mainAxes[1] = 127;
  //Выведем главные оси
  inputValues[2] = mainAxes[0];
  inputValues[3] = mainAxes[1];

  //Посчитаем дополнительные оси
  int16_t subAxes[] = { 0, 0 };
  subAxes[0] += PSX.getAxis(0);
  subAxes[1] += -PSX.getAxis(1);
  if (N64.getButton(8)) subAxes[0] += 127;
  if (N64.getButton(9)) subAxes[0] -= 127;
  if (N64.getButton(10)) subAxes[1] += 127;
  if (N64.getButton(11)) subAxes[1] -= 127;
  if (subAxes[0] < -127) subAxes[0] = -127;
  if (subAxes[1] < -127) subAxes[1] = -127;
  if (subAxes[0] > 127) subAxes[0] = 127;
  if (subAxes[1] > 127) subAxes[1] = 127;
  //Выведем дополнительные оси
  inputValues[4] = subAxes[0];
  inputValues[5] = subAxes[1];


  //ДАЛЬШЕ МАПНЕМ КНОПКИ

  //Квадрат (кнопка X)
  if (PSX.getButton(15)) inputValues[0] |= 1;
  if (N64.getButton(6)) inputValues[0] |= 1;
  if (GPad.getButton(0)) inputValues[0] |= 1;

  //Крест (кнопка A)
  if (PSX.getButton(14)) inputValues[0] |= 1 << 1;
  if (N64.getButton(7)) inputValues[0] |= 1 << 1;
  if (GPad.getButton(1)) inputValues[0] |= 1 << 1;

  //Круг (Кнопка B)
  if (PSX.getButton(13)) inputValues[0] |= 1 << 2;
  if (GPad.getButton(2)) inputValues[0] |= 1 << 2;

  //Треугольник (Кнопка Y)
  if (PSX.getButton(12)) inputValues[0] |= 1 << 3;
  if (GPad.getButton(5)) inputValues[0] |= 1 << 3;

  //Левый шифт
  if (PSX.getButton(10)) inputValues[0] |= 1 << 4;
  if (N64.getButton(13)) inputValues[0] |= 1 << 4;
  if (GPad.getButton(4)) inputValues[0] |= 1 << 4;

  //Правый шифт
  if (PSX.getButton(11)) inputValues[0] |= 1 << 5;
  if (N64.getButton(12)) inputValues[0] |= 1 << 5;
  if (GPad.getButton(6)) inputValues[0] |= 1 << 5;

  //Левый курок
  if (PSX.getButton(8)) inputValues[0] |= 1 << 6;
  if (N64.getButton(5)) inputValues[0] |= 1 << 6;

  //Правый курок
  if (PSX.getButton(9)) inputValues[0] |= 1 << 7;

  //Select
  if (PSX.getButton(0)) inputValues[1] |= 1;
  if (GPad.getButton(7)) inputValues[1] |= 1;

  //Start
  if (PSX.getButton(3)) inputValues[1] |= 1 << 1;
  if (N64.getButton(4)) inputValues[1] |= 1 << 1;
  if (GPad.getButton(3)) inputValues[1] |= 1 << 1;

  //Левый стик
  if (PSX.getButton(1)) inputValues[1] |= 1 << 2;

  //Левый стик
  if (PSX.getButton(2)) inputValues[1] |= 1 << 3;
}