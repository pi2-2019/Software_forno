// Copyright 2017 Ville Skyttä (scop)
// Copyright 2017, 2018 David Conran
//
// Code to emulate Gree protocol compatible HVAC devices.
// Should be compatible with:
// * Heat pumps carrying the "Ultimate" brand name.
// * EKOKAI air conditioners.
//

#include "ir_Gree.h"
#include <algorithm>
#ifndef ARDUINO
#include <string>
#endif
#include "IRrecv.h"
#include "IRremoteESP8266.h"
#include "IRsend.h"
#include "IRutils.h"
#include "ir_Kelvinator.h"

// Constants
// Ref: https://github.com/ToniA/arduino-heatpumpir/blob/master/GreeHeatpumpIR.h
const uint16_t kGreeHdrMark = 9000;
const uint16_t kGreeHdrSpace = 4500;  // See #684 and real example in unit tests
const uint16_t kGreeBitMark = 620;
const uint16_t kGreeOneSpace = 1600;
const uint16_t kGreeZeroSpace = 540;
const uint16_t kGreeMsgSpace = 19000;
const uint8_t kGreeBlockFooter = 0b010;
const uint8_t kGreeBlockFooterBits = 3;

#if SEND_GREE
// Send a Gree Heat Pump message.
//
// Args:
//   data: An array of bytes containing the IR command.
//   nbytes: Nr. of bytes of data in the array. (>=kGreeStateLength)
//   repeat: Nr. of times the message is to be repeated. (Default = 0).
//
// Status: ALPHA / Untested.
//
// Ref:
//   https://github.com/ToniA/arduino-heatpumpir/blob/master/GreeHeatpumpIR.cpp
void IRsend::sendGree(const unsigned char data[], const uint16_t nbytes,
                      const uint16_t repeat) {
  if (nbytes < kGreeStateLength)
    return;  // Not enough bytes to send a proper message.

  for (uint16_t r = 0; r <= repeat; r++) {
    // Block #1
    sendGeneric(kGreeHdrMark, kGreeHdrSpace, kGreeBitMark, kGreeOneSpace,
                kGreeBitMark, kGreeZeroSpace, 0, 0,  // No Footer.
                data, 4, 38, false, 0, 50);
    // Footer #1
    sendGeneric(0, 0,  // No Header
                kGreeBitMark, kGreeOneSpace, kGreeBitMark, kGreeZeroSpace,
                kGreeBitMark, kGreeMsgSpace, 0b010, 3, 38, false, 0, 50);

    // Block #2
    sendGeneric(0, 0,  // No Header for Block #2
                kGreeBitMark, kGreeOneSpace, kGreeBitMark, kGreeZeroSpace,
                kGreeBitMark, kGreeMsgSpace, data + 4, nbytes - 4, 38, false, 0,
                50);
  }
}

// Send a Gree Heat Pump message.
//
// Args:
//   data: The raw message to be sent.
//   nbits: Nr. of bits of data in the message. (Default is kGreeBits)
//   repeat: Nr. of times the message is to be repeated. (Default = 0).
//
// Status: ALPHA / Untested.
//
// Ref:
//   https://github.com/ToniA/arduino-heatpumpir/blob/master/GreeHeatpumpIR.cpp
void IRsend::sendGree(const uint64_t data, const uint16_t nbits,
                      const uint16_t repeat) {
  if (nbits != kGreeBits)
    return;  // Wrong nr. of bits to send a proper message.
  // Set IR carrier frequency
  enableIROut(38);

  for (uint16_t r = 0; r <= repeat; r++) {
    // Header
    mark(kGreeHdrMark);
    space(kGreeHdrSpace);

    // Data
    for (int16_t i = 8; i <= nbits; i += 8) {
      sendData(kGreeBitMark, kGreeOneSpace, kGreeBitMark, kGreeZeroSpace,
               (data >> (nbits - i)) & 0xFF, 8, false);
      if (i == nbits / 2) {
        // Send the mid-message Footer.
        sendData(kGreeBitMark, kGreeOneSpace, kGreeBitMark, kGreeZeroSpace,
                 0b010, 3);
        mark(kGreeBitMark);
        space(kGreeMsgSpace);
      }
    }
    // Footer
    mark(kGreeBitMark);
    space(kGreeMsgSpace);
  }
}
#endif  // SEND_GREE

