#ifndef INPUTS_H
#define INPUTS_H

#include <Arduino.h>
#include <Adafruit_MCP23X17.h>

/**
 * Responsible for managing buttons and potentiometers.
 */

 enum input {
  i_RESERVED = 0,
  i_sd_detect,
  i_down,
  i_up,
  i_back,
  i_select,
  i_reload_2,
  i_reload_1
};

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

class Inputs
{
private:

static Adafruit_MCP23X17 mcp;
static const uint8_t INTA_PIN = 4;
static const uint8_t INTB_PIN = 5;

static constexpr const uint8_t irPins[2] = {2, 3};

  /*
   * stateBuf & risenBuf contains one bit per input (using MCP pins):
   * 0: RESERVED
   * 1: sd_detect
   * 2: down
   * 3: up
   * 4: back
   * 5: select
   * 6: reload_2
   * 7: reload_1
   */
   
  static uint8_t stateBuf;
  static uint8_t risenBuf;
  static bool inputChanged;

  static Log activity_log;
  static void logIR1();
  static void logIR2();
  static void setInputChanged();
  
public:
  /** initializes inputs */
  static void init();

  static void update(uint8_t pin = 255);

  static bool isPressed(input in, bool risingOnly);

  static void enableBarrelTwo(bool enable);

  static void logActivity(bool reload, bool channel);

  /** returns the current log instance **/
  static Log *getActivityLog();
};

#endif
