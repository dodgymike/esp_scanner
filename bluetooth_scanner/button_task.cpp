#include "button_task.h"
#include "button_state.h"

void buttonTask(void* parameter) {
  ButtonState* buttonState = (ButtonState*)parameter;

  digitalWrite(BUTTON_GPIO_UP,   LOW); // UP
  digitalWrite(BUTTON_GPIO_DOWN, LOW); // DOWN
  digitalWrite(BUTTON_GPIO_RIGHT,     LOW); // RIGHT
  digitalWrite(BUTTON_GPIO_LEFT,      LOW); // LEFT

  pinMode(BUTTON_GPIO_UP,    INPUT); // UP
  pinMode(BUTTON_GPIO_DOWN,  INPUT); // DOWN
  pinMode(BUTTON_GPIO_RIGHT, INPUT); // UP
  pinMode(BUTTON_GPIO_LEFT,  INPUT); // DOWN

  unsigned long startMillis = millis();

  int upA    = 0;
  int downA  = 0;
  int leftA  = 0;
  int rightA = 0;
  int checkCount = 0;

  while(millis() - startMillis < 1500) {
    upA    += touchRead(BUTTON_GPIO_UP);
    downA  += touchRead(BUTTON_GPIO_DOWN);
    leftA  += touchRead(BUTTON_GPIO_LEFT);
    rightA += touchRead(BUTTON_GPIO_RIGHT);
    checkCount++;

    delay(50);
  }

  buttonState->setTop(ButtonState::up,    upA    / checkCount);
  buttonState->setTop(ButtonState::down,  downA  / checkCount);
  buttonState->setTop(ButtonState::left,  leftA  / checkCount);
  buttonState->setTop(ButtonState::right, rightA / checkCount);

  while(true) {
    buttonState->set(ButtonState::up,    touchRead(BUTTON_GPIO_UP));
    buttonState->set(ButtonState::down,  touchRead(BUTTON_GPIO_DOWN));
    buttonState->set(ButtonState::left,  touchRead(BUTTON_GPIO_LEFT));
    buttonState->set(ButtonState::right, touchRead(BUTTON_GPIO_RIGHT));

    delay(50);
  }
}