IRGreeAC::IRGreeAC(uint16_t pin) : _irsend(pin) { stateReset(); }

void IRGreeAC::stateReset(void) {
  // This resets to a known-good state to Power Off, Fan Auto, Mode Auto, 25C.
  for (uint8_t i = 0; i < kGreeStateLength; i++) remote_state[i] = 0x0;
  remote_state[1] = 0x09;
  remote_state[2] = 0x20;
  remote_state[3] = 0x50;
  remote_state[5] = 0x20;
  remote_state[7] = 0x50;
}

void IRGreeAC::fixup(void) {
  checksum();  // Calculate the checksums
}

void IRGreeAC::begin(void) { _irsend.begin(); }

#if SEND_GREE
void IRGreeAC::send(const uint16_t repeat) {
  fixup();  // Ensure correct settings before sending.
  _irsend.sendGree(remote_state, kGreeStateLength, repeat);
}
#endif  // SEND_GREE

uint8_t* IRGreeAC::getRaw(void) {
  fixup();  // Ensure correct settings before sending.
  return remote_state;
}

void IRGreeAC::setRaw(const uint8_t new_code[]) {
  for (uint8_t i = 0; i < kGreeStateLength; i++) {
    remote_state[i] = new_code[i];
  }
}

void IRGreeAC::checksum(const uint16_t length) {
  // Gree uses the same checksum alg. as Kelvinator's block checksum.
  uint8_t sum = IRKelvinatorAC::calcBlockChecksum(remote_state, length);
  remote_state[length - 1] = (sum << 4) | (remote_state[length - 1] & 0xFU);
}

// Verify the checksum is valid for a given state.
// Args:
//   state:  The array to verify the checksum of.
//   length: The size of the state.
// Returns:
//   A boolean.
bool IRGreeAC::validChecksum(const uint8_t state[], const uint16_t length) {
  // Top 4 bits of the last byte in the state is the state's checksum.
  if (state[length - 1] >> 4 ==
      IRKelvinatorAC::calcBlockChecksum(state, length))
    return true;
  else
    return false;
}

void IRGreeAC::on(void) {
  remote_state[0] |= kGreePower1Mask;
  remote_state[2] |= kGreePower2Mask;
}

void IRGreeAC::off(void) {
  remote_state[0] &= ~kGreePower1Mask;
  remote_state[2] &= ~kGreePower2Mask;
}

void IRGreeAC::setPower(const bool on) {
  if (on)
    this->on();
  else
    this->off();
}

bool IRGreeAC::getPower(void) {
  return (remote_state[0] & kGreePower1Mask) &&
         (remote_state[2] & kGreePower2Mask);
}

// Set the temp. in deg C
void IRGreeAC::setTemp(const uint8_t temp) {
  uint8_t new_temp = std::max((uint8_t)kGreeMinTemp, temp);
  new_temp = std::min((uint8_t)kGreeMaxTemp, new_temp);
  if (getMode() == kGreeAuto) new_temp = 25;
  remote_state[1] = (remote_state[1] & 0xF0U) | (new_temp - kGreeMinTemp);
}

// Return the set temp. in deg C
uint8_t IRGreeAC::getTemp(void) {
  return ((remote_state[1] & 0xFU) + kGreeMinTemp);
}

// Set the speed of the fan, 0-3, 0 is auto, 1-3 is the speed
void IRGreeAC::setFan(const uint8_t speed) {
  uint8_t fan = std::min((uint8_t)kGreeFanMax, speed);  // Bounds check

  if (getMode() == kGreeDry) fan = 1;  // DRY mode is always locked to fan 1.
  // Set the basic fan values.
  remote_state[0] &= ~kGreeFanMask;
  remote_state[0] |= (fan << 4);
}

uint8_t IRGreeAC::getFan(void) { return (remote_state[0] & kGreeFanMask) >> 4; }

