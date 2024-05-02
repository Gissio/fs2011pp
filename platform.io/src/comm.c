/*
 * Rad Pro
 * Communications
 *
 * (C) 2022-2024 Gissio
 *
 * License: MIT
 */

#include <stdbool.h>

#include <stdio.h>

#include "adc.h"
#include "comm.h"
#include "cstring.h"
#include "display.h"
#include "events.h"
#include "measurements.h"
#include "rng.h"
#include "rtc.h"
#include "system.h"
#include "tube.h"

Comm comm;

#if defined(DATA_MODE)
static const Menu dataModeMenu;
#endif

void initComm(void)
{
#if !defined(DATA_MODE)
    startComm();
#else
    selectMenuItem(&dataModeMenu,
                   0,
                   2);
#endif
}

void setCommEnabled(bool value)
{
    comm.enabled = value;
}

static bool matchCommCommand(const char *command)
{
    uint32_t index = 0;

    while (true)
    {
        char c1 = comm.buffer[index];
        char c2 = command[index];
        index++;

        if (c2 == '\0')
            return (c1 <= ' ');

        if (c1 != c2)
            return false;
    }
}

static bool matchCommCommandWithUInt32(const char *command, uint32_t *value)
{
    if (!matchCommCommand(command))
        return false;

    return (strlen(comm.buffer) > (strlen(command) + 1)) &&
           parseUInt32(comm.buffer + strlen(command) + 1, value);
}

static void sendCommOk(void)
{
    strcpy(comm.buffer, "OK");
}

static void sendCommOkWithString(const char *value)
{
    sendCommOk();
    strcatChar(comm.buffer, ' ');
    strcat(comm.buffer, value);
}

static void sendCommOkWithUInt32(uint32_t value)
{
    sendCommOk();
    strcatChar(comm.buffer, ' ');
    strcatUInt32(comm.buffer, value, 0);
}

static void sendCommOkWithFloat(float value, uint32_t fractionalDecimals)
{
    sendCommOk();
    strcatChar(comm.buffer, ' ');
    strcatFloat(comm.buffer, value, fractionalDecimals);
}

static void sendCommError(void)
{
    strcpy(comm.buffer, "ERROR");
}

static void strcatDatalogEntry(char *buffer, const Dose *entry)
{
    strcatUInt32(buffer, entry->time, 0);
    strcatChar(comm.buffer, ',');
    strcatUInt32(buffer, entry->pulseCount, 0);
}

static void startDatalogDump(void)
{
    sendCommOk();

    strcat(comm.buffer, " time,tubePulseCount");

    startDatalogDownload();
    comm.sendingDatalog = true;

    transmitComm();
}

void dispatchCommEvents(void)
{
    updateCommController();

    if (comm.state == COMM_RX_READY)
    {
        uint32_t value;

        if (matchCommCommand("GET deviceId"))
        {
            sendCommOkWithString(commId);
            strcatChar(comm.buffer, ';');
            strcatUInt32Hex(comm.buffer, getDeviceId());
        }
        else if (matchCommCommand("GET deviceBatteryVoltage"))
            sendCommOkWithFloat(getDeviceBatteryVoltage(), 3);
        else if (matchCommCommand("GET deviceTime"))
            sendCommOkWithUInt32(getDeviceTime());
        else if (matchCommCommandWithUInt32("SET deviceTime", &value))
        {
            setDeviceTime(value);

            sendCommOk();
        }
        else if (matchCommCommand("GET tubeTime"))
            sendCommOkWithUInt32(getTubeTime());
        else if (matchCommCommandWithUInt32("SET tubeTime", &value))
        {
            setTubeTime(value);

            sendCommOk();
        }
        else if (matchCommCommand("GET tubePulseCount"))
            sendCommOkWithUInt32(getTubePulseCount());
        else if (matchCommCommandWithUInt32("SET tubePulseCount", &value))
        {
            setTubePulseCount(value);

            sendCommOk();
        }
        else if (matchCommCommand("GET tubeRate"))
            sendCommOkWithFloat(60.0F * getInstantaneousRate(), 3);
        else if (matchCommCommand("GET tubeDeadTime"))
            sendCommOkWithFloat(getTubeDeadTime(), 7);
        else if (matchCommCommand("GET tubeConversionFactor"))
            sendCommOkWithFloat(getTubeConversionFactor(), 3);
        else if (matchCommCommand("GET tubeDeadTimeCompensation"))
            sendCommOkWithFloat(getTubeDeadTimeCompensation(), 7);
        else if (matchCommCommand("GET tubeBackgroundCompensation"))
            sendCommOkWithFloat(60.0F * getTubeBackgroundCompensation(), 3);
        else if (matchCommCommand("GET tubeHVFrequency"))
            sendCommOkWithFloat(getTubeHVFrequency(), 2);
        else if (matchCommCommand("GET tubeHVDutyCycle"))
            sendCommOkWithFloat(getTubeHVDutyCycle(), 4);
        else if (matchCommCommandWithUInt32("GET datalog", &comm.datalogTimeLimit))
        {
            startDatalogDump();

            return;
        }
        else if (matchCommCommand("GET datalog"))
        {
            comm.datalogTimeLimit = 0;

            startDatalogDump();

            return;
        }
        else if (matchCommCommand("GET randomData"))
        {
            sendCommOk();

            for (int i = 0; i < 16; i++)
            {
                int32_t randomData = getRandomData();
                if (randomData < 0)
                    break;

                if (i == 0)
                    strcatChar(comm.buffer, ' ');

                strcatUInt8Hex(comm.buffer, randomData);
            }
        }
#if defined(START_BOOTLOADER_SUPPORT)
        else if (matchCommCommand("START bootloader"))
        {
            comm.startBootloader = true;

            sendCommOk();
        }
#endif
        else
            sendCommError();

        strcatChar(comm.buffer, '\n');

        transmitComm();
    }
    else if (comm.state == COMM_TX_READY)
    {
        if (comm.sendingDatalog)
        {
            strclr(comm.buffer);

            uint32_t i = 0;
            while (i < 2)
            {
                Dose dose;

                if (!getDatalogDownloadEntry(&dose))
                {
                    strcatChar(comm.buffer, '\n');

                    comm.sendingDatalog = false;

                    break;
                }

                if (dose.time >= comm.datalogTimeLimit)
                {
                    strcatChar(comm.buffer, ';');
                    strcatDatalogEntry(comm.buffer, &dose);

                    i++;
                }
            }

            transmitComm();
        }
        else
        {
#if defined(START_BOOTLOADER_SUPPORT)
            if (comm.startBootloader)
                startBootloader();
#endif

            comm.state = COMM_RX;
        }
    }
}

// Communications mode

#if defined(DATA_MODE)

static const char *const dataModeMenuOptions[] = {
    "Off",
    "On",
    NULL,
};

static const char *onDataModeMenuGetOption(const Menu *menu,
                                           uint32_t index,
                                           MenuStyle *menuStyle)
{
    *menuStyle = (index == isCommStarted());

    return dataModeMenuOptions[index];
}

static void onDataModeMenuSelect(const Menu *menu)
{
    if (menu->state->selectedIndex)
        startComm();
    else
        stopComm();
}

static MenuState dataModeMenuState;

static const Menu dataModeMenu = {
    "Data Mode",
    &dataModeMenuState,
    onDataModeMenuGetOption,
    onDataModeMenuSelect,
    onSettingsSubMenuBack,
};

const View dataModeMenuView = {
    onMenuEvent,
    &dataModeMenu,
};

#endif
