#include <Arduino.h>

class GenPad {
private:
  uint8_t input[6];
  uint8_t output;
  bool pup = false;
  int8_t axes[2] = { 0, 0 };                                                     // X and Y
  bool buttons[8] = { false, false, false, false, false, false, false, false };  // A, B, C, START, X, Y, Z, MODE
  bool sixB = true;

public:
  GenPad(uint8_t UP_UP_Z, uint8_t DOWN_DOWN_Y, uint8_t LEFT_ZERO_X, uint8_t RIGHT_ZERO_MODE, uint8_t B_A_NONE, uint8_t C_START_NONE, uint8_t SELECT) {
    output = SELECT;
    input[0] = UP_UP_Z;
    input[1] = DOWN_DOWN_Y;
    input[2] = LEFT_ZERO_X;
    input[3] = RIGHT_ZERO_MODE;
    input[4] = B_A_NONE;
    input[5] = C_START_NONE;
  }
  void begin(bool PULL_UP) {
    pinMode(output, OUTPUT);
    digitalWrite(output, HIGH);
    for (uint8_t i = 0; i < 6; i++)
      pinMode(input[i], PULL_UP ? INPUT_PULLUP : INPUT);
    delay(10);
    if (sixB) {
      updateControls();
      if (buttons[4] || buttons[7]) {
        sixB = false;
        buttons[4] = buttons[5] = buttons[6] = buttons[7] = false;
      }
    }
  }
  void updateControls() {

    delayMicroseconds(20);
    axes[0] = axes[1] = 0;
    if (!digitalRead(input[0])) axes[1]++;
    if (!digitalRead(input[1])) axes[1]--;
    if (!digitalRead(input[2])) axes[0]--;
    if (!digitalRead(input[3])) axes[0]++;
    buttons[1] = !digitalRead(input[4]);
    buttons[2] = !digitalRead(input[5]);

    digitalWrite(output, LOW);
    delayMicroseconds(20);
    buttons[0] = !digitalRead(input[4]);
    buttons[3] = !digitalRead(input[5]);
    digitalWrite(output, HIGH);

    if (sixB) {
      delayMicroseconds(20);
      digitalWrite(output, LOW);
      delayMicroseconds(20);
      digitalWrite(output, HIGH);
      delayMicroseconds(20);
      digitalWrite(output, LOW);
      delayMicroseconds(20);
      digitalWrite(output, HIGH);
      delayMicroseconds(20);

      buttons[4] = !digitalRead(input[2]);
      buttons[5] = !digitalRead(input[1]);
      buttons[6] = !digitalRead(input[0]);
      buttons[7] = !digitalRead(input[3]);

      digitalWrite(output, LOW);
      delayMicroseconds(20);
      digitalWrite(output, HIGH);
    }

    //delayMicroseconds(20);
  }
  int8_t getAxis(int8_t val) {
    if (val > 1)
      return 0;
    return axes[val];
  }
  bool getButton(uint8_t val) {
    if (val > 7)
      return false;
    return buttons[val];
  }
  bool sixPresent() {
    return sixB;
  }
};