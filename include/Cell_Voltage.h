#ifndef Cell_Voltage_H
#define Cell_Voltage_H

#include <stdint.h>
#include "SPI_Parser.h"
#include "mbed.h"

#define NO_READINGS_PER_CMU 12
#define NO_CMUS 2

void LTC6804_acquireVoltage(uint16_t cellcodes [][NO_READINGS_PER_CMU], uint8_t cmu_count);
void LTC6804_balance(const uint8_t ic, const uint8_t total_ic, uint8_t states[NO_READINGS_PER_CMU]);
void LTC6804_balanceVoltage(uint16_t voltages[][NO_READINGS_PER_CMU], uint16_t maxVoltage, uint8_t cmu_count);


#endif

