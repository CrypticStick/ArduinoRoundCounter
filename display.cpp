#include "display.h"

uSSD1331 Display::oled = uSSD1331(OLED_pin_cs_ss, OLED_pin_dc_rs, OLED_pin_res_rst);
    
menu Display::currentMenu = m_init;
menu Display::previousMenu = m_init;
int Display::selectionIndex = 0;
long Display::lastSecondsElapsed = -1;
bool Display::temperatureDrawn = false;

uint16_t Display::OLED_Color_Menu = OLED_Color_Red;
uint16_t Display::lastHueSelection = 0;

DateTime Display::tempDateTime = DateTime(__DATE__, __TIME__);

uint8_t Display::SegVal[] = {20, 20};
uint16_t Display::SegMem[] = {0, 0};
bool Display::segUpdating = false;

const uint8_t Display::segNum[10];
const uint8_t Display::segPos[7][2];

void Display::init() {
  oled.begin(10000000L);
  lastHueSelection = Storage::getMenuHue()/5;
  updateMenuColor();
  oled.setTextColor(OLED_Color_White);
}

void Display::setMenu(menu _menu) {
  currentMenu = _menu;
  oled.fillScreen(OLED_Color_Black);
  switch (_menu) {
  case m_main:
  {
    previousMenu = _menu;
    oled.setFont(&title6x12Capital);
    uint16_t accentColor = HueToRgb(Storage::getMenuHue()+120);
    oled.setTextColor(accentColor);
    oled.setCursor(4,4);
    oled.print(F("ROUND COUNTER"));
    for (uint8_t i = 0; i < 4; i++) {
      oled.drawRect(
        75 + ((i == 0) ? -9 : (i == 2) ? 3 : 0), 
        33 + ((i == 1) ? -9 : (i == 3) ? 3 : 0), 
        (i % 2) ? 2 : 8, 
        (i % 2) ? 8 : 2, 
        OLED_Color_White);
    }
    oled.setFont(&version6x8Bold);
    oled.setCursor(60,52);
    for (uint8_t i = 0; i < 6; i++) {
      oled.setTextColor(i % 2 ? OLED_Color_White : accentColor);
      oled.print(progVersion[i]);
    }
  }
    break;
  case m_counter:
  {
    temperatureDrawn = false;
    lastSecondsElapsed = -1;
    oled.drawRect(0, 48, SSD1331_WIDTH, 2, OLED_Color_White);
    if (Storage::getBarrelCount() == 2) {
      oled.drawRect(47, 0, 2, 48, OLED_Color_White);
    }
    if (Storage::rtcExists()) {
      temperatureDrawn = true;
      updateTemperature(false);
    }
    SegMem[0] = 0;
    SegMem[1] = 0;
  }
    break;
  case m_matchStats:
  {
    oled.setFont(&default5x6All);
    oled.setTextColor(OLED_Color_Menu);
    drawTitle("STATS");
    oled.setTextColor(OLED_Color_Cyan);
    printCentered("Coming Soon");
    if (Storage::sdCardExists()) {
      //coming soon
    } else {
      //coming soon
    }
  }
    break;
  case m_settings:
  {
    previousMenu = _menu;
    oled.setFont(&default5x6All);
    oled.setTextColor(OLED_Color_Menu);
    drawTitle("SETTINGS");
  }
    break;
  case pop_barrels:
    oled.setTextColor(OLED_Color_Menu);
    drawPopup("BARREL COUNT", false);
    break;
  case pop_date:
    oled.setTextColor(OLED_Color_Menu);
    drawPopup("SET DATE", false);
    break;
  case pop_time:
    oled.setTextColor(OLED_Color_Menu);
    drawPopup("SET TIME", false);
    break;
  case pop_temp:
    oled.setTextColor(OLED_Color_Menu);
    drawPopup("TEMP. TYPE", false);
    break;
  case pop_menuColor:
  {
    lastHueSelection = Storage::getMenuHue()/5;
    oled.setTextColor(OLED_Color_Menu);
    drawPopup("MENU COLOR", false);
    int16_t left = (SSD1331_WIDTH - 64) / 2;
    int16_t top = SSD1331_HEIGHT / 2 - 1;
    for (int i = 0; i < 64; i++) {
      oled.drawFastVLine(left + i, top, 6, HueToRgb(i * 5));
    }
    drawHueSelector();
  }
    break;
  case pop_clear:
    oled.setTextColor(OLED_Color_Menu);
    drawPopup("WIPE MEMORY?", false);
    break;
  case pop_success:
    oled.setTextColor(OLED_Color_Green);
    drawPopup("Success!", true);
    break;
  case pop_no_rtc:
    oled.setTextColor(OLED_Color_Red);
    drawPopup("No RTC Found", true);
    break;
  default:
    break;
  }
  setSelection(0);
}

