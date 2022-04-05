#include "PCU.h"


// ----------------------------------------------
// Functions

void SendCellVoltages(uint16_t cellvoltages[NO_CMUS][12], uint16_t addresses[4])
{
    CANMessage msg;
    char message[8];
    
    message[0] = cellvoltages[0][0] & 0xFF;
    message[1] = (cellvoltages[0][0] >> 8) & 0xFF;
    message[2] = cellvoltages[0][1] & 0xFF;
    message[3] = (cellvoltages[0][1] >> 8) & 0xFF;
    message[4] = cellvoltages[0][2] & 0xFF;
    message[5] = (cellvoltages[0][2] >> 8) & 0xFF;
    message[6] = cellvoltages[0][3] & 0xFF;
    message[7] = (cellvoltages[0][3] >> 8) & 0xFF;
    msg = CANMessage(addresses[0], message, 8);
    can_send(CANMessage(msg));
    
    message[0] = cellvoltages[0][4] & 0xFF;
    message[1] = (cellvoltages[0][4] >> 8) & 0xFF;
    message[2] = cellvoltages[0][5] & 0xFF;
    message[3] = (cellvoltages[0][5] >> 8) & 0xFF;
    message[4] = cellvoltages[0][6] & 0xFF;
    message[5] = (cellvoltages[0][6] >> 8) & 0xFF;
    message[6] = cellvoltages[0][7] & 0xFF;
    message[7] = (cellvoltages[0][7] >> 8) & 0xFF;
    msg = CANMessage(addresses[1], message, 8);
    can_send(CANMessage(msg));
    
    message[0] = cellvoltages[0][8] & 0xFF;
    message[1] = (cellvoltages[0][8] >> 8) & 0xFF;
    message[2] = cellvoltages[0][9] & 0xFF;
    message[3] = (cellvoltages[0][9] >> 8) & 0xFF;
    message[4] = cellvoltages[0][10] & 0xFF;
    message[5] = (cellvoltages[0][10] >> 8) & 0xFF;
    message[6] = cellvoltages[0][11] & 0xFF;
    message[7] = (cellvoltages[0][11] >> 8) & 0xFF;
    msg = CANMessage(addresses[2], message, 8);
    can_send(CANMessage(msg));
     
    message[0] = cellvoltages[1][0] & 0xFF;
    message[1] = (cellvoltages[1][0] >> 8) & 0xFF;
    message[2] = cellvoltages[1][1] & 0xFF;
    message[3] = (cellvoltages[1][1] >> 8) & 0xFF;
    message[4] = cellvoltages[1][2] & 0xFF;
    message[5] = (cellvoltages[1][2] >> 8) & 0xFF;
    message[6] = cellvoltages[1][3] & 0xFF;
    message[7] = (cellvoltages[1][3] >> 8) & 0xFF;
    msg = CANMessage(addresses[3], message, 8);
    can_send(CANMessage(msg));
}

void PrintCellVoltages(uint16_t cellvoltages[NO_CMUS][12])
{
    printf("Cell 17 voltage: %i\r\n", cellvoltages[1][4]);
    printf("Cell 16 voltage: %i\r\n", cellvoltages[1][3]);
    printf("Cell 15 voltage: %i\r\n", cellvoltages[1][2]);
    printf("Cell 14 voltage: %i\r\n", cellvoltages[1][1]);
    printf("Cell 13 voltage: %i\r\n", cellvoltages[1][0]);
    printf("Cell 12 voltage: %i\r\n", cellvoltages[0][11]);
    printf("Cell 11 voltage: %i\r\n", cellvoltages[0][10]);
    printf("Cell 10 voltage: %i\r\n", cellvoltages[0][9]);
    printf("Cell 9  voltage: %i\r\n", cellvoltages[0][8]);
    printf("Cell 8  voltage: %i\r\n", cellvoltages[0][7]);
    printf("Cell 7  voltage: %i\r\n", cellvoltages[0][6]);
    printf("Cell 6  voltage: %i\r\n", cellvoltages[0][5]);
    printf("Cell 5  voltage: %i\r\n", cellvoltages[0][4]);
    printf("Cell 4  voltage: %i\r\n", cellvoltages[0][3]);
    printf("Cell 3  voltage: %i\r\n", cellvoltages[0][2]);
    printf("Cell 2  voltage: %i\r\n", cellvoltages[0][1]);
    printf("Cell 1  voltage: %i\r\n", cellvoltages[0][0]);
}

