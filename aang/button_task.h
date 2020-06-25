#ifndef _BUTTON_TASK_H_
#define _BUTTON_TASK_H_

#include <Arduino.h>

#define BUTTON_GPIO_UP          (14)
#define BUTTON_GPIO_DOWN        (33)
#define BUTTON_GPIO_LEFT        (32)
#define BUTTON_GPIO_RIGHT       (27)
#define BUTTON_GPIO_B           (2)
#define BUTTON_GPIO_LEFT_SELECT (12)
#define BUTTON_GPIO_A           (15)

void buttonTask(void* parameter);

#endif
