#ifndef DISPLAY_H
#define DISPLAY_H

#include <uSSD1331.h>
#include <stdarg.h>
#include <SPI.h>
#include "storage.h"

// 'hSeg, 15x5px
const unsigned char hSeg_bmp [] PROGMEM = {
  0x3f, 0xf8, 0x7f, 0xfc, 0xff, 0xfe, 0x7f, 0xfc, 0x3f, 0xf8
};

// 'vSeg, 5x15px
const unsigned char vSeg_bmp [] PROGMEM = {
  0x20, 0x70, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0x70, 0x20
};

/**
 * Responsible for managing the display.
 */

enum menu {
  m_init = 0,
  m_main,
  m_counter,
  m_matchStats,
  m_genStats,
  m_settings,
  pop_success,
  pop_no_rtc,
  pop_barrels,
  pop_date,
  pop_time,
  pop_temp,
  pop_menuColor,
  pop_clear
};

class Display
{
private:
  // The SSD1331 is connected like this (plus VCC plus GND)
  static const uint8_t   OLED_pin_cs_ss          = 10;
  static const uint8_t   OLED_pin_res_rst        = 9;
  static const uint8_t   OLED_pin_dc_rs          = 8;
    
  // SSD1331 color definitions
  static const uint16_t  OLED_Color_Black        = 0x0000;
  static const uint16_t  OLED_Color_Blue         = 0x001F;
  static const uint16_t  OLED_Color_Red          = 0xF800;
  static const uint16_t  OLED_Color_Green        = 0x07E0;
  static const uint16_t  OLED_Color_Cyan         = 0x07FF;
  static const uint16_t  OLED_Color_Magenta      = 0xF81F;
  static const uint16_t  OLED_Color_Yellow       = 0xFFE0;
  static const uint16_t  OLED_Color_White        = 0xFFFF;
  static const uint16_t  OLED_Color_Gray         = 0x4208;
  static const uint16_t  OLED_Color_Select       = 0x7BC0;
  static uint16_t OLED_Color_Menu;
  static uint16_t lastHueSelection; //ranges 0-64 (actual hue is 5x this)

  static const uint8_t shot_threshold_warn       = 12;
  static const uint8_t shot_threshold_crit       = 6;

  static const uint8_t popup_margin              = 6;
  static const uint8_t popup_option_y            = 36;

  static constexpr const char* progVersion = "v0.5.0";  //only supports single-digit numbers

  // right-most bit is top segment
  static constexpr const uint8_t segNum[10] = 
  { 0b01110111, 0b00100100, 0b01011101, 0b01101101, 
    0b00101110, 0b01101011, 0b01111011, 0b00100101, 
    0b01111111, 0b01101111 };

  static constexpr const uint8_t segPos[7][2] = 
  { {3,0}, {0,4}, {16,4}, {3,18}, {0,22}, {16,22}, {3,36} };

  // declare the display
  static uSSD1331 oled;
  static menu previousMenu;
  static menu currentMenu;
  static int selectionIndex;
  static long lastSecondsElapsed;
  static bool temperatureDrawn;
  static DateTime tempDateTime;

  static uint8_t SegVal[2];
  static uint16_t SegMem[2];
  static bool segUpdating;

  static void drawSelection(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint16_t color, bool stylize);
  static void drawOptions(int index, uint8_t numOptions, const char* options[], uint8_t y, bool vert, bool largePad);
  static void printCentered(const char str[], int y = -1);
  static void drawPopup(const char title[], bool vCenter);
  static void drawTitle(const char title[]);

public:
  /** initializes display */
  static void init();

  static void setMenu(menu menu);

  static void setPrevMenu(menu menu);

  static void setSelection(int index);

  static int getSelection();

  static menu getMenu();

  static menu getPrevMenu();

  static void setDateTime(DateTime dateTime);

  static DateTime getDateTime();

  static void updateTimer(unsigned long startMillis);

  static void updateTemperature(bool checkChange);

  static void drawSegmentNumber(uint8_t num, bool secondary);

  static void moveHueSelector(bool forward);

  static void drawHueSelector();

  static uint8_t getHueSelector();

  static void updateMenuColor();

  static uint16_t HueToRgb(uint16_t hue);
};

#endif
