#include "storage.h"
#include "inputs.h"

RTC_DS3231 Storage::rtc;
int Storage::lastTemp = 0;

SdFat32 Storage::sd_card;
File32 Storage::sd_matchFile;
bool Storage::sdCardFound;
bool Storage::reloadCard = true;

void Storage::init() {
  checkSdCard();
}

DateTime Storage::getDateTime() {
  if (rtc.lostPower()) {
    return DateTime(0, 0, 0, 0, 0, 0);
  } else {
    return rtc.now();
  }
}

int Storage::getTemperature() {
  switch (getTemperaturePref()) {
  default:
  case 0:
    lastTemp = round((rtc.getTemperature() * 9/5) + 32);
    break;
  case 1:
    lastTemp = round(rtc.getTemperature());
    break;
  case 2:
    lastTemp = round(rtc.getTemperature() + 273.15);
  }
  return lastTemp;
}

bool Storage::temperatureChanged() {
  int buf = lastTemp;
  return buf != getTemperature();
}

bool Storage::rtcExists() {
  return rtc.begin();
}

void Storage::setDateTime(DateTime dateTime) {
  rtc.adjust(dateTime);
}

uint8_t Storage::getBarrelCount() {
  return EEPROM.read(barrel_addr);
}

void Storage::setBarrelCount(uint8_t count) {
  EEPROM.write(barrel_addr, count);
}

uint8_t Storage::getTemperaturePref() {
  return EEPROM.read(tempPref_addr);
}

void Storage::setTemperaturePref(uint8_t preference) {
  EEPROM.write(tempPref_addr, preference);
}

uint16_t Storage::getMenuHue() {
  uint16_t hue = 0;
  EEPROM.get(menuHue_addr, hue);
  if (hue != constrain(hue, 0, 360)) {
    setMenuHue(0);
    return 0;
  }
  return hue;
}

void Storage::setMenuHue(uint16_t hue) {
  EEPROM.put(menuHue_addr, hue);
}

void Storage::wipeMemory() {
  for (uint16_t i = 0; i < EEPROM.length(); i++) {
    EEPROM.write(i, 0);
  }
}

bool Storage::checkSdCard() {
  if (!Inputs::isPressed(i_sd_detect, false) && sdCardFound) {
    Serial.println("Card removed!");
    sdCardFound = false;
  } else if (Inputs::isPressed(i_sd_detect, true)){
    Serial.println("Card inserted!");
    reloadCard = true;
  }
  if (reloadCard) {
    reloadCard = false;
    delay(250);
    sdCardFound = sd_card.begin(SD_CONFIG);
    Serial.print("Card found: ");
    Serial.println(sdCardFound);
  }
}

bool Storage::sdCardExists() {
  return sdCardFound;
}

void Storage::createMatchFile(DateTime dateTime) {
  if (sdCardFound) {
    char fileDir[23] = "/RND_STAT/YYYY/MM/DD/";
    char fileName[13] = "hh_mm_ss.csv";
    char dest[37];
    dateTime.toString(fileDir);
    dateTime.toString(fileName);
    if (!sd_card.exists(fileDir)) {
      sd_card.mkdir(fileDir, true);
    }
    strcpy(dest, fileDir);
    strcat(dest, fileName);
    sd_matchFile = sd_card.open(dest, O_WRONLY | O_CREAT);
    if (sd_matchFile.isOpen()) {
      sd_matchFile.println("TIMESTAMP,RELOAD,CHANNEL");
    }
  }
}

void Storage::closeMatchFile(long startMillis) {
  if (sdCardFound && sd_matchFile.isOpen()) {
    sd_matchFile.print(millis() - startMillis);
    sd_matchFile.print(",,");
    sd_matchFile.close();
  }
}

void Storage::removeMatchFile() {
  if (sdCardFound && sd_matchFile.isOpen()) {
    sd_matchFile.remove();
  }
}

void Storage::flushFile() {
  if (sdCardFound && sd_matchFile.isOpen()) {
    sd_matchFile.flush();
  }
}

void Storage::logData(Log* activity, long startMillis) {
  if (sdCardFound && sd_matchFile.isOpen()) {
    for (uint8_t i = 0; i < activity->size; i++) {
      sd_matchFile.print(activity->timestamps[i] - startMillis);
      sd_matchFile.print(',');
      sd_matchFile.print(activity->reloads[i]);
      sd_matchFile.print(',');
      sd_matchFile.println(activity->channels[i]);
    }
  }
}
