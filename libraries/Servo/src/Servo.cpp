/*
Servo library using shared TIMER1 infrastructure

Original Copyright (c) 2015 Michael C. Miller. All right reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#if defined(ESP8266)

#include <Arduino.h>
#include <Servo.h>
#include "core_esp8266_waveform.h"

uint32_t Servo::_servoMap = 0;

// similiar to map but will have increased accuracy that provides a more
// symmetrical api (call it and use result to reverse will provide the original value)
int improved_map(int value, int minIn, int maxIn, int minOut, int maxOut)
{
    const int rangeIn = maxIn - minIn;
    const int rangeOut = maxOut - minOut;
    const int deltaIn = value - minIn;
    // fixed point math constants to improve accurancy of divide and rounding
    constexpr int fixedHalfDecimal = 1;
    constexpr int fixedDecimal = fixedHalfDecimal * 2;

    return ((deltaIn * rangeOut * fixedDecimal) / (rangeIn) + fixedHalfDecimal) / fixedDecimal + minOut;
}

//-------------------------------------------------------------------
// Servo class methods

Servo::Servo()
{
  _attached = false;
  _valueUs = DEFAULT_NEUTRAL_PULSE_WIDTH;
  _minUs = DEFAULT_MIN_PULSE_WIDTH;
  _maxUs = DEFAULT_MAX_PULSE_WIDTH;
}

Servo::~Servo() {
  detach();
}


uint8_t Servo::attach(int pin)
{
  return attach(pin, DEFAULT_MIN_PULSE_WIDTH, DEFAULT_MAX_PULSE_WIDTH);
}

uint8_t Servo::attach(int pin, uint16_t minUs, uint16_t maxUs)
{
  return attach(pin, minUs, maxUs, _valueUs);
}

uint8_t Servo::attach(int pin, uint16_t minUs, uint16_t maxUs, int value)
{
  if (!_attached) {
    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
    _pin = pin;
    _attached = true;
  }

  // keep the min and max within 200-3000 us, these are extreme
  // ranges and should support extreme servos while maintaining
  // reasonable ranges
  _maxUs = max((uint16_t)250, min((uint16_t)3000, maxUs));
  _minUs = max((uint16_t)200, min(_maxUs, minUs));

  write(value);

  return pin;
}

void Servo::detach()
{
  if (_attached) {
    _servoMap &= ~(1 << _pin);
    startWaveform(_pin, 0, REFRESH_INTERVAL, 1);
    delay(REFRESH_INTERVAL / 1000); // long enough to complete active period under all circumstances.
    stopWaveform(_pin);
    _attached = false;
    _valueUs = DEFAULT_NEUTRAL_PULSE_WIDTH;
  }
}

void Servo::write(int value)
{
  // treat any value less than 200 as angle in degrees (values equal or larger are handled as microseconds)
  if (value < 200) {
    // assumed to be 0-180 degrees servo
    value = constrain(value, 0, 180);
    value = improved_map(value, 0, 180, _minUs, _maxUs);
  }
  writeMicroseconds(value);
}

void Servo::writeMicroseconds(int value)
{
  value = constrain(value, _minUs, _maxUs);
  _valueUs = value;
  if (_attached) {
    _servoMap &= ~(1 << _pin);
    if (startWaveform(_pin, _valueUs, REFRESH_INTERVAL - _valueUs, 0)) {
      _servoMap |= (1 << _pin);
    }
  }
}

int Servo::read() // return the value as degrees
{
  // read returns the angle for an assumed 0-180
  return improved_map(readMicroseconds(), _minUs, _maxUs, 0, 180);
}

int Servo::readMicroseconds()
{
  return _valueUs;
}

bool Servo::attached()
{
  return _attached;
}

#endif
