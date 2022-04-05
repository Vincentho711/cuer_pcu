#include "SPI_Parser.h"

//Define SPI and I2C I/O pins
SPI spi(p11, p12, p13); // mosi, miso, sclk
DigitalOut chipSelect(p14);

uint8_t ADCV[2];
uint8_t ADAX[2];

//I2C i2c(p9, p10); This line gives conflicting definition errors


void wake_LTC6804()
{
    spi.format(8,3);//All data transfer on LTC6804 occur in byte groups. LTC6820 set up such that POL=1 and PHA=3, this corresponds to mode 3 in mbed library. spi.frequency(spiBitrate);
    spi.frequency(spiBitrate);
    chipSelect=0;
    wait_us(70);
    spi.write(0x00); //Ensures isoSPI is in ready mode
    wait_us(30);
    chipSelect=1;
}

void wake_SPI()
{
    chipSelect=0;
    wait_us(25);
    chipSelect=1;
}

void LTC6804_init(uint8_t mode, uint8_t balancingEn, uint8_t cellCh, uint8_t gpioCh)
{
    uint8_t bitSetter;

    bitSetter = (mode & 0x02) >> 1;
    ADCV[0] = bitSetter + 0x02;
    bitSetter = (mode & 0x01) << 7;
    ADCV[1] =  bitSetter + 0x60 + (balancingEn<<4) + cellCh;

    bitSetter = (mode & 0x02) >> 1;
    ADAX[0] = bitSetter + 0x04;
    bitSetter = (mode & 0x01) << 7;
    ADAX[1] = bitSetter + 0x60 + gpioCh;
}

void LTC6804_acquireVoltageTx()
{
    uint8_t broadcastCmd[4];
    uint16_t pec;

    broadcastCmd[0]=ADCV[0];
    broadcastCmd[1]=ADCV[1];

    pec=pec15_calc(2, ADCV);
    broadcastCmd[2] = (uint8_t)(pec>>8);
    broadcastCmd[3] = (uint8_t)(pec);

    wake_SPI();
    //chipSelect=1;
    chipSelect=0; //select LTC6820 by driving pin low
    spi_write_array(4, broadcastCmd);
    chipSelect=1; //de-select LTC6820 by driving pin high
}

uint8_t LTC6804_acquireAllVoltageRegRx(uint8_t reg, uint8_t cmuCount, uint16_t cell_codes[][12])
{
    //rdcv
    const uint8_t NUM_RX_BYT = 8;
    const uint8_t BYT_IN_REG = 6;
    const uint8_t CELL_IN_REG = 3;

    uint8_t *cell_data;
    int8_t pec_error = 0;
    uint16_t parsed_cell;
    uint16_t received_pec;
    uint16_t data_pec;
    uint8_t data_counter = 0;
    cell_data = (uint8_t *)malloc((NUM_RX_BYT*cmuCount) * sizeof(uint8_t));

    if (reg == 0) {
        for (uint8_t cell_reg = 1; cell_reg < 5; cell_reg++) {
            data_counter = 0;
            LTC6804_acquireSingleVoltageRegRx(cell_reg, cmuCount, cell_data);
            for (uint8_t current_cmu = 0; current_cmu < cmuCount; current_cmu++) {
                for (uint8_t current_cell = 0; current_cell < CELL_IN_REG; current_cell++) {
                    parsed_cell = cell_data[data_counter] + (cell_data[data_counter + 1] << 8);
                    cell_codes[current_cmu][current_cell + ((cell_reg - 1)*CELL_IN_REG)] = parsed_cell;
                    data_counter = data_counter + 2;
                }
                received_pec = (cell_data[data_counter] << 8) + cell_data[data_counter + 1];
                data_pec = pec15_calc(BYT_IN_REG, &cell_data[current_cmu*NUM_RX_BYT]);
                if (received_pec != data_pec) {
                    // if (LTC6804_DEBUG) printf(" PEC ERROR  \r\n");
                    pec_error--;//pec_error=-1;
                }
                data_counter = data_counter + 2;
            }
        }
    }

    else {
        LTC6804_acquireSingleVoltageRegRx(reg, cmuCount, cell_data);
        for (uint8_t current_cmu = 0; current_cmu < cmuCount; current_cmu++) {
            for (uint8_t current_cell = 0; current_cell < CELL_IN_REG; current_cell++) {
                parsed_cell = cell_data[data_counter] + (cell_data[data_counter + 1] << 8);
                cell_codes[current_cmu][current_cell + ((reg - 1)*CELL_IN_REG)] = 0X0000FFFF & parsed_cell;
                data_counter = data_counter + 2;
            }
            received_pec = (cell_data[data_counter] << 8) + cell_data[data_counter + 1];
            data_pec = pec15_calc(BYT_IN_REG, &cell_data[current_cmu*NUM_RX_BYT*(reg - 1)]);
            if (received_pec != data_pec) {
                /* cout << " PEC ERROR " << endl; */
                pec_error--;//pec+error=-1;
            }
        }
    }
    free(cell_data);
    return(pec_error);
}

