#include "smartevse_sensorbox.h"

#define TAG "smartevse_sensorbox"

namespace esphome {
namespace smartevse_sensorbox {
    SmartEVSESensorbox::SmartEVSESensorbox() {}

    void SmartEVSESensorbox::add_p1_value_sensor(int type, esphome::sensor::Sensor* &sensor) {
        P1Sensors[type] = sensor;
        P1SensorSet[type] = true;
    }

    void SmartEVSESensorbox::use_ct_readings(bool useCTs) {
        UseCTs = useCTs ? 1 : 0;
    }

    void SmartEVSESensorbox::set_ct_wire(int wire) {
        CTWire = wire;
        Wire = CTWire + CTRotation;
    }

    void SmartEVSESensorbox::set_ct_rotation(int rotation) {
        CTRotation = rotation;
        Wire = CTWire + CTRotation;
    }

    void SmartEVSESensorbox::set_p1_version_sensor(esphome::text_sensor::TextSensor* &sensor) {
      DSMRVersionSensor = sensor;
      sensor->add_on_state_callback([this](std::__cxx11::basic_string<char> val) {
        P1LastUpdate = time(NULL);
      });
    }

    uint16_t SmartEVSESensorbox::modbus_input_on_read(uint16_t reg, uint16_t val) {
      unsigned char dataready = 0;
      unsigned char DSMRver = 0;

      float value = 0;
      float produceValue;

      ESP_LOGD("modbus", "register %i read request from master", reg);

      switch (reg) {
        case 0:
          // Sensorbox version 1 (the one without extra fields), Wire setting
          return (uint16_t) 0 + Wire;
          break;
        case 1:
          // DSMR Version(MSB), CT's or P1(LSB)
          // 0x3283 = DSMR version 50, P1 port connected (0x80) CT's Used (0x03)
          if (UseCTs && is_ct_ready()) {
            dataready |= 0x03;
          }
          if (is_p1_ready()) {
            DSMRver = atoi(DSMRVersionSensor->state.c_str());
            if (DSMRver == 50) {
              dataready |= 0x80;
            } else {
              dataready |= 0x40;
            }
          }
          return (uint16_t) (DSMRver<<8) + dataready;
          break;
        case 2:
        case 3:
          // Volts L1 (32 bit floating point), Smartmeter P1 data
          if (P1SensorSet[4]) {
            value = P1Sensors[4]->state;
          }
          return float_to_modbus(value, reg);
          break;
        case 4:
        case 5:
          // Volts L2 (32 bit floating point), Smartmeter P1 data
          if (P1SensorSet[5]) {
            value = P1Sensors[5]->state;
          }
          return float_to_modbus(value, reg);
          break;
        case 6:
        case 7:
          // Volts L3 (32 bit floating point), Smartmeter P1 data
          if (P1SensorSet[6]) {
            value = P1Sensors[6]->state;
          }
          return float_to_modbus(value, reg);
          break;
        case 8:
        case 9:
          // Current L1 (32 bit floating point), Smartmeter P1 data
          if (P1SensorSet[1] && P1SensorSet[4]) {
            if (P1Sensors[4]->state > 200) {
              value = P1Sensors[1]->state * 1000 / P1Sensors[4]->state;
              if (P1SensorSet[7]) {
                if (P1Sensors[7]->state > 0) {
                  value = -(P1Sensors[7]->state * 1000 / P1Sensors[4]->state);
                }
              }
            }
          }
          return float_to_modbus(value, reg);
          break;
        case 10:
        case 11:
          // Current L2 (32 bit floating point), Smartmeter P1 data
          if (P1SensorSet[2] && P1SensorSet[5]) {
            if (P1Sensors[5]->state > 200) {
              value = P1Sensors[2]->state * 1000 / P1Sensors[5]->state;
              if (P1SensorSet[8]) {
                if (P1Sensors[8]->state > 0) {
                  value = -(P1Sensors[8]->state * 1000 / P1Sensors[5]->state);
                }
              }
            }
          }
          return float_to_modbus(value, reg);
          break;
        case 12:
        case 13:
          // Current L3 (32 bit floating point), Smartmeter P1 data
          if (P1SensorSet[3] && P1SensorSet[6]) {
            if (P1Sensors[6]->state > 200) {
              value = P1Sensors[3]->state * 1000 / P1Sensors[6]->state;
              if (P1SensorSet[9]) {
                if (P1Sensors[9]->state > 0) {
                  value = -(P1Sensors[9]->state * 1000 / P1Sensors[6]->state);
                }
              }
            }
          }
          return float_to_modbus(value, reg);
          break;
        case 14:
        case 15:
          // Current L1 (32 bit floating point), CT reading data
          return float_to_modbus(IrmsCT[0], reg);
          break;
        case 16:
        case 17:
          // Current L2 (32 bit floating point), CT reading data
          return float_to_modbus(IrmsCT[1], reg);
          break;
        case 18:
        case 19:
          // Current L3 (32 bit floating point), CT reading data
          return float_to_modbus(IrmsCT[2], reg);
          break;
      }

      // By default we return a zero
      return 0;
    }

