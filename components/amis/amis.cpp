#include "amis.h"
#include "aes.h"
#include "esphome/core/log.h"
#include <sstream>
#include <iomanip>

namespace esphome {
namespace amis {

static const char *TAG = "amis";

#define CHR2BIN(c) (c-(c>='A'?55:48))

#define OFFS_DIF 19

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

uint8_t amis::AMISComponent::dif2len(uint8_t dif) {
  switch (dif&0x0F)
  {
    case 0x0:
      return 0;
    case 0x1:
      return 1;
    case 0x2:
      return 2;
    case 0x3:
      return 3;
    case 0x4:
      return 4;
    case 0x5:
      return 4;
    case 0x6:
      return 6;
    case 0x7:
      return 8;
    case 0x8:
      return 0;
    case 0x9:
      return 1;
    case 0xA:
      return 2;
    case 0xB:
      return 3;
    case 0xC:
      return 4;
    case 0xD: 
      // variable data length, 
      // data length stored in data field
      return 0;
    case 0xE:
      return 6;
    case 0xF:
      return 8;

    default: // never reached
      return 0x00;
  }
}

void amis::AMISComponent::amis_decode() {
  char cs=0;
  int i;
  uint32_t temp;
  struct tm t;
  uint8_t dif;
  uint8_t vif;
  uint8_t dife;
  uint8_t vife;
  uint8_t data_len;

  if(this->bytes < 78) {
    ESP_LOGD(TAG, "received incomplete frame");
    goto out;
  }

  for(i=4; i<bytes-2; i++)
    cs += this->buffer[i];

  if(cs == this->buffer[this->bytes-2]) {
    ESP_LOGD(TAG, "checksum correct, decrypting");

    this->iv[0] = this->buffer[11];
    this->iv[1] = this->buffer[12];
    this->iv[2] = this->buffer[7];
    this->iv[3] = this->buffer[8];
    this->iv[4] = this->buffer[9];
    this->iv[5] = this->buffer[10];
    this->iv[6] = this->buffer[13];
    this->iv[7] = this->buffer[14];
    for(int i=8; i<16; i++)
      this->iv[i] = this->buffer[15];

    AES128_CBC_decrypt_buffer(this->decode_buffer,      this->buffer + OFFS_DIF + 0,  16, this->key, this->iv);
    AES128_CBC_decrypt_buffer(this->decode_buffer + 16, this->buffer + OFFS_DIF + 16, 16, 0, 0);
    AES128_CBC_decrypt_buffer(this->decode_buffer + 32, this->buffer + OFFS_DIF + 32, 16, 0, 0);
    AES128_CBC_decrypt_buffer(this->decode_buffer + 48, this->buffer + OFFS_DIF + 48, 16, 0, 0);
    AES128_CBC_decrypt_buffer(this->decode_buffer + 64, this->buffer + OFFS_DIF + 64, 16, 0, 0);

    if(this->decode_buffer[0] != 0x2f || this->decode_buffer[1] != 0x2f) {
      ESP_LOGD(TAG, "decryption sanity check failed.");
      goto out;
    }

    // https://github.com/volkszaehler/vzlogger/blob/master/src/protocols/MeterOMS.cpp
    // line 591



    i = 2;
    // 80 is the maximum size of data that we decrypt
    while(i < 80) {
      dif = this->decode_buffer[i];
      if(dif == 0x0f or dif == 0x1f) {
        ESP_LOGE(TAG, "Variable length not supported.");
        goto out;
      }
      dife = 0;
      vife = 0;
      data_len = this->dif2len(dif);
      
      if(dif & 0x80) {
        while(this->decode_buffer[i] & 0x80) {
          dife = this->decode_buffer[i+1];
          i++;
        }
      }

      i++;

      vif = this->decode_buffer[i];
      if(vif == 0x7c) {
        ESP_LOGE(TAG, "Variable length vif not supported.");
        goto out;
      }
      
      if(vif & 0x80) {
        while(this->decode_buffer[i] & 0x80) {
          vife = this->decode_buffer[i+1];
          i++;
        }
      }
      if((dif & 0x0f) == 0x0d) {
        ESP_LOGE(TAG, "Variable length data not supported.");
        goto out;
      }
      
      i++;
      
      switch(vif) {
        case 0x6d:
          t.tm_sec = this->decode_buffer[i] & 0x3f;
          t.tm_min = this->decode_buffer[i+1] & 0x3f;
          t.tm_hour = this->decode_buffer[i+2] & 0x1f;
          t.tm_mday = this->decode_buffer[i+3] & 0x1f;
          t.tm_mon = this->decode_buffer[i+4] & 0xf;
          if(t.tm_mon > 0)
            t.tm_mon -= 1;
          t.tm_year = 100 + (((this->decode_buffer[i+3] & 0xe0) >> 5) | ((this->decode_buffer[i+4] & 0xf0) >> 1));
          t.tm_isdst = ((this->decode_buffer[i] & 0x40) == 0x40) ? 1 : 0;

          if((this->decode_buffer[i+1] & 0x80) == 0x80) {
            ESP_LOGD(TAG, "time invalid");
            goto out;
          } else {
            ESP_LOGD(TAG, "time=%.2d-%.2d-%.2d %.2d:%.2d:%.2d",
                 1900 + t.tm_year, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
            ESP_LOGD(TAG, "timestamp=%ld", mktime(&t));
          }
          if(this->timestamp_sensor)
            this->timestamp_sensor->publish_state(mktime(&t));
        break;
        case 0x03:
          if(dif == 0x04) {
            // 1.8.0
            memcpy(&temp, &this->decode_buffer[i], data_len);
            ESP_LOGD(TAG, "1.8.0: %d", temp);
            if(this->energy_a_positive_sensor)
              this->energy_a_positive_sensor->publish_state(temp);
          }
        break;
        case 0x83:
          if(dif == 0x04 && vife == 0x3c) {
            // 2.8.0
            memcpy(&temp, &this->decode_buffer[i], data_len);
            ESP_LOGD(TAG, "2.8.0: %d", temp);
            if(this->energy_a_negative_sensor)
              this->energy_a_negative_sensor->publish_state(temp);
          }
        break;
        case 0xfb:
          if(dif == 0x84 && dife == 0x10 && vife == 0x73) {
            // 3.8.1
            memcpy(&temp, &this->decode_buffer[i], data_len);
            ESP_LOGD(TAG, "3.8.1: %d", temp);
            if(this->reactive_energy_a_positive_sensor)
              this->reactive_energy_a_positive_sensor->publish_state(temp);
          }
          if(dif == 0x84 && dife == 0x10 && vife == 0x3c) {
            // 4.8.1
            memcpy(&temp, &this->decode_buffer[i], data_len);
            ESP_LOGD(TAG, "4.8.1: %d", temp);
            if(this->reactive_energy_a_negative_sensor)
              this->reactive_energy_a_negative_sensor->publish_state(temp);
          }
          if(dif == 0x04 && dife == 0x00 && vife == 0x14) {
            // 3.7.0
            memcpy(&temp, &this->decode_buffer[i], data_len);
            ESP_LOGD(TAG, "3.7.0: %d", temp);
            if(this->reactive_instantaneous_power_a_positive_sensor)
              this->reactive_instantaneous_power_a_positive_sensor->publish_state(temp);
          }
          if(dif == 0x04 && dife == 0x00 && vife == 0x3c) {
            // 4.7.0
            memcpy(&temp, &this->decode_buffer[i], data_len);
            ESP_LOGD(TAG, "4.7.0: %d", temp);
            if(this->reactive_instantaneous_power_a_negative_sensor)
              this->reactive_instantaneous_power_a_negative_sensor->publish_state(temp);
          }
        break;
        case 0x2b:
          if(dif == 0x04) {
            // 1.7.0
            memcpy(&temp, &this->decode_buffer[i], data_len);
            ESP_LOGD(TAG, "1.7.0: %d", temp);
            if(this->instantaneous_power_a_positive_sensor)
              this->instantaneous_power_a_positive_sensor->publish_state(temp);
          }
        break;
        case 0xab:
          if(dif == 0x04 && vife == 0x3c) {
            // 2.7.0
            memcpy(&temp, &this->decode_buffer[i], data_len);
            ESP_LOGD(TAG, "2.7.0: %d", temp);
            if(this->instantaneous_power_a_negative_sensor)
              this->instantaneous_power_a_negative_sensor->publish_state(temp);
          }
        break;
      }
      
      
      i += data_len;
    }

  } else {
    ESP_LOGD(TAG, "check bad");
  }

out:
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

    if(this->expect && this->bytes >= this->expect) {
      ESP_LOGD(TAG, "amis_decode");
      this->amis_decode();
    }
  }
}

}  // namespace amis
}  // namespace esphome
