#include <nds.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include <ctype.h>
#include <nds/card.h>
#include <nds/system.h>
#include <nds/memory.h>
#include <nds/bios.h>

#include "ds-avr.h"
#include "ds-avr-ds.h"

int shortwait;
uint8_t speed;

//--------------------------------------------------------------
//    SPI section
//--------------------------------------------------------------
uint8_t incoming[BUFFER_LENGTH];

uint8_t spi_send(uint8_t in_byte) {
	uint8_t out_byte;
	//swiDelay(WAIT_CYCLES);

	AUXSPIDATA = in_byte; // send the output byte to the SPI bus
	CARD_WaitBusy(); // wait for transmission to complete
	out_byte=AUXSPIDATA; // read the input byte from the SPI bus
#ifdef ARM9
	iprintf("transmit: 0x%x recieved: 0x%x\n", in_byte, out_byte);	
#endif
	swiDelay(30 + shortwait);

	return out_byte;
}

uint8_t send_array(int length, uint8_t *data)
{
    int j;
    AUXSPION();
	swiDelay(WAIT_CYCLES);

    for(j=0; j<length; j++) {
        incoming[j] = spi_send(*data);
        data++;
    }
    swiDelay(12 + WAIT_CYCLES);
    
    AUXSPIOFF();
    return incoming[0];
}


uint8_t send_chars(int length, ...)
{
    uint8_t check, data_array[BUFFER_LENGTH];
    va_list ap;
    int j;
    
    if(length > BUFFER_LENGTH - 2) {
//        iprintf("Error: cannot send more then %d bytes\n", BUFFER_LENGTH - 2);
        return 0xFF;
    }
    
    check = 0xFF;
    va_start(ap, length);
    data_array[0] = 0xDE;
    check = CRC8(0xDE, check);
    for(j=0; j<length; j++) {
        data_array[j+1] = va_arg(ap, int);
        check = CRC8(data_array[j+1], check);
    }
    //iprintf("CRC: 0x%x\n", check);
    data_array[j+1] = check;
    va_end(ap);
    
    return send_array(length + 2, data_array);
}

//uint8_t recieve_chars(

uint8_t read_increment(uint8_t n) {
	uint8_t data;
	data = send_chars(2, READ_INCREMENT_COMMAND, n);
	return data;
}

