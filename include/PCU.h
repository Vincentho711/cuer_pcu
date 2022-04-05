#ifndef PCU_H
#define PCU_H

#include "mbed.h"
#include "Cell_Voltage.h"
#include "CUER_CAN.h"

#define CONTACTORS_CHANGED 1
#define CONTACTORS_UNCHANGED 0
#define CONTACTORS_CLOSED 1
#define CONTACTORS_OPEN 0

#define FRONT 0
#define REAR 1

struct PCUState {
    bool contactorState;
    float fan1duty;
    float fan2duty;
    float fan3duty;
    float fan4duty;
    uint8_t config;
    bool buzzerState;
    bool updateReason;
    }   ;


// ----------------------------------------------
// Variables

// Serial is decaprecated in mbed os 6
// extern Serial pc;
extern DigitalIn DIP1;
extern DigitalIn DIP2;
extern DigitalIn DIP3;
extern DigitalIn DIP4;
extern DigitalIn DIP5;


// CAN ID's
// Sending:
const int CELL_VOLTAGES_01_04_ID = 0x360;
const int CELL_VOLTAGES_05_08_ID = 0x361;
const int CELL_VOLTAGES_09_12_ID = 0x362;
const int CELL_VOLTAGES_13_16_ID = 0x363;
const int CELL_VOLTAGES_17_ID = 0x364;
const int CELL_VOLTAGES_18_21_ID = 0x365;
const int CELL_VOLTAGES_22_25_ID = 0x366;
const int CELL_VOLTAGES_26_29_ID = 0x367;
const int CELL_VOLTAGES_30_33_ID = 0x368;
const int CELL_VOLTAGES_34_ID = 0x369;
const int PCU_STATUS_FRONT_ID = 0x340;
const int PCU_STATUS_REAR_ID = 0x341;

// Recieving:
const int BMU_UPDATE_CONTACTORS = 0x34F;
const int BMU_UPDATE_FRONT_FANS = 0x350;
const int BMU_UPDATE_REAR_FANS = 0x351;
const int BMU_STATUS = 0x400;

// ----------------------------------------------
// Function Prototypes

void SendCellVoltages(uint16_t cellvoltages[NO_CMUS][12], uint16_t addresses[5]);
void printCellVoltages(uint16_t cellvoltages[NO_CMUS][12]);


#endif