void Display::setPrevMenu(menu _menu) {
  previousMenu = _menu;
}

void Display::drawSelection(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint16_t color, bool stylize) {
  oled.fillRect(x, y, width, height, color);
  if (stylize) {
    oled.fillTriangle(x+width, y+1, x+width, y+height-1, x+width+(height-2)/2, y+height/2, color);
  }
}

void Display::drawOptions(int index, uint8_t numOptions, const char* options[], uint8_t y, bool vert, bool largePad) {
  uint16_t textWidth = 0;
  for (uint8_t i = 0; i < numOptions; i++) {
    textWidth += oled.getTextWidth(options[i]) + (largePad ? 4 : 2);
  }
  uint8_t spacing = ((SSD1331_WIDTH - ((int)currentMenu > 5 ? popup_margin * 2 + 2: 0)) - textWidth) / (numOptions + 1);
  uint8_t pastSpace = 0;
  index = (index > numOptions - 1) ? 0 : ((index < 0) ? numOptions - 1 : index);
  selectionIndex = index;

  if (largePad) {
    oled.setFont(&menu8x11Capital);
  } else {
    oled.setFont(&default5x6All);
  }
    
  for (uint8_t i = 0; i < numOptions; i++) {
    
    //TEMPORARY!!
    if (options[i] == "STATS") {
      oled.setTextColor(OLED_Color_Gray);
    } else {
      oled.setTextColor(OLED_Color_White);
    }
      
    drawSelection(
      vert ? 0 : (((int)currentMenu > 5 ? popup_margin + 1 : 0) + spacing * (i + 1) + pastSpace), 
      y + i * (vert ? (largePad ? 17 : 8) : 0), 
      oled.getTextWidth(options[i]) + (largePad ? 4 : 2), 
      largePad ? 15 : 8, 
      (i == index) ? OLED_Color_Select : OLED_Color_Black, 
      vert
      );
    oled.setCursor(
      (largePad ? 2 : 1) + (vert ? 0 : (((int)currentMenu > 5 ? popup_margin + 1 : 0) + spacing * (i + 1) + pastSpace)), 
      y + (largePad ? 2 : 1) + i * (vert ? (largePad ? 17 : 8) : 0)
      );
    oled.print(options[i]);
    pastSpace += oled.getTextWidth(options[i]) + (largePad ? 4 : 2);
  }
}

void Display::printCentered(const char str[], int y) {
  oled.setCursor((SSD1331_WIDTH - oled.getTextWidth(str)) / 2, (y < 0) ? (SSD1331_HEIGHT - 6 ) / 2 : y);
  oled.print(str);
}

void Display::drawPopup(const char title[], bool vCenter) {
  oled.drawRect(popup_margin, popup_margin, SSD1331_WIDTH - popup_margin * 2, SSD1331_HEIGHT - popup_margin * 2, OLED_Color_White);
  oled.drawRect(popup_margin + 1, popup_margin + 1, SSD1331_WIDTH - popup_margin * 2 - 2, SSD1331_HEIGHT - popup_margin * 2 - 2, OLED_Color_Black);
  oled.setFont(&default5x6All);
  printCentered(title, vCenter ? -1 : 15);
}

void Display::drawTitle(const char title[]) {
  printCentered(title, 2);
}

