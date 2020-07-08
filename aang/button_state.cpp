#include "button_state.h"

void ButtonState::set(int button, int value) {
  if ( xSemaphoreTake(xButtonSemaphore, ( TickType_t ) 5 ) == pdTRUE ) {  
    buttonStates[buttonStatesIndex[button]][button] = (value < (buttonTops[button] - 5));

    buttonStatesIndex[button]++;
    if(buttonStatesIndex[button] >= BUTTON_STATE_HISTORY_SIZE) {
      buttonStatesIndex[button] = 0; 
    }

    xSemaphoreGive(xButtonSemaphore);
  }
}

void ButtonState::setTop(int button, int value) {
  if ( xSemaphoreTake(xButtonSemaphore, ( TickType_t ) 5 ) == pdTRUE ) {  
    buttonTops[button] = value;
    
    xSemaphoreGive(xButtonSemaphore);
  }
}

bool ButtonState::pressed(int button) {
  bool rv = false;

  if ( xSemaphoreTake(xButtonSemaphore, ( TickType_t ) 5 ) == pdTRUE ) {
    int buttonPressedCount = 0;
    for(int i = 0; i < BUTTON_STATE_HISTORY_SIZE; i++) {
      if(buttonStates[i][button]) {
        buttonPressedCount++;
      }
    }
    
    if(buttonPressedCount >= 3) {
      for(int i = 0; i < BUTTON_STATE_HISTORY_SIZE; i++) {
        buttonStates[i][button] = false;
      }

      rv = true;
    }
    
    xSemaphoreGive(xButtonSemaphore);
  }

  return rv;
}

ButtonState::ButtonState()
  : xButtonSemaphore(xSemaphoreCreateMutex())
{
  for(int i = 0; i < 6; i++) {
    for(int historyIndex = 0; historyIndex < BUTTON_STATE_HISTORY_SIZE; historyIndex++) {
      buttonStates[historyIndex][i] = false;
    }
    
    buttonStatesIndex[i] = 0;
    buttonTops[i] = 0;
  }

  xSemaphoreGive(xButtonSemaphore);
}