void IRGreeAC::setMode(const uint8_t new_mode) {
  uint8_t mode = new_mode;
  switch (mode) {
    case kGreeAuto:
      // AUTO is locked to 25C
      setTemp(25);
      break;
    case kGreeDry:
      // DRY always sets the fan to 1.
      setFan(1);
      break;
    case kGreeCool:
    case kGreeFan:
    case kGreeHeat:
      break;
    default:
      // If we get an unexpected mode, default to AUTO.
      mode = kGreeAuto;
  }
  remote_state[0] &= ~kGreeModeMask;
  remote_state[0] |= mode;
}

uint8_t IRGreeAC::getMode(void) { return (remote_state[0] & kGreeModeMask); }

void IRGreeAC::setLight(const bool on) {
  remote_state[2] &= ~kGreeLightMask;
  remote_state[2] |= (on << 5);
}

bool IRGreeAC::getLight(void) { return remote_state[2] & kGreeLightMask; }

void IRGreeAC::setXFan(const bool on) {
  remote_state[2] &= ~kGreeXfanMask;
  remote_state[2] |= (on << 7);
}

bool IRGreeAC::getXFan(void) { return remote_state[2] & kGreeXfanMask; }

void IRGreeAC::setSleep(const bool on) {
  remote_state[0] &= ~kGreeSleepMask;
  remote_state[0] |= (on << 7);
}

bool IRGreeAC::getSleep(void) { return remote_state[0] & kGreeSleepMask; }

void IRGreeAC::setTurbo(const bool on) {
  remote_state[2] &= ~kGreeTurboMask;
  remote_state[2] |= (on << 4);
}

bool IRGreeAC::getTurbo(void) { return remote_state[2] & kGreeTurboMask; }

void IRGreeAC::setSwingVertical(const bool automatic, const uint8_t position) {
  remote_state[0] &= ~kGreeSwingAutoMask;
  remote_state[0] |= (automatic << 6);
  uint8_t new_position = position;
  if (!automatic) {
    switch (position) {
      case kGreeSwingUp:
      case kGreeSwingMiddleUp:
      case kGreeSwingMiddle:
      case kGreeSwingMiddleDown:
      case kGreeSwingDown:
        break;
      default:
        new_position = kGreeSwingLastPos;
    }
  } else {
    switch (position) {
      case kGreeSwingAuto:
      case kGreeSwingDownAuto:
      case kGreeSwingMiddleAuto:
      case kGreeSwingUpAuto:
        break;
      default:
        new_position = kGreeSwingAuto;
    }
  }
  remote_state[4] &= ~kGreeSwingPosMask;
  remote_state[4] |= new_position;
}

bool IRGreeAC::getSwingVerticalAuto(void) {
  return remote_state[0] & kGreeSwingAutoMask;
}

uint8_t IRGreeAC::getSwingVerticalPosition(void) {
  return remote_state[4] & kGreeSwingPosMask;
}


// Convert a standard A/C mode into its native mode.
uint8_t IRGreeAC::convertMode(const stdAc::opmode_t mode) {
  switch (mode) {
    case stdAc::opmode_t::kCool:
      return kGreeCool;
    case stdAc::opmode_t::kHeat:
      return kGreeHeat;
    case stdAc::opmode_t::kDry:
      return kGreeDry;
    case stdAc::opmode_t::kFan:
      return kGreeFan;
    default:
      return kGreeAuto;
  }
}

// Convert a standard A/C Fan speed into its native fan speed.
uint8_t IRGreeAC::convertFan(const stdAc::fanspeed_t speed) {
  switch (speed) {
    case stdAc::fanspeed_t::kMin:
      return kGreeFanMin;
    case stdAc::fanspeed_t::kLow:
    case stdAc::fanspeed_t::kMedium:
      return kGreeFanMax - 1;
    case stdAc::fanspeed_t::kHigh:
    case stdAc::fanspeed_t::kMax:
      return kGreeFanMax;
    default:
      return kGreeFanAuto;
  }
}

// Convert a standard A/C Vertical Swing into its native version.
uint8_t IRGreeAC::convertSwingV(const stdAc::swingv_t swingv) {
  switch (swingv) {
    case stdAc::swingv_t::kHighest:
      return kGreeSwingUp;
    case stdAc::swingv_t::kHigh:
      return kGreeSwingMiddleUp;
    case stdAc::swingv_t::kMiddle:
      return kGreeSwingMiddle;
    case stdAc::swingv_t::kLow:
      return kGreeSwingMiddleDown;
    case stdAc::swingv_t::kLowest:
      return kGreeSwingDown;
    default:
      return kGreeSwingAuto;
  }
}

