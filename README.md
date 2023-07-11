# esphome-components
Custom components for ESPHome

## Pentair Easytouch Component
This custom component provides an interface to a Pentair Easytouch controller via an RS232/RS485 connection.

### Hardware
TODO

### Usage
Include the following in your yaml file:
```yaml
external_components:
  - source: github://michaelusner/esphome-components

uart:
  id: uart_1
  tx_pin: GPIO16
  rx_pin: GPIO17
  baud_rate: 9600

pentair_easytouch:
  id: pentair_easytouch_component_1
  uart_id: uart_1
  flow_control_pin: GPIO4
  air_temperature_sensor:
    name: Air Temperature
    unit_of_measurement: °F
  water_temperature_sensor:
    name: Water Temperature
    unit_of_measurement: °F
  pump1_switch:
    name: Pump 1
    address: 0x60
```