void LTC6804_acquireSingleVoltageRegRx(uint8_t reg, uint8_t cmuCount, uint8_t *data)
{
    //rdcv_reg
    uint8_t cmd[4];
    uint16_t temporary_pec;

    if (reg == 1) {
        cmd[1] = 0x04;
        cmd[0] = 0x00;
    } else if (reg == 2) {
        cmd[1] = 0x06;
        cmd[0] = 0x00;
    } else if (reg == 3) {
        cmd[1] = 0x08;
        cmd[0] = 0x00;
    } else if (reg == 4) {
        cmd[1] = 0x0A;
        cmd[0] = 0x00;
    }

    wake_SPI();

    for (int current_cmu = 0; current_cmu < cmuCount; current_cmu++) {
        cmd[0] = 0x80 + (current_cmu << 3);//setting address
        temporary_pec = pec15_calc(2, cmd);
        cmd[2] = (uint8_t)(temporary_pec >> 8);
        cmd[3] = (uint8_t)(temporary_pec);
        chipSelect = 0;
        spi_write_read(cmd,4,&data[current_cmu*8],8);
        chipSelect = 1;
    }
}

void LTC6804_setConfigReg(uint8_t cmuCount, uint8_t config[][6])
{
    const uint8_t BYTES_IN_REG = 6;
    const uint8_t CMD_LEN = 4 + (8 * cmuCount);
    uint8_t *cmd;
    uint16_t temporary_pec;
    uint8_t cmd_index;

    cmd = (uint8_t *)malloc(CMD_LEN * sizeof(uint8_t));

    cmd[0] = 0x00;
    cmd[1] = 0x01;
    cmd[2] = 0x3d;
    cmd[3] = 0x6e;

    cmd_index = 4;
    for (int8_t current_cmu = cmuCount; current_cmu > 0; current_cmu--) {
        for (uint8_t current_byte = 0; current_byte < BYTES_IN_REG; current_byte++) {
            cmd[cmd_index] = config[current_cmu - 1][current_byte];
            cmd_index = cmd_index + 1;
        }
        temporary_pec = (uint16_t)pec15_calc(BYTES_IN_REG, &config[current_cmu - 1][0]);
        cmd[cmd_index] = (uint8_t)(temporary_pec >> 8);
        cmd[cmd_index + 1] = (uint8_t)temporary_pec;
        cmd_index = cmd_index + 2;
    }

    //wake_SPI();
    for (int current_cmu = 0; current_cmu < cmuCount; current_cmu++) {
        cmd[0] = 0x80 + (current_cmu << 3);
        temporary_pec = pec15_calc(2, cmd);
        cmd[2] = (uint8_t)(temporary_pec >> 8);
        cmd[3] = (uint8_t)(temporary_pec);
        chipSelect = 0;
        wait_us(350);
        spi_write_array(4, cmd);
        spi_write_array(8,&cmd[4+(8*current_cmu)]);
        chipSelect=1;
    }
    free(cmd);
}


uint16_t pec15_calc(uint8_t len, uint8_t *data)
{
    uint16_t remainder,addr;

    remainder = 16;//initialize the PEC
    for(uint8_t i = 0; i<len; i++) { // loops for each byte in data array
        addr = ((remainder>>7)^data[i])&0xff;//calculate PEC table address
        remainder = (remainder<<8)^crc15Table[addr];
    }
    return(remainder*2);//The CRC15 has a 0 in the LSB so the remainder must be multiplied by 2
}


