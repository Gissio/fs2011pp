/*
 * Rad Pro
 * Simulator pulse LED
 *
 * (C) 2022-2024 Gissio
 *
 * License: MIT
 */

#if defined(SIMULATOR)

#if defined(PULSE_LED)

#include "../display.h"
#include "../settings.h"

static bool pulseLED;

void initPulseLEDController(void)
{
}

void setPulseLED(bool value)
{
    pulseLED = value;
}

#endif

#endif
