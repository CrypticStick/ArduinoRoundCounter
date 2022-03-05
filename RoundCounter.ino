/*
 * Main project file.
 */

#include "display.h"
#include "inputs.h"
#include "storage.h"
#define SerialDebugging true

static const uint8_t BUZZER_PIN = 6;  //Reserved pin; no code yet

uint8_t fullMagVal = 20;
uint8_t ammoVal[2] = { fullMagVal, fullMagVal };
bool updateSeg[2] = { false, false };

unsigned long startTime = 0;
//unsigned int totalShots = 0;
//unsigned int totalReloads = 0;
//unsigned int averageBurstRate = 0;
//unsigned int maxBurstRate = 0;
//unsigned int rollingAvg = 0;

uint8_t lastSec = 0;
const uint8_t monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

uint8_t initialization = 2;

void setup() {
  if (SerialDebugging) {
    Serial.begin(9600);
    while (!Serial) { delay(1); } // wait until serial console is open, remove if not tethered to computer
  }
  Inputs::init();
  Display::init();
  Storage::init();
  Inputs::enableBarrelTwo(Storage::getBarrelCount() == 2);
}

void loop() {
  Storage::checkSdCard();
  Inputs::update();
  //reload check
  for (uint8_t i = 0; i < 2; i++) {
    if (Inputs::isPressed((input)(i_reload_1 + i), true)) {
      if (ammoVal[i] != fullMagVal) {
        ammoVal[i] = fullMagVal;
        Inputs::logActivity(true, i);
        updateSeg[i] = true;
      }
    }
  }

  //shot check
  Log *actLog = Inputs::getActivityLog();
  if (actLog->size != 0) {
    for (uint8_t i = 0; i < actLog->size; i++) {
      if (!(actLog->reloads[i])) {
        bool channel = actLog->channels[i];
        if (ammoVal[channel] != 0) {
          ammoVal[channel]--;
          updateSeg[channel] = true;
        }
      }
    }
    Log buf = *actLog;
    actLog->size = 0;
    if (Display::getMenu() == m_counter) {
      Storage::logData(&buf, startTime);
    } 
  }

  //config for first-time boot
  if (Display::getMenu() == m_init) {
    switch (initialization--) {
    case 2:
      if (Storage::getBarrelCount() == 0) {
        Storage::setBarrelCount(1);
        Display::setMenu(pop_barrels);
      }
      break;
    case 1:
      Display::setPrevMenu(m_main);
      if (Storage::rtcExists()) {
        if (!Storage::getDateTime().isValid()) {
          Display::setMenu(pop_date);
        } else {
          Display::setMenu(m_main);
        }
      } else {
        Display::setMenu(pop_no_rtc);
      }
      break;
    default:
      break;
    }
  }

  // navigation
  if (Display::getMenu() != pop_date && Display::getMenu() != pop_time) {
    if (Inputs::isPressed(i_down, true)) {
      Display::setSelection(Display::getSelection() + 1);
    } else if (Inputs::isPressed(i_up, true)) {
      Display::setSelection(Display::getSelection() - 1);
    }
  }

  // menu navigation & control
  switch (Display::getMenu()) {
  case m_main:
  {
    if (Inputs::isPressed(i_select, true)) {
      if (Display::getSelection() == 0) {
        Inputs::getActivityLog()->size = 0;
        updateSeg[0] = true;
        updateSeg[1] = true;
        if (Storage::sdCardExists()) {
          Storage::createMatchFile(Storage::getDateTime());
        }
        Display::setMenu(m_counter);
        startTime = millis();
        Display::updateTimer(startTime);
      }
    }
    if (Inputs::isPressed(i_back, true)) {
      Display::setMenu(m_settings);
    }
  }
    break;
  case m_counter:
  {
    Display::updateTimer(startTime);
    Display::updateTemperature(true);
    if (Inputs::isPressed(i_back, true)) {
      Storage::flushFile();
      Display::setMenu(m_matchStats);
    }
  }
    break;
  case m_matchStats:
  {
    if (Inputs::isPressed(i_back, true)) {
      Display::setMenu(m_main);
    }
    if (Inputs::isPressed(i_select, true)) {
      if (Display::getSelection() == 0) {
        Storage::closeMatchFile(startTime);
      } else {
        Storage::removeMatchFile();
      }
      Display::setMenu(m_main);
    }
  }
    break;
  case m_settings:
  {
    if (Inputs::isPressed(i_back, true)) {
      Display::setMenu(m_main);
    }
    if (Inputs::isPressed(i_select, true)) {
      if (Display::getSelection() == 0) {
        Display::setMenu(pop_barrels);
      } else if (Display::getSelection() == 1) {
        if (Storage::rtcExists()) {
          if (Storage::getDateTime().isValid()) {
            Display::setDateTime(Storage::getDateTime());
          }
          Display::setMenu(pop_date);
        } else {
          Display::setMenu(pop_no_rtc);
        }
      } else if (Display::getSelection() == 2) {
        Display::setMenu(pop_temp);
      } else if (Display::getSelection() == 3) {
        Display::setMenu(pop_menuColor);
      } else {
        Display::setMenu(pop_clear);
      }
    }
  }
    break;
  case pop_barrels:
  {
    if (Inputs::isPressed(i_back, true)) {
      Display::setMenu(Display::getPrevMenu());
    }
    if (Inputs::isPressed(i_select, true)) {
      if (Display::getSelection() == 0) {
        Storage::setBarrelCount(1);
      } else {
        Storage::setBarrelCount(2);
      }
      Inputs::enableBarrelTwo(Display::getSelection());
      Display::setMenu(pop_success);
    }
  }
    break;
  case pop_date:
  {
    int scrollDir = Inputs::isPressed(i_up, true) ? 1 : Inputs::isPressed(i_down, true) ? -1 : 0;
    if (scrollDir != 0) {
      startTime = millis();
      DateTime current = Display::getDateTime();
      bool isLeapYear = (!(current.year() % 4) && (current.year() % 100)) || !(current.year() % 400);
      uint16_t newYear = current.year() + (Display::getSelection() == 0 ? scrollDir : 0);
      uint8_t newMonth = current.month() + (Display::getSelection() == 1 ? scrollDir : 0);
      newMonth = (newMonth < 1) ? 12 : (newMonth > 12) ? 1 : newMonth;
      uint8_t newDay = current.day() + (Display::getSelection() == 2 ? scrollDir : 0);
      newDay =  (newDay < 1) ? monthDays[newMonth-1] + (newMonth == 2 && isLeapYear) : 
                (newDay > monthDays[newMonth-1] + (newMonth == 2 && isLeapYear)) ? 1 : newDay;
      Display::setDateTime(
        DateTime( newYear, newMonth, newDay, current.hour(), current.minute(), current.second())
        );
      Display::setSelection(Display::getSelection());
    }
    if (Inputs::isPressed(i_select, true)) {
      if (Display::getSelection() + 1 > 2) {
        DateTime stored = Storage::getDateTime();
        if (stored.isValid()) {
          DateTime current = Display::getDateTime();
          Display::setDateTime(
            DateTime( current.year(), current.month(), current.day(), stored.hour(), stored.minute(), stored.second())
            );
          lastSec = stored.second();
        }
        Display::setMenu(pop_time);
      } else {
        Display::setSelection(Display::getSelection() + 1);
      }
    }
    if (Inputs::isPressed(i_back, true)) {
      if (Display::getSelection() - 1 < 0) {
        Display::setMenu(Display::getPrevMenu());
      } else {
        Display::setSelection(Display::getSelection() - 1);
      }
    }
  }
    break;
  case pop_time:
  {
    if (Storage::getDateTime().isValid()) {
      if (lastSec != Storage::getDateTime().second()) {
        lastSec = Storage::getDateTime().second();
        DateTime current = Display::getDateTime();
        Display::setDateTime(DateTime(current.unixtime()+1));
        Display::setSelection(Display::getSelection());
      }
    }
    int scrollDir = Inputs::isPressed(i_up, true) ? 1 : Inputs::isPressed(i_down, true) ? -1 : 0;
    if (scrollDir != 0) {
      startTime = millis();
      DateTime current = Display::getDateTime();
      uint8_t newHour = current.hour() + (Display::getSelection() == 0 ? scrollDir : 0);
      newHour = (newHour > 24) ? 23 : (newHour > 23) ? 0 : newHour;
      uint8_t newMinute = current.minute() + (Display::getSelection() == 1 ? scrollDir : 0);
      newMinute = (newMinute > 60) ? 59 : (newMinute > 59) ? 0 : newMinute;
      uint8_t newSecond = current.second() + (Display::getSelection() == 2 ? scrollDir : 0);
      newSecond = (newSecond > 60) ? 59 : (newSecond > 59) ? 0 : newSecond;
      Display::setDateTime(
        DateTime(current.year(), current.month(), current.day(), newHour, newMinute, newSecond)
        );
      Display::setSelection(Display::getSelection());
    }
    if (Inputs::isPressed(i_select, true)) {
      if (Display::getSelection() + 1 > 2) {
        Storage::setDateTime(Display::getDateTime());
        Display::setMenu(pop_success);
      } else {
        Display::setSelection(Display::getSelection() + 1);
      }
    }
    if (Inputs::isPressed(i_back, true)) {
      if (Display::getSelection() - 1 < 0) {
        Display::setMenu(pop_date);
      } else {
        Display::setSelection(Display::getSelection() - 1);
      }
    }
  }
    break;
  case pop_temp:
  {
    if (Inputs::isPressed(i_back, true)) {
      Display::setMenu(Display::getPrevMenu());
    }
    if (Inputs::isPressed(i_select, true)) {
      Storage::setTemperaturePref(Display::getSelection());
      Display::setMenu(pop_success);
    }
  }
    break;
  case pop_menuColor:  
  {
    if (Inputs::isPressed(i_up, false)) {
      Display::moveHueSelector(true);
    } else if (Inputs::isPressed(i_down, false)) {
      Display::moveHueSelector(false);
    }
    if (Inputs::isPressed(i_back, true)) {
      Display::setMenu(Display::getPrevMenu());
    }
    if (Inputs::isPressed(i_select, true)) {
      Storage::setMenuHue(Display::getHueSelector()*5);
      Display::updateMenuColor();
      Display::setMenu(pop_success);
    }
  }
    break;
  case pop_clear:
  {
    if (Inputs::isPressed(i_back, true)) {
      Display::setMenu(Display::getPrevMenu());
    }
    if (Inputs::isPressed(i_select, true)) {
      if (Display::getSelection() == 0) {
        Display::setMenu(Display::getPrevMenu());
      } else {
        Storage::wipeMemory();
        Display::updateMenuColor();
        Display::setMenu(pop_success);
      }
    }
  }
    break;
  case pop_success:
  case pop_no_rtc:
  {
    if (Inputs::isPressed(i_select, true) || Inputs::isPressed(i_back, true)) {
      Display::setMenu(Display::getPrevMenu());
    }
  }
    break;
  default:
    break;
  }

  // update segmented counter
  if (Display::getMenu() == m_counter) {
    for (uint8_t i = 0; i < Storage::getBarrelCount(); i++) {
      if (updateSeg[i]) {
        Display::drawSegmentNumber(ammoVal[i], i);
        updateSeg[i] = false;
      }
    }
  }
}
