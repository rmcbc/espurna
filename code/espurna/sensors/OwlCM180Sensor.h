// -----------------------------------------------------------------------------
// Owl CM180 Power meter Sensor
// Copyright (C) 2020 by Rui Caridade <rui dot mcbc at gmail dot com>
// -----------------------------------------------------------------------------

#if SENSOR_SUPPORT && OWLCM180_SUPPORT

#pragma once

#include <Arduino.h>

#include "../debug.h"
#include "BaseSensor.h"

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
        // Sensor API
        // ---------------------------------------------------------------------

        // Initialization method, must be idempotent
        void begin() {
            //pinMode(_gpio, INPUT_PULLUP);
            pinMode(_gpio, INPUT);
            //pinMode(_gpio, OUTPUT);
            _ready = true;

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
            _read();
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

        void _process() {
            _kwh = 0;
        }

        void _read() {
            _error = SENSOR_ERROR_OK;

            _process();
        }

        // ---------------------------------------------------------------------

        unsigned int _owlPin = OWLCM180_PIN;
        double _kwh = 0;
};

#endif // SENSOR_SUPPORT && OWLCM180_SUPPORT
