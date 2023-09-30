#pragma once

#include <Arduino.h>

#include "esphome/core/component.h"
#include "esphome/core/entity_base.h"
#include "esphome/core/preferences.h"
#include "esphome/core/helpers.h"

#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"

#include "driver/uart.h"

#define CAL 151                                                                 // CT calibration value (normal mode)
#define CALRMS 0.3                                                              // CT calibration value (RMS mode)

#define PIN_PGD 23		// PIC connection to PGD, serial measurements input
#define PIN_PGC 22		// PIC connection to PGC, sets PIC wire mode (output)

#define DLY1  1      // 1 microsecond for toggling and stuff
#define DLY2  6000   // 6  millisecond for command delay

#define PIN_RESET 21
#define PIN_DAT   23
#define PIN_CLK   22

#define DAT_INPUT   pinMode(PIN_DAT, INPUT)
#define DAT_OUTPUT  pinMode(PIN_DAT, OUTPUT)
#define DAT_LOW     digitalWrite(PIN_DAT, LOW)
#define DAT_HIGH    digitalWrite(PIN_DAT, HIGH)
#define DAT_GET     digitalRead(PIN_DAT)

#define CLK_INPUT   pinMode(PIN_CLK, INPUT)
#define CLK_OUTPUT  pinMode(PIN_CLK, OUTPUT)
#define CLK_LOW     digitalWrite(PIN_CLK, LOW)
#define CLK_HIGH    digitalWrite(PIN_CLK, HIGH)

#define RESET_INPUT  pinMode(PIN_RESET, INPUT)
#define RESET_OUTPUT pinMode(PIN_RESET, OUTPUT)
#define RESET_LOW   digitalWrite(PIN_RESET, LOW)
#define RESET_HIGH  digitalWrite(PIN_RESET, HIGH)

#define PIN_LED_GREEN 32
#define PIN_LED_RED   33

namespace esphome {
    namespace smartevse_sensorbox {
        using namespace std;

        class SmartEVSESensorbox : public esphome::uart::UARTDevice, public Component {
         public:
          explicit SmartEVSESensorbox();

          uint16_t modbus_input_on_read(uint16_t reg, uint16_t val);

          void add_p1_value_sensor(int type, esphome::sensor::Sensor* &sensor);
          void set_p1_version_sensor(esphome::text_sensor::TextSensor* &sensor);

          void use_ct_readings(bool useCTs);
          void set_ct_wire(int wire);
          void set_ct_rotation(int rotation);
          void p1_updated();

          void setup() override;
          void loop() override;

          esphome::sensor::Sensor* ct1_current_ = new esphome::sensor::Sensor();
          esphome::sensor::Sensor* ct2_current_ = new esphome::sensor::Sensor();
          esphome::sensor::Sensor* ct3_current_ = new esphome::sensor::Sensor();

         private:
          unsigned char IrmsMode = 0, UseCTs = 0, Wire = 0, CTWire = 0, CTRotation = 0, NoP1Data = 0;
          float IrmsCT[3];

          bool P1SensorSet[10];
          esphome::sensor::Sensor* P1Sensors[10];
          esphome::text_sensor::TextSensor* DSMRVersionSensor;

          uint16_t P1LastUpdate = 0;
          uint16_t CTLastUpdate = 0;

          void ct_read_values();

          bool is_ct_ready();
          bool is_p1_ready();

          uint16_t float_to_modbus(float val, uint16_t reg);
          unsigned int CRC16(unsigned int crc, unsigned char *buf, int len);
        };

    }  // namespace smartevse_sensorbox
}  // namespace esphome