void spi_write_array(uint8_t len, // Option: Number of bytes to be written on the SPI port
                     uint8_t data[] //Array of bytes to be written on the SPI port
                    )
{
    wait_us(70);
    spi.format(8,3); //All data transfer on LTC6804 occur in byte groups. LTC6820 set up such that POL=1 and PHA=3, this corresponds to mode 3 in mbed library. spi.frequency(spiBitrate);
    spi.frequency(spiBitrate);
    for (uint8_t i = 0; i < len; i++) {
        spi.write((char)data[i]);
        wait_us(110);
    }
}


void spi_write_read(uint8_t tx_Data[],//array of data to be written on SPI port
                    uint8_t tx_len, //length of the tx data arry
                    uint8_t *rx_data,//Input: array that will store the data read by the SPI port
                    uint8_t rx_len //Option: number of bytes to be read from the SPI port
                   )
{
    //cout<<" SPI Write Read Called" << endl;
    spi.format(8,3);//All data transfer on LTC6804 occur in byte groups. LTC6820 set up such that POL=1 and PHA=3, this corresponds to mode 3 in mbed library. spi.frequency(spiBitrate);
    spi.frequency(spiBitrate);
    //printf("\r\n");
    //uint8_t ret;

    chipSelect=0;
    wait_us(70);
    for (uint8_t i = 0; i < tx_len; i++) {
        //printf("index is %d: %d \r\n",i,tx_Data[i]);
        //cout << (int)tx_Data[i] << " , " << endl;
        spi.write(tx_Data[i]);
        wait_us(110);
        //printf("writing has returned %d \r\n", ret);
    }
    for (uint8_t i = 0; i < rx_len; i++) {
        rx_data[i] = (uint8_t)spi.write(0x00);
        wait_us(110);
        //ret = rx_data[i];
        //printf("reading has returned %d \r\n", ret);
    }
    chipSelect=1;

    for (uint8_t i = 0; i < tx_len; i++) {
        //printf("index is %d: %d \r\n",i,tx_Data[i]);
        break;
    }

}



int8_t LTC6804_rdcfg(uint8_t total_ic, uint8_t r_config[][8])
{
    //cout<<"Read Config function called" << endl;
    const uint8_t BYTES_IN_REG = 8;

    uint8_t cmd[4];
    uint8_t *rx_data;
    int8_t pec_error = 0;
    uint16_t data_pec;
    uint16_t received_pec;
    rx_data = (uint8_t *) malloc((8*total_ic)*sizeof(uint8_t));
    //1
    cmd[0] = 0x00;
    cmd[1] = 0x02;
    cmd[2] = 0x2b;
    cmd[3] = 0x0A;

    //2
    //wake_SPI(); //This will guarantee that the LTC6804 isoSPI port is awake. This command can be removed.
    //3
    for(int current_ic = 0; current_ic<total_ic; current_ic++) {
        cmd[0] = 0x80 + (current_ic<<3); //Setting address
        data_pec = pec15_calc(2, cmd);
        cmd[2] = (uint8_t)(data_pec >> 8);
        cmd[3] = (uint8_t)(data_pec);
        //ut << "RdConf cmd:" << (int)cmd[0] << endl;
        spi_write_read(cmd,4,&rx_data[current_ic*8],8);
    }
    //for (int i=0; i<8; i++) {
    //cout << (int)rx_data[i] << " , ";
    //}
    //cout << endl;

    for (uint8_t current_ic = 0; current_ic < total_ic; current_ic++) { //executes for each LTC6804 in the stack
        //4.a
        for (uint8_t current_byte = 0; current_byte < BYTES_IN_REG; current_byte++) {
            r_config[current_ic][current_byte] = rx_data[current_byte + (current_ic*BYTES_IN_REG)];
        }
        //4.b
        received_pec = (r_config[current_ic][6]<<8) + r_config[current_ic][7];
        //cout << "Rxpec " << received_pec << endl;
        data_pec = pec15_calc(6, &r_config[current_ic][0]);
        //cout << "Datpec" << data_pec << endl;
        if(received_pec != data_pec) {
            pec_error = -1;
            /* cout << "PEC error!!!" << endl; */
        }
    }
    free(rx_data);
    //5
    //cout << r_config << endl;
    return(pec_error);
}
/*
    1. Load cmd array with the write configuration command and PEC
    2. wakeup isoSPI port, this step can be removed if isoSPI status is previously guaranteed
    3. read configuration of each LTC6804 on the stack
    4. For each LTC6804 in the stack
      a. load configuration data into r_config array
      b. calculate PEC of received data and compare against calculated PEC
    5. Return PEC Error

*/


