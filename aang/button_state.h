#ifndef _BUTTON_STATE_H_
#define _BUTTON_STATE_H_

#include <Arduino.h>

#define BUTTON_STATE_HISTORY_SIZE (10)

class ButtonState {
  private:
    bool buttonStates[BUTTON_STATE_HISTORY_SIZE][8];
    int buttonStatesIndex[8];
    int  buttonTops[8];
    
  public:
    static const int up        = 0;
    static const int down      = 1;
    static const int right     = 2;
    static const int left      = 3;
    static const int a         = 4;
    static const int b         = 5;
    static const int leftS     = 6;
  
    SemaphoreHandle_t xButtonSemaphore;

    void set(int button, int value);
    void setTop(int button, int value);
    bool pressed(int button);

    ButtonState();
};

#endif
