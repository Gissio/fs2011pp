/*
 * Rad Pro
 * STM32 Main module
 *
 * (C) 2022-2024 Gissio
 *
 * License: MIT
 */

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "adc.h"
#include "buzzer.h"
#include "comm.h"
#include "datalog.h"
#include "debug.h"
#include "display.h"
#include "events.h"
#include "flash.h"
#include "game.h"
#include "init.h"
#include "keyboard.h"
#include "led.h"
#include "measurements.h"
#include "menu.h"
#include "power.h"
#include "rng.h"
#include "rtc.h"
#include "settings.h"
#include "system.h"
#include "tube.h"
#include "vibrator.h"
#include "view.h"

#if SIMULATOR
bool updateSDLTicks();

static void simulateFrame(void)
{
    while (updateSDLTicks())
    {
#if defined(GAME)
        dispatchGameEvents();
#endif
        dispatchEvents();
    }
}
#endif

int main(void)
{
    // System initialization
    initSystem();
    initEvents();
    initFlash();
    initSettings();
    initPower();
    initComm();
    initADC();
    initTube();
    initKeyboard();
    initBuzzer();
    initDisplay();
#if defined(PULSE_LED)
    initLED();
#endif
#if defined(VIBRATOR)
    initVibrator();
#endif

#if defined(TEST_MODE)
    setPower(true);
    runTestMode();
#endif

    initMeasurements();
#if defined(GAME)
    initGame();
#endif
    initDatalog();

#if defined(KEYBOARD_WAIT_POWERON)
    sleep(1000);
#endif

    setPowerOnView();

    // Main loop
#if SIMULATOR
#if __EMSCRIPTEN__
    emscripten_set_main_loop(simulateFrame, 0, 1);
#else
    while (true)
        simulateFrame();
#endif
#else
    while (true)
    {
#if defined(GAME)
        dispatchGameEvents();
#endif
        dispatchEvents();

        sleep(1);
    }
#endif
}
