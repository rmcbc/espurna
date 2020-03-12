// -----------------------------------------------------------------------------
// Owl CM180 Power meter Sensor
// Copyright (C) 2020 by Rui Caridade <rui dot mcbc at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && OWLCM180_SUPPORT

#pragma once

#include <Arduino.h>

#include "BaseSensor.h"

// New code to decode OOK signals from Energy OWL CMR180 sensor
// Oregon V3 decoder added - Eric Vandecasteele (onlinux)
//
// Oregon V2 decoder modfied - Olivier Lebrun
// Oregon V2 decoder added - Dominique Pierre
// New code to decode OOK signals from weather sensors, etc.
// 2010-04-11 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
// $Id: ookDecoder.pde 5331 2010-04-17 10:45:17Z jcw $
 
class DecodeOOK {
protected:
    byte total_bits, bits, flip, state, pos, data[31];
 
    virtual char decode (word width) =0;
 
public:
 
    enum { UNKNOWN, T0, T1, T2, T3, OK, DONE };
 
    DecodeOOK () { resetDecoder(); }
 
    bool nextPulse (word width) {
        if (state != DONE)
 
            switch (decode(width)) {
                case -1: resetDecoder(); break;
                case 1:  done(); break;
            }
        return isDone();
    }
 
    bool isDone () const { return state == DONE; }
 
    const byte* getData (byte& count) const {
        count = pos;
        return data; 
    }
 
    void resetDecoder () {
        total_bits = bits = pos = flip = 0;
        state = UNKNOWN;
    }
 
    // add one bit to the packet data buffer
 
    virtual void gotBit (char value) {
        total_bits++;
        byte *ptr = data + pos;
        *ptr = (*ptr >> 1) | (value << 7);
 
        if (++bits >= 8) {
            bits = 0;
            if (++pos >= sizeof data) {
                resetDecoder();
                return;
            }
        }
        state = OK;
    }
 
    // store a bit using Manchester encoding_rx
    void manchester (char value) {
        flip ^= value; // manchester code, long pulse flips the bit
        gotBit(flip);
    }
 
    // move bits to the front so that all the bits are aligned to the end
    void alignTail (byte max =0) {
        // align bits
        if (bits != 0) {
            data[pos] >>= 8 - bits;
            for (byte i = 0; i < pos; ++i)
                data[i] = (data[i] >> bits) | (data[i+1] << (8 - bits));
            bits = 0;
        }
        // optionally shift bytes down if there are too many of 'em
        if (max > 0 && pos > max) {
            byte n = pos - max;
            pos = max;
            for (byte i = 0; i < pos; ++i)
                data[i] = data[i+n];
        }
    }
 
    void reverseBits () {
        for (byte i = 0; i < pos; ++i) {
            byte b = data[i];
            for (byte j = 0; j < 8; ++j) {
                data[i] = (data[i] << 1) | (b & 1);
                b >>= 1;
            }
        }
    }
 
    void reverseNibbles () {
        for (byte i = 0; i < pos; ++i)
            data[i] = (data[i] << 4) | (data[i] >> 4);
    }
 
    void done () {
        while (bits)
            gotBit(0); // padding
        state = DONE;
    }
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

class OregonDecoderV3 : public DecodeOOK {
  public:   
 
    OregonDecoderV3() {}
 
    // add one bit to the packet data buffer
    virtual void gotBit (char value) {
        data[pos] = (data[pos] >> 1) | (value ? 0x80 : 00);
        total_bits++;
        pos = total_bits >> 3;
        if (pos >= sizeof data) {
            //Serial.println("sizeof data");
            resetDecoder();
            return;
        }
        state = OK;
    }
 
    virtual char decode (word width) {
       if (200 <= width && width < 1200) {
            //Serial.println(width);
            byte w = width >= 700;
 
            switch (state) {
                case UNKNOWN:
                    if (w == 0) {
                        // Long pulse
                        ++flip;
                    } else if (32 <= flip) {
                        flip = 1;
                        manchester(1);
                    } else {
                        // Reset decoder
                        return -1;
                    }
                    break;
                case OK:
                    if (w == 0) {
                        // Short pulse
                        state = T0;
                    } else {
                        // Long pulse
                        manchester(1);
                    }
                    break;
                case T0:
                    if (w == 0) {
                      // Second short pulse
                        manchester(0);
                    } else {
                        // Reset decoder
                        return -1;
                    }
                    break;
              }
        } else  {
            // Trame intermédiaire 48bits ex: [OSV3 6281 3C 6801 70]
            return  (total_bits <104 && total_bits>=40  ) ? 1: -1;
        }
        
        return (total_bits == 104) ? 1: 0;
    }
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

class OwlCM180Sensor : public BaseSensor {

    public:

        // ---------------------------------------------------------------------
        // Public
        // ---------------------------------------------------------------------

        OwlCM180Sensor(): BaseSensor() {
            _count = 1;
            _sensor_id = SENSOR_OWLCM180_ID;
            _sensor_kwh_units == ENERGY_KWH;
        }

        // ---------------------------------------------------------------------

        void setGPIO(unsigned char gpio) {
            _gpio = gpio;
        }

        // ---------------------------------------------------------------------

        unsigned char getGPIO() {
            return _gpio;
        }

        // ---------------------------------------------------------------------

        void ext_int_1(void)
        {
            static word last;
            // determine the pulse length in microseconds, for either polarity
            pulse = micros() - last;
            last += pulse;
        }

        // ---------------------------------------------------------------------
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {
            //pinMode(_gpio, INPUT_PULLUP);
            pinMode(_gpio, INPUT);
            //pinMode(_gpio, OUTPUT);
            _ready = true;

            //attachInterrupt(1, ext_int_1, CHANGE);
        }

        // Descriptive name of the sensor
        String description() {
            char buffer[20];
            snprintf(buffer, sizeof(buffer), "OWL CM180 @ GPIO%d", _gpio);
            return String(buffer);
        }

        // Descriptive name of the slot # index
        String slot(unsigned char index) {
            return description();
        };

        // Address of the sensor (it could be the GPIO or I2C address)
        String address(unsigned char index) {
            return String(_owlPin);
        }

        // Loop-like method, call it in your main loop
        void tick() {
        }

        // Type for slot # index
        unsigned char type(unsigned char index) {
            return MAGNITUDE_ENERGY;
        }

        // Current value for slot # index
        double value(unsigned char index) {
            return _kwh;
        }

    protected:

        // ---------------------------------------------------------------------
        // Protected
        // ---------------------------------------------------------------------

        //OregonDecoderV3 orscV3;

        volatile word pulse;

        unsigned int _owlPin = OWLCM180_PIN;
        double _kwh = 0;
};

#endif // SENSOR_SUPPORT && OWLCM180_SUPPORT
