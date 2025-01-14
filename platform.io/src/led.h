/*
 * Rad Pro
 * LED
 *
 * (C) 2022-2025 Gissio
 *
 * License: MIT
 */

#if !defined(LED_H)
#define LED_H

#include <stdbool.h>

#include "view.h"

extern const View pulseLEDMenuView;

void initLED(void);
void initLEDController(void);

void setPulseLED(bool value);
void setAlertLED(bool value);

#endif