// Convert a native mode to it's common equivalent.
stdAc::opmode_t IRGreeAC::toCommonMode(const uint8_t mode) {
  switch (mode) {
    case kGreeCool: return stdAc::opmode_t::kCool;
    case kGreeHeat: return stdAc::opmode_t::kHeat;
    case kGreeDry: return stdAc::opmode_t::kDry;
    case kGreeFan: return stdAc::opmode_t::kFan;
    default: return stdAc::opmode_t::kAuto;
  }
}

// Convert a native fan speed to it's common equivalent.
stdAc::fanspeed_t IRGreeAC::toCommonFanSpeed(const uint8_t speed) {
  switch (speed) {
    case kGreeFanMax: return stdAc::fanspeed_t::kMax;
    case kGreeFanMax - 1: return stdAc::fanspeed_t::kMedium;
    case kGreeFanMin: return stdAc::fanspeed_t::kMin;
    default: return stdAc::fanspeed_t::kAuto;
  }
}

// Convert a native vertical swing to it's common equivalent.
stdAc::swingv_t IRGreeAC::toCommonSwingV(const uint8_t pos) {
  switch (pos) {
    case kGreeSwingUp: return stdAc::swingv_t::kHighest;
    case kGreeSwingMiddleUp: return stdAc::swingv_t::kHigh;
    case kGreeSwingMiddle: return stdAc::swingv_t::kMiddle;
    case kGreeSwingMiddleDown: return stdAc::swingv_t::kLow;
    case kGreeSwingDown: return stdAc::swingv_t::kLowest;
    default: return stdAc::swingv_t::kAuto;
  }
}

// Convert the A/C state to it's common equivalent.
stdAc::state_t IRGreeAC::toCommon(void) {
  stdAc::state_t result;
  result.protocol = decode_type_t::GREE;
  result.model = -1;  // No models used.
  result.power = this->getPower();
  result.mode = this->toCommonMode(this->getMode());
  result.celsius = true;
  result.degrees = this->getTemp();
  result.fanspeed = this->toCommonFanSpeed(this->getFan());
  if (this->getSwingVerticalAuto())
    result.swingv = stdAc::swingv_t::kAuto;
  else
    result.swingv = this->toCommonSwingV(this->getSwingVerticalPosition());
  result.turbo = this->getTurbo();
  result.light = this->getLight();
  result.clean = this->getXFan();
  result.sleep = this->getSleep() ? 0 : -1;
  // Not supported.
  result.swingh = stdAc::swingh_t::kOff;
  result.quiet = false;
  result.econo = false;
  result.filter = false;
  result.beep = false;
  result.clock = -1;
  return result;
}

// Convert the internal state into a human readable string.
String IRGreeAC::toString(void) {
  String result = "";
  result.reserve(150);  // Reserve some heap for the string to reduce fragging.
  result += F("Power: ");
  if (getPower())
    result += F("On");
  else
    result += F("Off");
  result += F(", Mode: ");
  result += uint64ToString(getMode());
  switch (getMode()) {
    case kGreeAuto:
      result += F(" (AUTO)");
      break;
    case kGreeCool:
      result += F(" (COOL)");
      break;
    case kGreeHeat:
      result += F(" (HEAT)");
      break;
    case kGreeDry:
      result += F(" (DRY)");
      break;
    case kGreeFan:
      result += F(" (FAN)");
      break;
    default:
      result += F(" (UNKNOWN)");
  }
  result += F(", Temp: ");
  result += uint64ToString(getTemp());
  result += F("C, Fan: ");
  result += uint64ToString(getFan());
  switch (getFan()) {
    case 0:
      result += F(" (AUTO)");
      break;
    case kGreeFanMax:
      result += F(" (MAX)");
      break;
  }
  result += F(", Turbo: ");
  if (getTurbo())
    result += F("On");
  else
    result += F("Off");
  result += F(", XFan: ");
  if (getXFan())
    result += F("On");
  else
    result += F("Off");
  result += F(", Light: ");
  if (getLight())
    result += F("On");
  else
    result += F("Off");
  result += F(", Sleep: ");
  if (getSleep())
    result += F("On");
  else
    result += F("Off");
  result += F(", Swing Vertical Mode: ");
  if (getSwingVerticalAuto())
    result += F("Auto");
  else
    result += F("Manual");
  result += F(", Swing Vertical Pos: ");
  result += uint64ToString(getSwingVerticalPosition());
  switch (getSwingVerticalPosition()) {
    case kGreeSwingLastPos:
      result += F(" (Last Pos)");
      break;
    case kGreeSwingAuto:
      result += F(" (Auto)");
      break;
  }
  return result;
}

