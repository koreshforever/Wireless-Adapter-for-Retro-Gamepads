#include <FastLED.h>

CRGB pixels[2];  // ТЕКУЩИЙ ЦВЕТ ПИКСЕЛЕЙ

//Расчитаем цвет, исходя из уровня заряда
uint8_t pixelsColor[] = { 0, 0, 0 };
void updateColor(uint8_t &level) {
  if (level > 89) {
    pixelsColor[0] = 0;
    pixelsColor[1] = 0;
    pixelsColor[2] = 2;
  } else if (level > 78) {
    pixelsColor[0] = 0;
    pixelsColor[1] = 1;
    pixelsColor[2] = 2;
  } else if (level > 67) {
    pixelsColor[0] = 0;
    pixelsColor[1] = 2;
    pixelsColor[2] = 2;
  } else if (level > 56) {
    pixelsColor[0] = 0;
    pixelsColor[1] = 2;
    pixelsColor[2] = 1;
  } else if (level > 45) {
    pixelsColor[0] = 0;
    pixelsColor[1] = 2;
    pixelsColor[2] = 0;
  } else if (level > 34) {
    pixelsColor[0] = 1;
    pixelsColor[1] = 2;
    pixelsColor[2] = 0;
  } else if (level > 23) {
    pixelsColor[0] = 2;
    pixelsColor[1] = 2;
    pixelsColor[2] = 0;
  } else if (level > 12) {
    pixelsColor[0] = 2;
    pixelsColor[1] = 1;
    pixelsColor[2] = 0;
  } else {
    pixelsColor[0] = 2;
    pixelsColor[1] = 0;
    pixelsColor[2] = 0;
  }
}

#define CONN_FRAMESKIP 20   //ПРОПУСКАЕМЫЕ КАДРЫ
//Переменные анимации
uint8_t frame_skip = 0;
uint8_t num_of_frame = 0;

//ПОКАЗАТЬ ЦВЕТА
//КАЖДЫЙ ВЫЗОВ УВЕЛИЧИВАЕТ СЧЁТЧИК КАДРОВ
void showLEDs(bool connected) {
  if (connected) {
    if (frame_skip++ > CONN_FRAMESKIP) {
      frame_skip = 0;
      uint8_t brightness[2];
      if (num_of_frame++ >= 62) num_of_frame = 0;
      if (num_of_frame < 32) {
        brightness[0] = num_of_frame;
        brightness[1] = 31 - num_of_frame;
      } else {
        brightness[0] = 31 - (num_of_frame - 32);
        brightness[1] = num_of_frame - 32;
      }
      pixels[0] = CRGB(brightness[0] * pixelsColor[0], brightness[0] * pixelsColor[1], brightness[0] * pixelsColor[2]);
      pixels[1] = CRGB(brightness[1] * pixelsColor[0], brightness[1] * pixelsColor[1], brightness[1] * pixelsColor[2]);
      FastLED.show();
    }
  } else {
    if (millis() % 4096 < 512) {
      pixels[0] = CRGB(31 * pixelsColor[0], 31 * pixelsColor[1], 31 * pixelsColor[2]);
      pixels[1] = CRGB(31 * pixelsColor[0], 31 * pixelsColor[1], 31 * pixelsColor[2]);
    } else {
      pixels[0] = CRGB(0, 0, 0);
      pixels[1] = CRGB(0, 0, 0);
    }
    FastLED.show();
  }
}
