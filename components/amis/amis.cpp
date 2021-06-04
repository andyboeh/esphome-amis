#include "amis.h"
#include "aes.h"
#include "esphome/core/log.h"

namespace esphome {
namespace amis {

static const char *TAG = "amis";

#define CHR2BIN(c) (c-(c>='A'?55:48))

#define OFFS_DIF 19

#define OFFS_180 12
#define OFFS_280 19
#define OFFS_381 28
#define OFFS_481 38
#define OFFS_170 44
#define OFFS_270 51
#define OFFS_370 58
#define OFFS_470 66
#define OFFS_11280 74

// Probieren wir das mal ohne Poll, mit loop() und sehen, wie schnell oder langsam
// da die Daten kommen. Ggf. müssen wir das etwas verlangsamen, aber loop() sollte
// etwa alle 10ms aufgerufen werden - das wäre eh passend. 
// Setup brauchen wir dann nicht, weil wir auf den ersten Discover() Frame 
// warten müssen, bevor er anspricht. dann 0x5e senden und "echte" Daten
// müssten kommen. AES decodieren und fertig.

void amis::AMISComponent::setup() {
  this->bytes = 0;
  this->expect = 0;

}

void amis::AMISComponent::hex2bin(const std::string s, uint8_t *buf) {
  unsigned len = s.length();
  if (len != 32) return;

  for (unsigned i=0; i<len; ++i) {
    buf[i] = CHR2BIN(s.c_str()[i*2])<<4 | CHR2BIN(s.c_str()[i*2+1]);
  }
}

void amis::AMISComponent::set_power_grid_key(const std::string &power_grid_key) {
  this->hex2bin(power_grid_key, this->key);
}

void amis::AMISComponent::amis_decode() {
  char cs=0;
  int i;

  for(int i=4; i<bytes-2; i++)
    cs += this->buffer[i];

  if(cs == this->buffer[this->bytes-2]) {
    ESP_LOGD(TAG, "checksum correct, decrypting");
    memcpy(this->iv, &this->buffer[7], 8);
    for(int i=8; i<16; i++)
      this->iv[i] = this->buffer[15];

    AES128_CBC_decrypt_buffer(this->decode_buffer,      this->buffer + OFFS_DIF + 0,  16, this->key, this->iv);
    AES128_CBC_decrypt_buffer(this->decode_buffer + 16, this->buffer + OFFS_DIF + 16, 16, 0, 0);
    AES128_CBC_decrypt_buffer(this->decode_buffer + 32, this->buffer + OFFS_DIF + 32, 16, 0, 0);
    AES128_CBC_decrypt_buffer(this->decode_buffer + 48, this->buffer + OFFS_DIF + 48, 16, 0, 0);
    AES128_CBC_decrypt_buffer(this->decode_buffer + 64, this->buffer + OFFS_DIF + 64, 16, 0, 0);

    yield();

    //FIXME: If we receive duplicate timestamps, we should throught the value away!
    // https://github.com/volkszaehler/vzlogger/blob/master/src/protocols/MeterOMS.cpp
    // siehe dort Zeile 591
    ESP_LOGD(TAG, "0x%02x%02x%02x%02x%02x", this->decode_buffer[8], this->decode_buffer[7], this->decode_buffer[6], this->decode_buffer[5], this->decode_buffer[4]);
    ESP_LOGD(TAG, "dow: %d", this->decode_buffer[6] >> 5);

    memcpy(&this->a_result[0], this->decode_buffer + OFFS_180, 4);
    memcpy(&this->a_result[1], this->decode_buffer + OFFS_280, 4);
    memcpy(&this->a_result[2], this->decode_buffer + OFFS_381, 4);
    memcpy(&this->a_result[3], this->decode_buffer + OFFS_481, 4);
    memcpy(&this->a_result[4], this->decode_buffer + OFFS_170, 4);
    memcpy(&this->a_result[5], this->decode_buffer + OFFS_270, 4);
    memcpy(&this->a_result[6], this->decode_buffer + OFFS_370, 4);
    memcpy(&this->a_result[7], this->decode_buffer + OFFS_470, 4);
    memcpy(&this->a_result[8], this->decode_buffer + OFFS_11280, 4);
    
    ESP_LOGD(TAG, "OFFS_180: %d", this->a_result[0]);
    ESP_LOGD(TAG, "OFFS_280: %d", this->a_result[1]);
    ESP_LOGD(TAG, "OFFS_381: %d", this->a_result[2]);
    ESP_LOGD(TAG, "OFFS_481: %d", this->a_result[3]);
    ESP_LOGD(TAG, "OFFS_170: %d", this->a_result[4]);
    ESP_LOGD(TAG, "OFFS_270: %d", this->a_result[5]);
    ESP_LOGD(TAG, "OFFS_370: %d", this->a_result[6]);
    ESP_LOGD(TAG, "OFFS_470: %d", this->a_result[7]);
    ESP_LOGD(TAG, "OFFS_11280: %d", this->a_result[8]);
    
    if(this->energy_a_positive_sensor)
      this->energy_a_positive_sensor->publish_state(this->a_result[0]);
    if(this->energy_a_negative_sensor)
      this->energy_a_negative_sensor->publish_state(this->a_result[1]);
    if(this->reactive_energy_a_positive_sensor)
      this->reactive_energy_a_positive_sensor->publish_state(this->a_result[2]);
    if(this->reactive_energy_a_negative_sensor)
      this->reactive_energy_a_negative_sensor->publish_state(this->a_result[3]);
    if(this->instantaneous_power_a_positive_sensor)
      this->instantaneous_power_a_positive_sensor->publish_state(this->a_result[4]);
    if(this->instantaneous_power_a_negative_sensor)
      this->instantaneous_power_a_negative_sensor->publish_state(this->a_result[5]);
    if(this->reactive_instantaneous_power_a_positive_sensor)
      this->reactive_instantaneous_power_a_positive_sensor->publish_state(this->a_result[6]);
    if(this->reactive_instantaneous_power_a_negative_sensor)
      this->reactive_instantaneous_power_a_negative_sensor->publish_state(this->a_result[7]);
    

  } else {
    ESP_LOGD(TAG, "check bad");
  }

  this->bytes = 0;
  this->expect = 0;

  this->write_byte(0xe5);
}

void amis::AMISComponent::dump_config() {

}

void amis::AMISComponent::loop() {
  // This is the polling routine
  // Do we actually need a loop?
  uint8_t cnt = this->available();
  while (cnt > 0) {
    ESP_LOGD(TAG, "bytes available, reading");
    if((this->bytes + cnt) < sizeof(this->buffer)) {
	  this->read_array(&this->buffer[bytes], cnt);
	  bytes += cnt;
  
	  cnt = this->available();
    } else {
      ESP_LOGD(TAG, "rcv'd incomplete frame, clearing buffer");
      while(cnt > 0) {
        this->read_array(this->buffer, cnt);
        cnt = this->available();
      }
      cnt = 0;
      this->bytes = 0;
    }
    
    if(this->bytes >= 5 && this->expect == 0) {
      if(memcmp(this->buffer,"\x10\x40\xF0\x30\x16", 5) == 0) {
        ESP_LOGD(TAG, "ack'ed frame");
        this->write_byte(0xe5);
        this->bytes = 0;
      } else {
        if(this->buffer[0] == 0x68 && this->buffer[3] == 0x68) {
          this->expect = this->buffer[1] + 6;
        }
      }
    }

    if(this->expect && this->bytes >= expect) {
      ESP_LOGD(TAG, "amis_decode");
      this->amis_decode();
    }
  }
}

}  // namespace amis
}  // namespace esphome