void Display::setSelection(int index) {
  switch (currentMenu) {
  case m_main:
  {
    const char* mainOptions[] = {"START", "STATS"};
    drawOptions(index, 2, mainOptions, 30, true, true);
  }
    break;
  case m_matchStats:
  {
    const char* matchStatsOptions[] = {"Save", "Discard"};
    drawOptions(index, 2, matchStatsOptions, SSD1331_HEIGHT-8, false, false);
  }
    break;
  case m_settings:
  {
    const char* settingsOptions[] = {"BARREL COUNT", "DATE & TIME", "TEMP. PREF", "MENU COLOR", "WIPE MEMORY"};
    drawOptions(index, 5, settingsOptions, 12, true, false);
  }
    break;
  case pop_barrels:
  {
    const char* barrelOptions[] = {"One", "Two"};
    drawOptions(index, 2, barrelOptions, popup_option_y, false, false);
  }
    break;
  case pop_date:
  {
    //epic quadratic to save a few bytes lol
    for (uint8_t e = 0; e < 3; e++) {
      drawSelection(-3*e*e + 30*e + 17, popup_option_y, 25 - 6 * e, 8 + e % 2, (index == e) ? OLED_Color_Select : OLED_Color_Black, false);
    }
    oled.setFont(&default5x6All);
    oled.setTextColor(OLED_Color_White);
    oled.setCursor(18, popup_option_y + 1);
    char dateFormat[] = "YYYY MMM DD";
    tempDateTime.toString(dateFormat);
    oled.print(dateFormat);
    selectionIndex = index;
  }
    break;
  case pop_time:
  {
    for (uint8_t e = 0; e < 3; e++) {
      drawSelection(19 + 15 * e, popup_option_y, 13, 8, (index == e) ? OLED_Color_Select : OLED_Color_Black, false);
    }
    oled.fillRect(64, 38, 13, 8, OLED_Color_Black);
    oled.setFont(&default5x6All);
    oled.setTextColor(OLED_Color_White);
    oled.setCursor(20, popup_option_y + 1);
    char timeFormat[] = "hh:mm:ss AP";
    tempDateTime.toString(timeFormat);
    oled.print(timeFormat);
    selectionIndex = index;
  }
    break;
  case pop_temp:
  {
    const char* tempOptions[] = {"°F", "°C", "K"};
    drawOptions(index, 3, tempOptions, popup_option_y, false, false);
  }
    break;
  case pop_menuColor:
  {
    const char* colorOptions[] = {"Set"};
    drawOptions(index, 1, colorOptions, SSD1331_HEIGHT - 19, false, false);
  }
    break;
  case pop_clear:
  {
    const char* clearOptions[] = {"No", "Yes"};
    drawOptions(index, 2, clearOptions, popup_option_y, false, false);
  }
    break;
  default:
    break;
  }
}

int Display::getSelection() {
  return selectionIndex;
}

menu Display::getMenu() {
  return currentMenu;
}

menu Display::getPrevMenu() {
  return previousMenu;
}

void Display::setDateTime(DateTime dateTime) {
  tempDateTime = dateTime;
}

DateTime Display::getDateTime() {
  return tempDateTime;
}

void Display::updateTimer(unsigned long startMillis) {
  int secondsElapsed = (millis() - startMillis) / 1000;
  if (lastSecondsElapsed == secondsElapsed) {
    return;
  }
  oled.setFont(&menu8x11Capital);
  for (int i = 0; i < 2; i++) {
    if (lastSecondsElapsed == -1 && i == 0) {
      continue;
    }
    oled.setTextColor(i == 0 ? OLED_Color_Black : OLED_Color_White);
    int seconds = (i == 0 ? lastSecondsElapsed : secondsElapsed) % 60;
    unsigned long minutes = (i == 0 ? lastSecondsElapsed : secondsElapsed) / 60;
    int hours = minutes / 60; 
    int hourDigits = 0;
    while (hours != 0) {
      hours /= 10;
      hourDigits++;
    }
    hours = minutes / 60; 
    minutes %= 60;
    if (temperatureDrawn) {
      oled.setCursor(2, 52);
    } else {
      int hSize = 29 + (minutes > 9 ? 9 : 0) + (hours > 0 ? (3 + 9 * hourDigits) : 0);
      oled.setCursor((SSD1331_WIDTH - hSize) / 2, 52);
    }
  
    if (hours > 0) {
      oled.print(hours);
      oled.print(F(":"));
      if (minutes < 10) {
        oled.print(0);
      }
    }
    oled.print(minutes);
    oled.print(F(":"));
    if (seconds < 10) {
      oled.print(0);
    }
    oled.print(seconds);
  }
  lastSecondsElapsed = secondsElapsed;
}

void Display::updateTemperature(bool checkChange) {
  if (!temperatureDrawn || (checkChange && !Storage::temperatureChanged())) {
    return;
  }
  oled.fillRect(45, 52, 53, 11, OLED_Color_Black);
  oled.setFont(&menu8x11Capital);
  oled.setTextColor(OLED_Color_White);
  int temp = Storage::getTemperature();
  int hSize = 26 + (abs(temp) > 99 ? 18 : (abs(temp) > 9 ? 9 : 0)) + (temp < 0 ? 9 : 0) - (Storage::getTemperaturePref() == 2 ? 6 : 0);
  oled.setCursor(94 - hSize, 52);
  oled.print(temp);
  switch (Storage::getTemperaturePref()) {
  default:
  case 0:
    oled.print(F("°F"));
    break;
  case 1:
    oled.print(F("°C"));
    break;
  case 2:
    oled.print(F("K"));
  }
}

