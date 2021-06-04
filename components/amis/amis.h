#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace amis {


class AMISComponent : public Component, public uart::UARTDevice {
 public:
  void hex2bin(const std::string s, uint8_t *buf);
  void setup() override;
  void dump_config() override;
  void loop() override;
  void amis_decode();
  void set_power_grid_key(const std::string &power_grid_key);
  void set_energy_a_positive_sensor(sensor::Sensor *sensor) {
    this->energy_a_positive_sensor = sensor;
  }
  void set_energy_a_negative_sensor(sensor::Sensor *sensor) {
    this->energy_a_negative_sensor = sensor;
  }
  void set_reactive_energy_a_positive_sensor(sensor::Sensor *sensor) {
    this->reactive_energy_a_positive_sensor = sensor;
  }
  void set_reactive_energy_a_negative_sensor(sensor::Sensor *sensor) {
    this->reactive_energy_a_negative_sensor = sensor;
  }
  void set_instantaneous_power_a_positive_sensor(sensor::Sensor *sensor) {
    this->instantaneous_power_a_positive_sensor = sensor;
  }
  void set_instantaneous_power_a_negative_sensor(sensor::Sensor *sensor) {
    this->instantaneous_power_a_negative_sensor = sensor;
  }
  void set_reactive_instantaneous_power_a_positive_sensor(sensor::Sensor *sensor) {
    this->reactive_instantaneous_power_a_positive_sensor = sensor;
  }
  void set_reactive_instantaneous_power_a_negative_sensor(sensor::Sensor *sensor) {
    this->reactive_instantaneous_power_a_negative_sensor = sensor;
  }

  float get_setup_priority() const override { return setup_priority::DATA; }

 protected:
  int bytes;
  uint8_t buffer[256];
  uint8_t decode_buffer[128];
  int expect;
  uint8_t iv[16];
  uint8_t key[16];
  uint32_t a_result[9];
  sensor::Sensor *energy_a_positive_sensor{nullptr};
  sensor::Sensor *energy_a_negative_sensor{nullptr};
  sensor::Sensor *reactive_energy_a_positive_sensor{nullptr};
  sensor::Sensor *reactive_energy_a_negative_sensor{nullptr};
  sensor::Sensor *instantaneous_power_a_positive_sensor{nullptr};
  sensor::Sensor *instantaneous_power_a_negative_sensor{nullptr};
  sensor::Sensor *reactive_instantaneous_power_a_positive_sensor{nullptr};
  sensor::Sensor *reactive_instantaneous_power_a_negative_sensor{nullptr};
};

}  // namespace rdm6300
}  // namespace esphome
