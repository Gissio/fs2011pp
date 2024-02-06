/*
 * Rad Pro
 * System
 *
 * (C) 2022-2023 Gissio
 *
 * License: MIT
 */

#if !defined(SYSTEM_H)

#define SYSTEM_H

#include <stdint.h>

#include "view.h"

#define FIRMWARE_NAME "Rad Pro"
#define FIRMWARE_VERSION "2.0beta12"

extern const struct View statisticsView;

void initSystem(void);

uint32_t getDeviceId(void);

void startBootloader(void);

#endif
