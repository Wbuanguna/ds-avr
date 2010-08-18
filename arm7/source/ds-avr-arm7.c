/*---------------------------------------------------------------------------------

default ARM7 core

Copyright (C) 2005
Michael Noland (joat)
Jason Rogers (dovoto)
Dave Murphy (WinterMute)

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1.	The origin of this software must not be misrepresented; you
must not claim that you wrote the original software. If you use
this software in a product, an acknowledgment in the product
documentation would be appreciated but is not required.
2.	Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.
3.	This notice may not be removed or altered from any source
distribution.

---------------------------------------------------------------------------------*/
#include <nds.h>
#include <dswifi7.h>
#include <maxmod7.h>
#include <nds/fifocommon.h>
#include <nds/fifomessages.h>
#include <time.h>

#define __AVR_ATmega328P__
#define __SFR_OFFSET 0x20
#define _SFR_ASM_COMPAT 1
#include <avr/io.h>

#include "ds-avr.h"
#include "ds-avr-ds.h"

//---------------------------------------------------------------------------------
void VcountHandler() {
//---------------------------------------------------------------------------------
	inputGetAndSend();
}

//---------------------------------------------------------------------------------
void VblankHandler(void) {
//---------------------------------------------------------------------------------
	Wifi_Update();
}

uint8_t ioport_get(uint8_t address) {
    uint8_t value;
	send_chars(2, IO_REG_GET_COMMAND, address);
	swiDelay(10000); // 2.1 ms
	value = read_increment(1);
	return value;
}

void ioport_set(uint8_t address, uint8_t val) {
	send_chars(3, IO_REG_SET_COMMAND, address, val);
}


uint8_t reg_orequal(uint8_t address, uint8_t val) {
	uint8_t d;
	d = ioport_get(address);
	d |= val;
	swiDelay(COMMAND_WAIT);
	ioport_set(address, d);
	return d;
}


//---------------------------------------------------------------------------------
void avrDataHandler(int bytes, void *user_data) {
//---------------------------------------------------------------------------------
	AvrFifoMessage msg;
	int value;

	fifoGetDatamsg(FIFO_AVR, bytes, (u8*)&msg);
	
	if(msg.type == PROFILE_MESSAGE) {
    	long int i;
    	
		for(i=0;i < (50000);i++) {
		    swiDelay(10000);
		}
    	
    	fifoSendValue32(FIFO_AVR, (u32)1337 );
	} else if(msg.type == CONFIG_MESSAGE) {
	   	shortwait = WAIT_1MS * msg.SPIConfig.m1;
	   	speed = msg.SPIConfig.m2;
	} else if(msg.type == LED_FLASH_MESSAGE) {
	    send_chars(2, FLASH_LED_COMMAND, msg.LEDFlash.count);
	} else if (msg.type == ANALOG_READ_MESSAGE) {
		send_chars(2, ANALOG_READ_COMMAND, msg.AnalogRead.pin);
		swiDelay(10000);
		send_chars(3, NULL_COMMAND, 0, 0);

		fifoSendDatamsg(FIFO_AVR, sizeof(incoming), incoming);
		//value = incoming[1];
		//value |= (incoming[2] << 8);
		/*value = read_increment(1);
		swiDelay(COMMAND_WAIT);
		value |= (read_increment(1) << 8);*/
		
		//fifoSendValue32(FIFO_AVR, (u32)value);
	} else if (msg.type == IO_PORT_GET_MESSAGE) {
		value = ioport_get(msg.IOPortGetSet.address);
		fifoSendValue32(FIFO_AVR, (u32)value);
	} else if (msg.type == IO_PORT_SET_MESSAGE) {
		ioport_set(msg.IOPortGetSet.address, msg.IOPortGetSet.value);
	} else if (msg.type == PWM_MESSAGE) {
		if(msg.PWM.command == PWM_ON) {
			if(msg.PWM.output == PWM5) {
				reg_orequal(TCCR2B, _BV(CS22));
				//TCCR2B |= CS22
			}
		}
	}
}

//---------------------------------------------------------------------------------
void avrCommandHandler(u32 command, void* userdata) {
//---------------------------------------------------------------------------------
/*
	int cmd = (command ) & 0x00F00000;
	int data = command & 0xFFFF;
	int channel = (command >> 16) & 0xF;
*/
}

//---------------------------------------------------------------------------------
void installAvrFIFO(void) {
//---------------------------------------------------------------------------------

	fifoSetDatamsgHandler(FIFO_AVR, avrDataHandler, 0);
//	fifoSetValue32Handler(FIFO_AVR, avrCommandHandler, 0);
}

//---------------------------------------------------------------------------------
int main() {
//---------------------------------------------------------------------------------
	irqInit();
	fifoInit();

	// read User Settings from firmware
	readUserSettings();

	// Start the RTC tracking IRQ
	initClockIRQ();

	SetYtrigger(80);

	installWifiFIFO();
	installSoundFIFO();

	installAvrFIFO();

	//mmInstall(FIFO_MAXMOD);

	installSystemFIFO();
	
	irqSet(IRQ_VCOUNT, VcountHandler);
	irqSet(IRQ_VBLANK, VblankHandler);

	irqEnable( IRQ_VBLANK | IRQ_VCOUNT | IRQ_NETWORK);   

	reg_orequal(PORTD, _BV(PORTD3));

	// Keep the ARM7 mostly idle
	while (1) swiWaitForVBlank();
}