    void SmartEVSESensorbox::setup() {
        RESET_OUTPUT;
        DAT_INPUT;
        CLK_INPUT;
        RESET_HIGH;

        pinMode(PIN_PGD, INPUT);
        pinMode(PIN_PGC, OUTPUT);

        pinMode(PIN_LED_GREEN, OUTPUT);
        pinMode(PIN_LED_RED, OUTPUT);

        // workaround ESP three uart bug
        uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, 23, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

        ct1_current_->set_icon("mdi:current-ac");
        ct1_current_->set_unit_of_measurement("A");
        ct1_current_->set_accuracy_decimals(2);

        ct2_current_->set_icon("mdi:current-ac");
        ct2_current_->set_unit_of_measurement("A");
        ct2_current_->set_accuracy_decimals(2);

        ct3_current_->set_icon("mdi:current-ac");
        ct3_current_->set_unit_of_measurement("A");
        ct3_current_->set_accuracy_decimals(2);
    }

    void SmartEVSESensorbox::loop() {
        if (!NoP1Data && (time(NULL) - P1LastUpdate) < 6) {
            digitalWrite(PIN_LED_RED, HIGH);
        } else if(!NoP1Data) {
            digitalWrite(PIN_LED_RED, LOW);
            NoP1Data = 1;
        }

        ct_read_values();
        this->ct_read_values();
    }

