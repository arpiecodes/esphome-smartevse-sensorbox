# esphome-smartevse-sensorbox

This ESPHome component code provides the basic functionality of SmartEVSE Sensorbox 2 (https://github.com/SmartEVSE/Sensorbox-2) within ESPHome.

- Sends current and voltage information to SmartEVSE master through modbus
- Both P1 and CT readings are supported through simple configuration
- Possibility to adjust power rotation and 3/4 wire configurations
- Basic protection against loss of CT/P1 data (internal timer unsets dataready telegram if no fresh data)
- Allows to use CT readings without actually using the CT readings for SmartEVSE (for example; solar production)
- Allows to log/consume a whole array of extra P1 measurement data with the DSMR ESPHome component

Basically if you already use Home Assistant and ESPHome, it can easily be flashed and used as a drop-in replacement for the original Sensorbox firmware without losing its core functionality of feeding P1/CT power data to SmartEVSE without having to rely on WiFi, network or (for example) Home Assistant.

Below YAML can simply be copied and paste'd in your ESPHome project and ESPHome will then automatically fetch the latest versions of the code and compile everything for you. No need to copy and paste any custom files. Please note that it is only meant to be uploaded onto Sensorbox 2 hardware. No out of the box support for other configurations (even though you could in theory make this work).

*NOTE: The PIC on your Sensorbox should be programmed to only provide CT measurements, else it may interfere with modbus communication. Please first install the original ESP32 Sensorbox 2 firmware and reboot the device once (so the PIC can be reprogrammed) before using the ESPHome component.*

```yaml
esphome:
  name: sensorbox
  platformio_options:
    board_build.f_cpu: 240000000L

time:
  - platform: homeassistant
    id: homeassistant_time

external_components:
  - source: github://arpiecodes/esphome-modbus-server@master
    refresh: 60s
    components:
      - modbus_server
  - source: github://arpiecodes/esphome-smartevse-sensorbox@main
    refresh: 60s
    components:
      - smartevse_sensorbox

esp32:
  board: esp32dev
  variant: esp32
  framework:
    type: arduino

# Enable logging
logger:
  baud_rate: 0 # Disable logging over UART

# Enable Home Assistant API
api:
  # Don't reboot if the HomeAssistant connection times out (we can do just fine without it)
  reboot_timeout: 0s

ota:
  password: "YOURPASSWORDHERE"

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
  power_save_mode: none

  # Enable fallback hotspot (captive portal) in case wifi connection fails
  ap:
    ssid: SmartEVSE Sensorbox
    password: !secret wifi_fallback_password

captive_portal:

status_led:
  pin: GPIO32

uart:
  - id: uart_pic
    baud_rate: 115200
    rx_pin: 3 # pin is changed to 23 inside component setup method
    tx_pin: 1 # circumvents limitation with 3 active UARTs on non-default pins in ESPHome
  - id: uart_modbus
    baud_rate: 9600
    parity: NONE
    stop_bits: 1
    tx_pin: 16
    rx_pin: 19
  - id: uart_p1
    baud_rate: 115200
    rx_buffer_size: 1700
    rx_pin:
      number: 27
      inverted: true

modbus_server:
  - id: modbus_evse
    uart_id: uart_modbus
    address: 10 # fixed sensorbox address
    de_pin: 17 # don't change
    re_pin: 18 # don't change
    input_registers:
      - start_address: 0
        default: 0
        number: 20
        on_read: |
          // This lambda processes modbus input register requests from SmartEVSE and responds with latest values
          return id(sensorbox)->modbus_input_on_read(address, value);

smartevse_sensorbox:
  id: sensorbox
  uart_id: uart_pic
  use_ct_readings: false # whether to send CT values to SmartEVSE as EnergyMeter readings
  ct_rotation: CW # CT field rotation (CW: clockwise, CCW: counter-clockwise)
  ct_wires: 4WIRE # CT wire configuration (3WIRE or 4WIRE)
  p1_sensors: # optional if use_ct_readings is true
    # IDs of sensors to use for P1 measurements (see dsmr section below) 
    dsmr_version: dsmr_version
    power_consumed_phase_1: power_consumed_phase_1
    power_consumed_phase_2: power_consumed_phase_2
    power_consumed_phase_3: power_consumed_phase_3
    voltage_phase_1: voltage_phase_1
    voltage_phase_2: voltage_phase_2
    voltage_phase_3: voltage_phase_3
    power_produced_phase_1: power_produced_phase_1
    power_produced_phase_2: power_produced_phase_2
    power_produced_phase_3: power_produced_phase_3

# DSMR component config
# see https://esphome.io/components/sensor/dsmr.html
dsmr:
  uart_id: uart_p1
  max_telegram_length: 1700
  request_interval: 2s
  receive_timeout: 300ms

sensor:
  - platform: dsmr
    energy_delivered_tariff1:
      name: "Energy Consumed Tariff 1"
      state_class: total_increasing
    energy_delivered_tariff2:
      name: "Energy Consumed Tariff 2"
      state_class: total_increasing
    energy_returned_lux:
      name: "Energy Produced Luxembourg"
      state_class: total_increasing
    energy_returned_tariff1:
      name: "Energy Produced Tariff 1"
      state_class: total_increasing
    energy_returned_tariff2:
      name: "Energy Produced Tariff 2"
      state_class: total_increasing
    power_delivered:
      name: "Power Consumed"
      accuracy_decimals: 3
    power_returned:
      name: "Power Produced"
      accuracy_decimals: 3
    electricity_failures:
      name: "Electricity Failures"
      icon: mdi:alert
    electricity_long_failures:
      name: "Long Electricity Failures"
      icon: mdi:alert
    voltage_l1:
      id: voltage_phase_1
      name: "Voltage Phase 1"
    voltage_l2:
      id: voltage_phase_2
      name: "Voltage Phase 2"
    voltage_l3:
      id: voltage_phase_3
      name: "Voltage Phase 3"
    current_l1:
      id: current_phase_1
      name: "Current Phase 1"
    current_l2:
      id: current_phase_2
      name: "Current Phase 2"
    current_l3:
      id: current_phase_3
      name: "Current Phase 3"
    power_delivered_l1:
      id: power_consumed_phase_1
      name: "Power Consumed Phase 1"
      accuracy_decimals: 3
    power_delivered_l2:
      id: power_consumed_phase_2
      name: "Power Consumed Phase 2"
      accuracy_decimals: 3
    power_delivered_l3:
      id: power_consumed_phase_3
      name: "Power Consumed Phase 3"
      accuracy_decimals: 3
    power_returned_l1:
      id: power_produced_phase_1
      name: "Power Produced Phase 1"
      accuracy_decimals: 3
    power_returned_l2:
      name: "Power Produced Phase 2"
      id: power_produced_phase_2
      accuracy_decimals: 3
    power_returned_l3:
      id: power_produced_phase_3
      name: "Power Produced Phase 3"
      accuracy_decimals: 3
    gas_delivered:
      name: "Gas Consumed"
      state_class: total_increasing
  - platform: uptime
    name: "Sensorbox Uptime"
    update_interval: 30s
  - platform: wifi_signal
    name: "Sensorbox Wi-Fi Signal"
    update_interval: 60s
  - platform: custom
    lambda: |-
      return { id(sensorbox)->ct1_current_, id(sensorbox)->ct2_current_, id(sensorbox)->ct3_current_};
    # Sensor names for PIC connected current clamps in HA, you can change these if you want
    # You do not have to set use_ct_readings to true to get these readings
    # This means you can use them for other purposes as well
    sensors:
      - name: Current CT 1
      - name: Current CT 2
      - name: Current CT 3

text_sensor:
  - platform: dsmr
    identification:
      id: dsmr_identification
      name: "DSMR Identification"
    p1_version:
      id: dsmr_version
      name: "DSMR Version"
  - platform: wifi_info
    ip_address:
      name: "Sensorbox IP Address"
    ssid:
      name: "Sensorbox Wi-Fi SSID"
    bssid:
      name: "Sensorbox Wi-Fi BSSID"
  - platform: version
    name: "ESPHome Version"
    hide_timestamp: true
```

```
MIT License

Copyright (c) 2022 Jorrit Pouw

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
