/*
    Si115X_SAMD21.cpp
    As long as the manufacturer or someone else doesn't fix the official lib

    Author: Carlos Alberto
    Modified Time: November 2022

    Patched from 0hotpotman0/Si115x_arduino
    https://github.com/0hotpotman0/Si115x_arduino/blob/master/Si115X.cpp
*/

#include <Arduino.h>
#include "Si115X_SAMD21.h"

Si115X::Si115X() {
  // empty constructor
}

void Si115X::config_channel(uint8_t index, uint8_t *conf) {
    int len = sizeof(conf);

    if(len != 4 || index < 0 || index > 5)
      return;

    int inc = index * len;

    param_set(Si115X::ADCCONFIG_0 + inc, conf[0]);
    param_set(Si115X::ADCSENS_0 + inc, conf[1]);
    param_set(Si115X::ADCPOST_0 + inc, conf[2]);
    param_set(Si115X::MEASCONFIG_0 + inc, conf[3]);
}

void Si115X::write_data(uint8_t addr, uint8_t *data, size_t len) {
    Wire.beginTransmission(addr);
    Wire.write(data, len);
    Wire.endTransmission();
}

int Si115X::read_register(uint8_t addr, uint8_t reg, int bytesOfData) {
    int val = -1;

    Si115X::write_data(addr, &reg, sizeof(reg));
    Wire.requestFrom(addr, bytesOfData);

    if(Wire.available())
      val = Wire.read();

    return val;
}

void Si115X::param_set(uint8_t loc, uint8_t val) {
    uint8_t packet[2];
    int r;
    int cmmnd_ctr;

    do {
        cmmnd_ctr = Si115X::read_register(Si115X::DEVICE_ADDRESS, Si115X::RESPONSE_0, 1);

        packet[0] = Si115X::HOSTIN_0;
        packet[1] = val;
        Si115X::write_data(Si115X::DEVICE_ADDRESS, packet, sizeof(packet));

        packet[0] = Si115X::COMMAND;
        packet[1] = loc | (0B10 << 6);
        Si115X::write_data(Si115X::DEVICE_ADDRESS, packet, sizeof(packet));

        r = Si115X::read_register(Si115X::DEVICE_ADDRESS, Si115X::RESPONSE_0, 1);
    } while(r > cmmnd_ctr);
}

int Si115X::param_query(uint8_t loc) {
    int result = -1;
    uint8_t packet[2];
    int r;
    int cmmnd_ctr;

    do {
        cmmnd_ctr = Si115X::read_register(Si115X::DEVICE_ADDRESS, Si115X::RESPONSE_0, 1);

        packet[0] = Si115X::COMMAND;
        packet[1] = loc | (0B01 << 6);

        Si115X::write_data(Si115X::DEVICE_ADDRESS, packet, sizeof(packet));

        r = Si115X::read_register(Si115X::DEVICE_ADDRESS, Si115X::RESPONSE_0, 1);
    } while(r > cmmnd_ctr);

    result = Si115X::read_register(Si115X::DEVICE_ADDRESS, Si115X::RESPONSE_1, 1);

    return result;
}

void Si115X::send_command(uint8_t code) {
    uint8_t packet[2];
    int r;
    int cmmnd_ctr;
    do {
        cmmnd_ctr = Si115X::read_register(Si115X::DEVICE_ADDRESS, Si115X::RESPONSE_0, 1);

        packet[0] = Si115X::COMMAND;
        packet[1] = code;

        Si115X::write_data(Si115X::DEVICE_ADDRESS, packet, sizeof(packet));

        r = Si115X::read_register(Si115X::DEVICE_ADDRESS, Si115X::RESPONSE_0, 1);
    } while(r > cmmnd_ctr);
}

int Si115X::get_int_from_bytes(uint8_t *data, size_t len) {
    int result = 0;
    int shift = 8 * len;

    for(int i = 0; i < len; i++) {
        shift -= 8;
        result |= ((data[i] << shift) & (0xFF << shift));
    }

    return result;
}

bool Si115X::Begin(void) {
    Wire.begin();
    if (ReadByte(0x00) != 0x51) {
        return false;
    }
    Si115X::send_command(Si115X::START);

    Si115X::param_set(Si115X::CHAN_LIST, 0B000010);

    Si115X::param_set(Si115X::MEASRATE_H, 0);
    Si115X::param_set(Si115X::MEASRATE_L, 1);
    Si115X::param_set(Si115X::MEASCOUNT_0, 5);
    Si115X::param_set(Si115X::MEASCOUNT_1, 10);
    Si115X::param_set(Si115X::MEASCOUNT_2, 10);
    Si115X::param_set(Si115X::THRESHOLD0_L, 200);
    Si115X::param_set(Si115X::THRESHOLD0_H, 0);

    uint8_t conf[4];

////////////////// fix to SAMD21 MKR WiFi 1010 boards //////////////////

    conf[0] = 0B00000000;
    conf[1] = 0B00000010, 
    conf[2] = 0B01111000;
    conf[3] = 0B11000000;
    Si115X::config_channel(1, conf);

    conf[0] = 0B00000000;
    conf[1] = 0B00000011, 
    conf[2] = 0B01000000;
    conf[3] = 0B11000000;
    Si115X::config_channel(3, conf);

    return true;

}

uint16_t Si115X::ReadHalfWord(void) {
    Si115X::send_command(Si115X::FORCE);
    uint8_t data[3];
    data[1] = Si115X::read_register(Si115X::DEVICE_ADDRESS, Si115X::HOSTOUT_1, 0);
    data[2] = Si115X::read_register(Si115X::DEVICE_ADDRESS, Si115X::HOSTOUT_2, 1);

    return data[2];
}

float Si115X::ReadHalfWord_UV(void) {
    Si115X::send_command(Si115X::FORCE);
    uint8_t data[3];
    data[1] = Si115X::read_register(Si115X::DEVICE_ADDRESS, Si115X::HOSTOUT_1, 0);
    data[2] = Si115X::read_register(Si115X::DEVICE_ADDRESS, Si115X::HOSTOUT_2, 1);

    return ( (data[2] + data[1])/3 ) * 0.0012;
}

uint16_t Si115X::ReadHalfWord_VISIBLE(void) {
    Si115X::send_command(Si115X::FORCE);
    uint8_t data[3];
    data[1] = Si115X::read_register(Si115X::DEVICE_ADDRESS, Si115X::HOSTOUT_1, 0);
    data[2] = Si115X::read_register(Si115X::DEVICE_ADDRESS, Si115X::HOSTOUT_2, 1);

    return ( data[2] + data[1] )/3;
}

uint8_t Si115X::ReadByte(uint8_t Reg) {
    Wire.beginTransmission(Si115X::DEVICE_ADDRESS);
    Wire.write(Reg);
    Wire.endTransmission();
    Wire.requestFrom(Si115X::DEVICE_ADDRESS, 1);
    return Wire.read();
}