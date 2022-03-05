#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include <EEPROM.h>
#include <RTClib.h>
#include <SdFat.h>
#include <sdios.h>

/**
 * Responsible for managing configuration and history.
 */

static const uint8_t SD_CS_PIN = 14;
static const uint8_t SOFT_SCK_PIN = 15;
static const uint8_t SOFT_MOSI_PIN = 16;
static const uint8_t SOFT_MISO_PIN = 17;

// SdFat software SPI template
static SoftSpiDriver<SOFT_MISO_PIN, SOFT_MOSI_PIN, SOFT_SCK_PIN> sd_softSpi;
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SD_SCK_MHZ(0), &sd_softSpi)

#ifndef LOG_STRUCT
#define LOG_STRUCT
const uint8_t log_max = 16;

struct log {
  long timestamps[log_max] = {0};
  bool reloads[log_max] = {false};
  bool channels[log_max] = {false};
  uint8_t size = 0;
};

typedef struct log Log;
#endif

class Storage
{
private:

static SdFat32 sd_card;
static File32 sd_matchFile;
static bool sdCardFound;
static bool reloadCard;

/** the current address in the EEPROM (i.e. which byte we're going to write to next) **/
 static const int barrel_addr = 0;
 static const int tempPref_addr = 1;
 static const int menuHue_addr = 2;
 
 static bool rtcFound;
 static RTC_DS3231 rtc;
 static int lastTemp;


 static void checkCardChanged();

public:
  /** initializes storage */
  static void init();

  static DateTime getDateTime();

  static int getTemperature();

  static bool temperatureChanged();

  static bool rtcExists();

  static void setDateTime(DateTime dateTime);

  static uint8_t getBarrelCount();

  static void setBarrelCount(uint8_t count);

  static uint8_t getTemperaturePref();
  
  static void setTemperaturePref(uint8_t preference);

  static uint16_t getMenuHue();

  static void setMenuHue(uint16_t hue);

  static void wipeMemory();

  static bool checkSdCard();

  static bool sdCardExists();

  static void createMatchFile(DateTime dateTime);

  static void closeMatchFile(long startMillis);

  static void removeMatchFile();

  static void flushFile();

  static void logData(Log* activity, long startMillis);
};

#endif