#if DECODE_GREE
// Decode the supplied Gree message.
//
// Args:
//   results: Ptr to the data to decode and where to store the decode result.
//   nbits:   The number of data bits to expect. Typically kGreeBits.
//   strict:  Flag indicating if we should perform strict matching.
// Returns:
//   boolean: True if it can decode it, false if it can't.
//
// Status: ALPHA / Untested.
bool IRrecv::decodeGree(decode_results* results, uint16_t nbits, bool strict) {
  if (results->rawlen <
      2 * (nbits + kGreeBlockFooterBits) + (kHeader + kFooter + 1))
    return false;  // Can't possibly be a valid Gree message.
  if (strict && nbits != kGreeBits)
    return false;  // Not strictly a Gree message.

  uint32_t data;
  uint16_t offset = kStartOffset;

  // There are two blocks back-to-back in a full Gree IR message
  // sequence.
  int8_t state_pos = 0;
  match_result_t data_result;

  // Header
  if (!matchMark(results->rawbuf[offset++], kGreeHdrMark)) return false;
  if (!matchSpace(results->rawbuf[offset++], kGreeHdrSpace)) return false;
  // Data Block #1 (32 bits)
  data_result =
      matchData(&(results->rawbuf[offset]), 32, kGreeBitMark, kGreeOneSpace,
                kGreeBitMark, kGreeZeroSpace, kTolerance, kMarkExcess, false);
  if (data_result.success == false) return false;
  data = data_result.data;
  offset += data_result.used;

  // Record Data Block #1 in the state.
  for (uint16_t i = 0; i < 4; i++, data >>= 8)
    results->state[state_pos + i] = data & 0xFF;
  state_pos += 4;

  // Block #1 footer (3 bits, B010)
  data_result = matchData(&(results->rawbuf[offset]), kGreeBlockFooterBits,
                          kGreeBitMark, kGreeOneSpace, kGreeBitMark,
                          kGreeZeroSpace, kTolerance, kMarkExcess, false);
  if (data_result.success == false) return false;
  if (data_result.data != kGreeBlockFooter) return false;
  offset += data_result.used;

  // Inter-block gap.
  if (!matchMark(results->rawbuf[offset++], kGreeBitMark)) return false;
  if (!matchSpace(results->rawbuf[offset++], kGreeMsgSpace)) return false;

  // Data Block #2 (32 bits)
  data_result =
      matchData(&(results->rawbuf[offset]), 32, kGreeBitMark, kGreeOneSpace,
                kGreeBitMark, kGreeZeroSpace, kTolerance, kMarkExcess, false);
  if (data_result.success == false) return false;
  data = data_result.data;
  offset += data_result.used;

  // Record Data Block #2 in the state.
  for (uint16_t i = 0; i < 4; i++, data >>= 8)
    results->state[state_pos + i] = data & 0xFF;
  state_pos += 4;

  // Footer.
  if (!matchMark(results->rawbuf[offset++], kGreeBitMark)) return false;
  if (offset <= results->rawlen &&
      !matchAtLeast(results->rawbuf[offset], kGreeMsgSpace))
    return false;

  // Compliance
  if (strict) {
    // Correct size/length)
    if (state_pos != kGreeStateLength) return false;
    // Verify the message's checksum is correct.
    if (!IRGreeAC::validChecksum(results->state)) return false;
  }

  // Success
  results->decode_type = GREE;
  results->bits = state_pos * 8;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  return true;
}
#endif  // DECODE_GREE
