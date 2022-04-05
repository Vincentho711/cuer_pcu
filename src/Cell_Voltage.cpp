#include "Cell_Voltage.h"


/**
* Sets the values of the cellvoltage array with units 100 uV from all of the CMUs
* Uses the cmu_count parameter for the first dimension of cellcodes

* @param cellcodes[cmu_count][12] Will hold the voltage values for every cell in uV.
* @param cmu_count Number of cmus that are being communicated with
*/
void LTC6804_acquireVoltage(uint16_t cellcodes [][NO_READINGS_PER_CMU], uint8_t cmu_count)
{
    //This code seems to have had an error for multiple CMUs in writing the config.
    //The "write" array is being changed, but this is untested, revert change 
    //if necessary
    wake_LTC6804();
    wait_us(330);    
    
    LTC6804_init(MD_FAST, DCP_DISABLED, CELL_CH_ALL, AUX_CH_VREF2);
    uint8_t write [NO_CMUS][6];
    for(int i = 0; i < NO_CMUS; i++){
        for(int j = 0; j < 6; j++){
            write[i][j] = 0x00;
        }
    }
     wake_LTC6804();//ensures CMU's are in ready state and wakes it up from low power mode
     LTC6804_wrcfg(cmu_count,write);
     wait_us(330);
     wake_LTC6804(); //This might need to be removed
     LTC6804_acquireVoltageTx();
     wait_us(930);
     LTC6804_acquireAllVoltageRegRx(0, cmu_count, cellcodes);   
}

/**
Sets the balancing transistors by adjusting the configuration states of the 
LTC6804. This version of the function writes all 12 states for a chosen 
transistor. If other forms end up being more useful I will add other overloaded
versions.
 
@param uint8_t ic The specific IC to write to (can be changed in favor of a 
states array of size 12*cmu_count if preferred)

@param uint8_t cmu_count Number of CMUs total on the board 
 
@param uint8_t states[12] For Sn in S1-S12 set states[n-1] to 1 to enable 
balancing and 0 to disable balancing.
*/
void LTC6804_balance(const uint8_t ic, const uint8_t cmu_count, uint8_t states[NO_READINGS_PER_CMU])
{
    //Consider using this to define the configs: (uint8_t *)malloc(cmu_count*sizeof(uint8_t));
    uint8_t r_config[cmu_count][8];
    uint8_t w_config[cmu_count][6];//Size is smaller because there aren't PEC codes
    wake_LTC6804();
    wait_us(330);
    LTC6804_rdcfg(cmu_count, r_config); 
        
    /*for (int i=0; i<8; i++) {
      printf("TEST %d config \r\n", (uint8_t)r_config[0][i]); 
      } */
        
    uint8_t cfgr4 = 0; //This entire configuration is DCC states
    uint8_t cfgr5 = r_config[ic][5];
        
    for(int i = 0; i < 8; i++)
    {
	//Note: This disgusting thing is written by someone who has not used c++ in a long time
	cfgr4 = states[i] ? cfgr4 | (1u << i) : cfgr4 & ~(1u << i);
    }
        
    for(int i = 8; i < 12; i++)
    {
	cfgr5 = states[i] ? cfgr5 | (1u << (i-8)) : cfgr5 & ~(1u << (i-8));
    }
        
    //printf("cfgr4 %d \r\n", (uint8_t)cfgr4);
    //printf("cfgr5 %d \r\n", (uint8_t)cfgr5);
    for(int i =0 ; i < cmu_count; i++)
    {
	for(int j = 0; j < 6; j++)
	{
	    w_config[i][j] = r_config[i][j];   
	}   
    }
    w_config[ic][4] = cfgr4;
        w_config[ic][5] = cfgr5;
        
        wake_LTC6804();
        wait_us(330);
        LTC6804_wrcfg(cmu_count,w_config); //Make sure this is written in the write order
}
/**
Takes a set of voltages corresponding to cell voltage measurements and a voltage
limit (presumably 4200 mV) and then drains all cells as needed. If current state
of balancing is needed outside of this function, it can be modified.

Current implementation of this function just uses the hard limit, so it might end
up constantly enabling and disabling balancing as it hits the limit then goes under.
That behavior can be improved upon if needed. One idea is to use derivative of
voltage with time to predict when it will hit maxVoltage so there is no risk of 
going over, or allow them to drain such that all of the cells are at the same voltage.

@param uint16_t voltages[][12] Measured voltages to be used for deciding what to
balance. The dimensions must be [total_ic][12].

@param uint16_t maxVoltage voltage limit for the cells 
@param uint8_t cmu_count Number of CMUs that are connected on that isoSPI bus to be balanced
*/
void LTC6804_balanceVoltage(uint16_t voltages[][NO_READINGS_PER_CMU], uint16_t maxVoltage, uint8_t cmu_count)
{
    //Consider making states a parameter as a pointer so it can be referenced outside of the function easier.
    uint8_t states[cmu_count][12]; // May need to dynamically allocate this memory
    
    for(int i = 0; i < cmu_count; i++)
    {
	for(int j = 0; j < 12; j++)
	{
	    states[i][j] = voltages[i][j] >= maxVoltage ? 1 : 0; //Not sure if ternary operator is needed in C.    
	}
	LTC6804_balance(i, cmu_count, states[i]);    
    }
}