//hue is 0-360
uint16_t Display::HueToRgb(uint16_t hue)
{
  float r, g, b;

  // Convert HSV to RGB using Alternative Wikipedia formula
  r = 1 - (fmax(fmin(fmod(5.0+(hue/60.0), 6.0), fmin(4.0 - (fmod(5.0+(hue/60.0), 6.0)), 1.0)), 0.0));
  g = 1 - (fmax(fmin(fmod(3.0+(hue/60.0), 6.0), fmin(4.0 - (fmod(3.0+(hue/60.0), 6.0)), 1.0)), 0.0));
  b = 1 - (fmax(fmin(fmod(1.0+(hue/60.0), 6.0), fmin(4.0 - (fmod(1.0+(hue/60.0), 6.0)), 1.0)), 0.0));

  return (((int)(r* 31) << 11) | ((int)(g * 63) << 5) | ((int)(b * 31)));
}

void Display::moveHueSelector(bool forward) {
  if ((lastHueSelection < 64 && forward) || (lastHueSelection > 0 && !forward)) {
    int16_t left = (SSD1331_WIDTH - 64) / 2;
    int16_t top = SSD1331_HEIGHT / 2 - 1;
    for (uint8_t i = 0; i < 2; i++) {
      oled.drawFastVLine(left + lastHueSelection + i, top, 6, HueToRgb((lastHueSelection + i) * 5));
      oled.drawFastHLine(left + lastHueSelection, top + (i == 0 ? -1 : 6), 2, OLED_Color_Black);
    }
    lastHueSelection += forward ? 1 : -1;
    drawHueSelector();
    oled.setTextColor(HueToRgb(lastHueSelection * 5));
    oled.setFont(&default5x6All);
    printCentered("MENU COLOR", 15);
  }
}

void Display::drawHueSelector() {
  int16_t left = (SSD1331_WIDTH - 64) / 2;
  int16_t top = SSD1331_HEIGHT / 2 - 1;
  for (uint8_t i = 0; i < 2; i++) {
    oled.drawFastVLine(left + lastHueSelection + i, top - 1, 8, OLED_Color_White);
  }
}

uint8_t Display::getHueSelector() {
  return lastHueSelection;
}

void Display::updateMenuColor() {
  OLED_Color_Menu = HueToRgb(Storage::getMenuHue());
 }

void Display::drawSegmentNumber(uint8_t num, bool secondary) {

  if (SegVal[secondary] == 0 && num != 0) {
    oled.fillRect(secondary ? 53 : (Storage::getBarrelCount() == 2 ? 3 : 29), 5, 41, 40, OLED_Color_Black);
  } else if ((SegVal[secondary] > shot_threshold_warn && num <= shot_threshold_warn) ||
  (SegVal[secondary] > shot_threshold_crit && num <= shot_threshold_crit) ||
  (SegVal[secondary] < num)) {
    SegMem[secondary] = 0b1000000000000000;
  }
  SegVal[secondary] = num;
  
  segUpdating = true;

  uint8_t tens = (num / 10U) % 10;
  uint8_t ones = num % 10;

  uint16_t segColor = (num <= shot_threshold_crit) ? OLED_Color_Red : 
                      (num <= shot_threshold_warn) ? OLED_Color_Yellow : 
                      OLED_Color_Green;

  bool refresh = false;
  if (bitRead(SegMem[secondary], 15)) {
    refresh = true;
  }
  
  for(uint8_t i = 0; i < 16; i++) {
    bool currentSeg = bitRead(SegMem[secondary], i);
    bool targetSeg = num == 0 ? false : bitRead(segNum[i > 7 ? ones : tens], i % 8);
    if (currentSeg != targetSeg || refresh) {
      bool vert = i % 8 % 3;
      if (!segUpdating) {
        return;
      }
      oled.drawBitmap(segPos[i % 8][0] + (i > 7 ? 23 : 0) + (secondary ? 51 : (Storage::getBarrelCount() == 2 ? 1 : 27)), 
                      segPos[i % 8][1] + 4, vert ? vSeg_bmp : hSeg_bmp, vert ? 5 : 15, vert ? 15 : 5, 
                      targetSeg ? segColor :OLED_Color_Black );
      bitWrite(SegMem[secondary], i, targetSeg);
    }
  }

  if (num == 0) {
    oled.setFont(&title6x12Capital);
    oled.setTextColor(OLED_Color_Red);
    uint8_t hOffset = secondary ? 51 : (Storage::getBarrelCount() == 2 ? 1 : 27);
    for (uint8_t i = 0; i < 3; i++) {
      oled.setCursor(hOffset + 2, 5 + i * 14);
      oled.print(F("RELOAD"));
    }
  }
  segUpdating = false;
}
