#include "button_state.h"

void ButtonState::set(int button, int value) {
  if ( xSemaphoreTake(xButtonSemaphore, ( TickType_t ) 5 ) == pdTRUE ) {  
    buttonStates[button] = (value < (buttonTops[button] - 5));

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
    if(buttonStates[button]) {
      rv = true;
      buttonStates[button] = false;
    }

    xSemaphoreGive(xButtonSemaphore);
  }

  return rv;
}

ButtonState::ButtonState()
  : xButtonSemaphore(xSemaphoreCreateMutex())
{
  for(int i = 0; i < 6; i++) {
    buttonStates[i] = false;
    buttonTops[i] = 0;
  }

  xSemaphoreGive(xButtonSemaphore);
}
