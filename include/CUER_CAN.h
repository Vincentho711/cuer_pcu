#ifndef CUER_CAN_H
#define CUER_CAN_H

#include "mbed.h"

#define CAN_TIMEOUT_MS 100      // Defines how long to wait before deciding to timeout the CAN message sending


// ----------------------------------------------
// Variables

extern CAN can1;
extern bool CAN_data_sent;

// ----------------------------------------------
// Function Prototypes

bool can_send(CANMessage msg);
void CANDataSentCallback(void);

#endif
