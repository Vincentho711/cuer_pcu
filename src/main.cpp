/*****************************************************************************************************\
 CUER PCU Code by Vincent Ho (April 2022)

 This code works for both the PCU in the front and rear battery pack. The CAN ID for their status 
 message is set by the physical DIP switches on the board.

 Functionality:

    Main contactors control: The main contactors are responsible for connecting/disconnecting the pos and 
    neg wires of each battery pack to its output port.
    The state of the main contactors are controlled based on the CAN message from the BMU. 
    It compares the desired state to the current state of the HVDC contactor and make a decision.

    Check timeouts from BMU: Check if it's been too long since BMU has communicated through CAN. The main
    contactors will open if the BMU status message hasn't been received for 5s.
    
    Cell voltage measurements and broadcast: Interface with the LTC68XX chip to read cell voltages and send them
    onto the CAN bus for BMU to interpret.

\*****************************************************************************************************/

#include <mbed.h>
#include <chrono>
#include "Cell_Voltage.h"
#include "SPI_Parser.h"
#include "CUER_CAN.h"
#include "PCU.h"

// DEBUG flag
#define PCU_DEBUG 1 

// ----------------------------------------------
// Pin Definitions

// Contactor Configuration
DigitalOut contactorNeg(p6);
DigitalOut contactorPos(p5);

// Fan Configuration
PwmOut fan1(p21);
PwmOut fan2(p22);
PwmOut fan3(p23);
PwmOut fan4(p25);
DigitalOut fan3_enable(p24);

// DIP Configuration
DigitalIn DIP1(p26);
DigitalIn DIP2(p27);
DigitalIn DIP3(p28);
DigitalIn DIP4(p8);
DigitalIn DIP5(p7);

// Buzzer Configuration
DigitalOut buzzer(p10);

// LPC on board LED
DigitalOut myled(LED1);

// Serial interface
// Serial is decaprecated in mbed os 6
// Serial pc(USBTX, USBRX);

// CAN interface
CAN can1(p30, p29);

// Chrono-based elapsed_time for timer class
using namespace std::chrono;

// ----------------------------------------------
// Variable Definitions



// Global variable for PCU state and desired state
PCUState currentState;
PCUState desiredState;
uint16_t PCU_STATUS_ID;

// CAN Configuration
bool CAN_data_sent;
char message[8];
CANMessage msg;
CANMessage reccie;

// Ticker Configuration
Ticker heartbeat;
// tickerInterval = 1s = 1000ms
int tickerInterval = 1000;
volatile bool beating = false;
volatile bool can_read_flag = false;
uint8_t BMUTimeOut = 0;

// Cell Voltage Measurement
uint16_t cellvoltages[NO_CMUS][12]; // Contains the cell voltages read from the LTC6804
uint16_t cellVoltageID[4];          // CAN IDs to send the voltages to
uint16_t cellVoltageIDFront[4] = {CELL_VOLTAGES_01_04_ID, CELL_VOLTAGES_05_08_ID, CELL_VOLTAGES_09_12_ID, CELL_VOLTAGES_13_16_ID};
uint16_t cellVoltageIDRear[4] = {CELL_VOLTAGES_18_21_ID, CELL_VOLTAGES_22_25_ID, CELL_VOLTAGES_26_29_ID, CELL_VOLTAGES_30_33_ID};

// ----------------------------------------------
// Function Prototypes
void init (void);
void CANRecieveRoutine (void);
void Heartbeat (void);
void ChangeContactor (CANMessage recieved);
void ChangeFan(CANMessage recieved);
void UpdateFans (void);

// ----------------------------------------------
// Main
int main() {
	// One time setup code
	init();

	// Main loop
	while(1) {

		// Update the status of the contactors
		if ((desiredState.contactorState == CONTACTORS_CLOSED) && (BMUTimeOut < 5)){
            if (PCU_DEBUG)
            {
                printf("Main contactors are closed. \n");
            }
			contactorNeg = CONTACTORS_CLOSED;
			contactorPos = CONTACTORS_CLOSED;
		}
		if ((desiredState.contactorState == CONTACTORS_OPEN) || (BMUTimeOut >= 5)){
            if (PCU_DEBUG)
            {
                printf("Main contactors are open. \n");
            }
			contactorNeg = CONTACTORS_OPEN;
			contactorPos = CONTACTORS_OPEN;
		}

		if (currentState.contactorState != desiredState.contactorState){
			currentState.contactorState = desiredState.contactorState;
		}


		// Update the status of the fans
		UpdateFans();


		// If BMU is not detected, ensure the contactor state is open
		// TODO should be able to remove, see above or statment
		if (BMUTimeOut >= 5){
            if (PCU_DEBUG)
            {
                printf("Main contactors opened due to BMU timeout. \n");
            }
			desiredState.contactorState = CONTACTORS_OPEN;
			currentState.contactorState = CONTACTORS_OPEN;
		}


		// If contactors were changed, send status packet
		if (desiredState.updateReason == CONTACTORS_CHANGED){
			//Send the status update CAN message because contactors have been updated
			message[0] = currentState.contactorState;
			message[1] = currentState.fan1duty;
			message[2] = currentState.fan2duty;
			message[3] = currentState.fan3duty;
			message[4] = currentState.fan4duty;
			message[5] = currentState.config;
			message[6] = currentState.buzzerState;
			message[7] = desiredState.updateReason;
			msg = CANMessage(PCU_STATUS_ID, message, 8);

			can_send(CANMessage(msg));
			desiredState.updateReason = CONTACTORS_UNCHANGED;   // Have just sent ack message, reset to return to sending ticker updates
		}

		// If ticker has fired send status + the rest of the PCU goodies 
		if (beating == true){
			beating = false;

			//Send the status update CAN message with update reason: heartbeat ---
			message[0] = currentState.contactorState;
			message[1] = currentState.fan1duty;
			message[2] = currentState.fan2duty;
			message[3] = currentState.fan3duty;
			message[4] = currentState.fan4duty;
			message[5] = currentState.config;
			message[6] = currentState.buzzerState;
			message[7] = desiredState.updateReason;
			msg = CANMessage(PCU_STATUS_ID, message, 8);

			can_send(CANMessage(msg));

			// Voltage update ---
			// Use LTC6804_acquireVoltage to read from the CMUs
			LTC6804_acquireVoltage(cellvoltages, NO_CMUS);
			//printCellVoltages(cellvoltages);      // For debug, prints the cell voltages to serial

			// Send the cell voltages in 5 CAN packets
			SendCellVoltages(cellvoltages, cellVoltageID);


			//Power consumption update ---
			//TODO
		}        

		if (can_read_flag) {
			can_read_flag = false;

			int result = can1.read(reccie);
			switch(reccie.id) {
				case BMU_UPDATE_CONTACTORS :
					ChangeContactor(reccie);
					break;
				case BMU_UPDATE_FRONT_FANS :
					if(DIP1 == FRONT){
						ChangeFan(reccie);
					}
					break;
				case BMU_UPDATE_REAR_FANS :
					if(DIP1 == REAR){
						ChangeFan(reccie);
					}
					break;
				case BMU_STATUS :
					BMUTimeOut = 0; // BMU has shown itself and the timeout can be reset
					break;
			}
		}
	}
}