    void SmartEVSESensorbox::ct_read_values() {
      int Samples = 0;
      unsigned char x, CTwire = 0;
      static unsigned char CTstring[100], CTeot = 0;
      static unsigned char CTlength = 0, CTptr = 0;
      uint16_t crccal, crcdata;

      int Power1A = 0, Power1B = 0, Power2A = 0, Power2B = 0, Power3A = 0, Power3B = 0;
      const float y1 = -3.114637, x1 = 4.043093, y2 = -3.057741, x2 = 3.988535, y3 = -3.000724,
                  x3 = 3.933821;  // 60 samples 19.00 Ilead

      while (available()) {
        char *ret, ch;
        ch = read();

        if (ch == '/') {  // Start character
          CTptr = 0;
          CTeot = 0;  // start from beginning of buffer
        }

        if (CTeot) {  // end of transmission?
          if (CTeot > 4)
            ch = 0;  // we have also received the CRC, null terminate
          CTeot++;
        }

        CTstring[CTptr] = ch;  // Store in buffer
        if (CTptr < 90)
          CTptr++;  // prevent overflow of buffer

        if (ch == '!' && CTstring[0] == '/') {
          CTlength = CTptr;  // store pointer of start of CRC
          CTeot = 1;
        }

        if (CTeot > 5) {
          crcdata =
              (uint16_t) strtol((const char *) CTstring + CTlength, NULL, 16);  // get crc from data, convert to int
          crccal = SmartEVSESensorbox::CRC16(0, CTstring, CTlength);            // calculate CRC16 from data

          ESP_LOGD("pic", "length: %u, CRC16: %04x : %04x string: %s", CTlength, crccal, crcdata, CTstring);

          if (crcdata == crccal) {
            ret = strstr((const char *) CTstring, (const char *) "1A:");  // Extract CT measurements from the buffer.
            if (ret != NULL)
              Power1A = atoi((const char *) ret + 3);
            ret = strstr((const char *) CTstring, (const char *) "1B:");
            if (ret != NULL)
              Power1B = atoi((const char *) ret + 3);
            ret = strstr((const char *) CTstring, (const char *) "2A:");
            if (ret != NULL)
              Power2A = atoi((const char *) ret + 3);
            ret = strstr((const char *) CTstring, (const char *) "2B:");
            if (ret != NULL)
              Power2B = atoi((const char *) ret + 3);
            ret = strstr((const char *) CTstring, (const char *) "3A:");
            if (ret != NULL)
              Power3A = atoi((const char *) ret + 3);
            ret = strstr((const char *) CTstring, (const char *) "3B:");
            if (ret != NULL)
              Power3B = atoi((const char *) ret + 3);
            ret = strstr((const char *) CTstring, (const char *) "SA:");
            if (ret != NULL)
              Samples = atoi((const char *) ret + 3);
            // as we divide by Samples, it can not be 0!
            if (Samples < 1)
              Samples = 1;

            ret = strstr((const char *) CTstring, (const char *) "WI:");  // CT wire setting 4Wire=0, 3Wire=1 (bit1)
            if (ret != NULL)
              CTwire = atoi((const char *) ret + 3);  // and phase rotation CW=0, CCW=1 (bit0)

            // Irms data when there is no mains plug connected.
            // There is no way of knowing the direction of the current.
            ret = strstr((const char *) CTstring, (const char *) "1R:");
            if (ret != NULL)
              Power1A = atoi((const char *) ret + 3);
            ret = strstr((const char *) CTstring, (const char *) "2R:");
            if (ret != NULL)
              Power2A = atoi((const char *) ret + 3);
            ret = strstr((const char *) CTstring, (const char *) "3R:");
            if (ret != NULL) {
              Power3A = atoi((const char *) ret + 3);

              IrmsCT[0] = sqrt((float) Power1A / Samples) * CALRMS;
              IrmsCT[1] = sqrt((float) Power2A / Samples) * CALRMS;
              IrmsCT[2] = sqrt((float) Power3A / Samples) * CALRMS;

              // CT Measurement with no current direction information
              IrmsMode = 1;

            } else {
              // We do have enough data to calculate the Irms and direction of current for each phase
              IrmsCT[0] = (x1 * ((float) Power1A / Samples) + y1 * ((float) Power1B / Samples)) / CAL;
              IrmsCT[1] = (x2 * ((float) Power2A / Samples) + y2 * ((float) Power2B / Samples)) / CAL;
              IrmsCT[2] = (x3 * ((float) Power3A / Samples) + y3 * ((float) Power3B / Samples)) / CAL;

              IrmsMode = 0;
            }

            // very small values will be displayed as 0.0A
            for (x = 0; x < 3; x++) {
              if ((IrmsCT[x] > -0.05) && (IrmsCT[x] < 0.05))
                IrmsCT[x] = 0.0;
            }

            // if selected Wire setting (3-Wire or 4-Wire) and CW and CCW phase rotation are not correctly set, we can toggle the PGC pin to set it.
            if ((CTwire != Wire) && IrmsMode == 0) {
              x = (4 + Wire - CTwire) % 4;
              ESP_LOGD("pic", "Wire:%u CTwire:%u pulses %u\n", Wire, CTwire, x);
              do {
                digitalWrite(PIN_PGC, HIGH);
                digitalWrite(PIN_PGC, LOW);
                vTaskDelay(1 / portTICK_PERIOD_MS);
              } while (--x);
            }

            ct1_current_->publish_state(IrmsCT[0]);
            ct2_current_->publish_state(IrmsCT[1]);
            ct3_current_->publish_state(IrmsCT[2]);

            CTLastUpdate = time(NULL);

          } else {
            ESP_LOGW("pic", "CRC error in CTdata\n");
          }

          CTeot = 0;
          CTptr = 0;
          memset(CTstring, 0u, 100u);
        }
      }
    }

    uint16_t SmartEVSESensorbox::float_to_modbus(float val, uint16_t reg) {
        char *pBytes = (char*)&val;
        ESP_LOGD("modbus", "register %i read response value %.2f", reg, val);
        if (reg % 2 == 0) {
            return (uint16_t) (pBytes[3]<<8)+pBytes[2];
        } else {
            return (uint16_t) (pBytes[1]<<8)+pBytes[0];
        }
    }

    bool SmartEVSESensorbox::is_p1_ready() {
        return NoP1Data == 1;
    }

    bool SmartEVSESensorbox::is_ct_ready() {
      uint16_t now = time(NULL);
      return (now - CTLastUpdate) < 6;
    }

    // Poly used is x^16+x^15+x^2+x
    unsigned int SmartEVSESensorbox::CRC16(unsigned int crc, unsigned char *buf, int len)
    {
        for (int pos = 0; pos < len; pos++)
        {
            crc ^= (unsigned int)buf[pos];        // XOR byte into least sig. byte of crc

            for (int i = 8; i != 0; i--) {        // Loop over each bit
                if ((crc & 0x0001) != 0) {          // If the LSB is set
                    crc >>= 1;                        // Shift right and XOR 0xA001
                    crc ^= 0xA001;
                }
                else                                // Else LSB is not set
                    crc >>= 1;                        // Just shift right
            }
        }
        return crc;
    }
}  // namespace smartevse_sensorbox
}  // namespace esphome