int8_t LTC6804_rdstat(uint8_t total_ic, uint8_t r_config[][8])
{
    /* cout<<"Read Config function called" << endl; */
    const uint8_t BYTES_IN_REG = 8;

    uint8_t cmd[4];
    uint8_t *rx_data;
    int8_t pec_error = 0;
    uint16_t data_pec;
    uint16_t received_pec;
    rx_data = (uint8_t *) malloc((8*total_ic)*sizeof(uint8_t));
    //1
    cmd[0] = 0x00;
    cmd[1] = 0x12;

    //2
    //wake_SPI(); //This will guarantee that the LTC6804 isoSPI port is awake. This command can be removed.
    //3
    for(int current_ic = 0; current_ic<total_ic; current_ic++) {
        cmd[0] = 0x80 + (15<<3); //Setting address
        data_pec = pec15_calc(2, cmd);
        cmd[2] = (uint8_t)(data_pec >> 8);
        cmd[3] = (uint8_t)(data_pec);
        //ut << "RdConf cmd:" << (int)cmd[0] << endl;
        spi_write_read(cmd,4,&rx_data[current_ic*8],8);
    }
    //for (int i=0; i<8; i++) {
    //cout << (int)rx_data[i] << " , ";
    //}
    //cout << endl;

    for (uint8_t current_ic = 0; current_ic < total_ic; current_ic++) { //executes for each LTC6804 in the stack
        //4.a
        for (uint8_t current_byte = 0; current_byte < BYTES_IN_REG; current_byte++) {
            r_config[current_ic][current_byte] = rx_data[current_byte + (current_ic*BYTES_IN_REG)];
        }
        //4.b
        received_pec = (r_config[current_ic][6]<<8) + r_config[current_ic][7];
        //cout << "Rxpec " << received_pec << endl;
        data_pec = pec15_calc(6, &r_config[current_ic][0]);
        //cout << "Datpec" << data_pec << endl;
        if(received_pec != data_pec) {
            pec_error = -1;
            //cout << "PEC error!!!" << endl;
        }
    }
    free(rx_data);
    //5
    //cout << r_config << endl;
    return(pec_error);
}


void LTC6804_wrcfg(uint8_t total_ic,uint8_t config[][6])
{
    const uint8_t BYTES_IN_REG = 6;
    const uint8_t CMD_LEN = 4+(8*total_ic);
    uint8_t *cmd;
    uint16_t temp_pec;
    uint8_t cmd_index; //command counter

    cmd = (uint8_t *)malloc(CMD_LEN*sizeof(uint8_t));
    //1
    cmd[0] = 0x00;
    cmd[1] = 0x01;
    cmd[2] = 0x3d;
    cmd[3] = 0x6e;

    //2
    cmd_index = 4;
    for (uint8_t current_ic = total_ic; current_ic > 0; current_ic--) {     // executes for each LTC6804 in stack,
        for (uint8_t current_byte = 0; current_byte < BYTES_IN_REG; current_byte++) { // executes for each byte in the CFGR register
            // i is the byte counter

            cmd[cmd_index] = config[current_ic-1][current_byte];    //adding the config data to the array to be sent
            cmd_index = cmd_index + 1;
        }
        //3
        temp_pec = (uint16_t)pec15_calc(BYTES_IN_REG, &config[current_ic-1][0]);// calculating the PEC for each board
        cmd[cmd_index] = (uint8_t)(temp_pec >> 8);
        cmd[cmd_index + 1] = (uint8_t)temp_pec;
        cmd_index = cmd_index + 2;
    }

    //4                               //This will guarantee that the LTC6804 isoSPI port is awake.This command can be removed.
    //5
    for (int current_ic = 0; current_ic<total_ic; current_ic++) {
        cmd[0] = 0x80 + (current_ic<<3); //Setting address
        temp_pec = pec15_calc(2, cmd);
        cmd[2] = (uint8_t)(temp_pec >> 8);
        cmd[3] = (uint8_t)(temp_pec);
        chipSelect=0;
        wait_us(350);
        spi_write_array(4,cmd);
        spi_write_array(8,&cmd[4+(8*current_ic)]);
        chipSelect=1;
    }
    free(cmd);
}