// ----------------------------------------------
// Functions

void init (void)
{
	// Turn off the damn buzzer
	buzzer = 0;

	// Ensure contactors are open to begin
	contactorNeg = CONTACTORS_OPEN;
	contactorPos = CONTACTORS_OPEN;

	// Initialise CAN
	can1.frequency(500000);
	/* can1.attach(&CANRecieveRoutine, CAN::RxIrq); // receive interrupt handler */
	can1.attach(&CANDataSentCallback, CAN::TxIrq); // send interrupt handler
	CAN_data_sent = false; //initialize CAN status

	// Initiliase 1s heartbeat interrupt
	heartbeat.attach(&Heartbeat, milliseconds(tickerInterval));

	// Initialise LCT6804
	LTC6804_init(MD_FAST, DCP_DISABLED, CELL_CH_ALL, AUX_CH_VREF2);

	// Initialise all fans to off
	fan1.period_us(40);
	fan2.period_us(40);
	fan3.period_us(40);
	fan4.period_us(40);

	fan3_enable = 1;
	currentState.fan1duty = 0.0f;
	currentState.fan2duty = 0.0f;
	currentState.fan3duty = 0.0f;
	currentState.fan4duty = 0.0f;

	fan1.write(1.0 - (float(currentState.fan1duty) / 255));
	fan2.write(1.0 - (float(currentState.fan2duty) / 255));
	fan3.write(1.0 - (float(currentState.fan3duty) / 255));
	fan4.write(1.0 - (float(currentState.fan4duty) / 255));

	// Decide whether this is front or rear pack and change CAN ID's appropriately
	if(DIP1 == FRONT){                
		PCU_STATUS_ID = PCU_STATUS_FRONT_ID;
	}
	else{
		if(DIP1 == REAR){
			PCU_STATUS_ID = PCU_STATUS_REAR_ID;
		}
	}

	if(DIP1 == FRONT){
		for(int i = 0; i < 5; i++){              
			cellVoltageID[i] = cellVoltageIDFront[i];
		}
	}
	else{
		if(DIP1 == REAR){
			for(int i = 0; i < 5; i++){
				cellVoltageID[i] = cellVoltageIDRear[i];
			}
		}
	}
}

void CANRecieveRoutine (void)
{
	can_read_flag = true;
}

void ChangeContactor(CANMessage recieved)
{
	if (recieved.data[0] == 0b1){
        if (PCU_DEBUG)
        {
                printf("BMU decides that main contactors should be closed. \n");
        }
		desiredState.contactorState = CONTACTORS_CLOSED;     // Close contactors
	}
	else{
        if (PCU_DEBUG)
        {
                printf("BMU decides that main contactors should be opened. \n");
        }
		if (recieved.data[0] == 0b0){
			desiredState.contactorState = CONTACTORS_OPEN;    // Open contactors
		}
	}
	desiredState.updateReason = CONTACTORS_CHANGED; // Reason for update - ack
}

void ChangeFan(CANMessage recieved){
	desiredState.fan1duty = recieved.data[0];
	desiredState.fan2duty = recieved.data[1];
	desiredState.fan3duty = recieved.data[2];
	desiredState.fan4duty = recieved.data[3];
}

void Heartbeat(void){
	beating = true;
	BMUTimeOut++;   // Increment the BMU timeout counter by 1 second
}

void UpdateFans (void)
{
	if (BMUTimeOut >= 5){
		desiredState.fan1duty = 0.0;
		desiredState.fan2duty = 0.0;
		desiredState.fan3duty = 0.0;
		desiredState.fan4duty = 0.0;
	}

	fan1.write(1.0 - (float(desiredState.fan1duty) / 255));
	fan2.write(1.0 - (float(desiredState.fan2duty) / 255));
	fan3.write(1.0 - (float(desiredState.fan3duty) / 255));
	fan4.write(1.0 - (float(desiredState.fan4duty) / 255));
	currentState.fan1duty = desiredState.fan1duty;
	currentState.fan2duty = desiredState.fan2duty;
	currentState.fan3duty = desiredState.fan3duty;
	currentState.fan4duty = desiredState.fan4duty;    
}