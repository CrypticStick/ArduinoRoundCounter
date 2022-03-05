#include "inputs.h"

Adafruit_MCP23X17 Inputs::mcp;
const uint8_t Inputs::irPins[2];
uint8_t Inputs::stateBuf = 0;
uint8_t Inputs::risenBuf = 0;
bool Inputs::inputChanged = false;

Log Inputs::activity_log;

void Inputs::logIR1() {
  if (digitalRead(irPins[0])) {
    logActivity(false, false);
  }
}

void Inputs::logIR2() {
  if (digitalRead(irPins[1])) {
    logActivity(false, true);
  }
}

void Inputs::logActivity(bool reload, bool channel) {
  if (activity_log.size < log_max) {
    uint8_t index = (++activity_log.size)-1;
    activity_log.timestamps[index] = millis();
    activity_log.reloads[index] = reload;
    activity_log.channels[index] = channel;
  }
}

void Inputs::init() {
  if (!mcp.begin_I2C()) {
    Serial.println("I2C MCP Error.");
    while (1);
  }
  pinMode(INTA_PIN, INPUT);
  pinMode(INTB_PIN, INPUT);
  mcp.setupInterrupts(false, false, LOW);
  pinMode(irPins[0], INPUT);
  pinMode(irPins[1], INPUT);
  attachInterrupt(digitalPinToInterrupt(INTA_PIN), setInputChanged, FALLING);
  mcp.getLastInterruptPin(); // clear any potential low state
  attachInterrupt(digitalPinToInterrupt(irPins[0]), logIR1, CHANGE);
  for (uint8_t i = 1; i < 8; i++) {
    mcp.pinMode(i, INPUT_PULLUP);
    mcp.setupInterruptPin(i, CHANGE);
  }
  update(1); //get SD card state
}

void Inputs::setInputChanged() {
  inputChanged = true;
}

void Inputs::update(uint8_t pin) {
  if (inputChanged || pin != 255) {
    uint8_t mcpPin = (pin != 255) ? pin : mcp.getLastInterruptPin();
    inputChanged = false;
    if (mcpPin != 255) {
      uint8_t mask = (1 << mcpPin);
      if (mcp.digitalRead(mcpPin)) {
        stateBuf &= ~mask;
        risenBuf &= ~mask;
      } else if (!(stateBuf & mask)) {
        stateBuf |= mask;
        risenBuf |= mask;
      } 
    }
  }
}

bool Inputs::isPressed(input in, bool risingOnly) {
  if (risingOnly) {
    if (risenBuf & (1 << in)) {
      risenBuf &= ~(1 << in);
      return true;
    }
    return false;
  }
  return stateBuf & (1 << in);
}

void Inputs::enableBarrelTwo(bool enable) {
  if (enable) {
    attachInterrupt(digitalPinToInterrupt(irPins[1]), logIR2, CHANGE);
  } else {
    detachInterrupt(digitalPinToInterrupt(irPins[1]));
  }
}

Log *Inputs::getActivityLog() {
  return &activity_log;
}
