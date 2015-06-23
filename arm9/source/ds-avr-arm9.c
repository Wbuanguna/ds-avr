
#include <nds.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#include <ctype.h>
#include <nds/card.h>
#include <nds/system.h>
#include <nds/memory.h>
#include <nds/bios.h>

#include <PA9.h>       // Include for PA_Lib

//#include <avr/io.h>

#include "ds-avr.h"
#include "ds-avr-ds.h"

extern FifoValue32HandlerFunc		fifo_value32_func[16];
int longwait;
int shortwait;

/*
uint8 spi_transfer(uint8 out) {
	AUXSPICNT = SLOT_ENABLE | SLOT_MODE_SPI;
	AUXSPIDATA = out;
	while (AUXSPICNT & SPI_BUSY); // Wait for transfer
	AUXSPICNT = SLOT_ENABLE;
	return AUXSPIDATA;
} 
*/

void pause() {
	iprintf("Press start...\n");
	while(1) {
		scanKeys();
		if(keysDown() & KEY_START)
			break;
		swiWaitForVBlank();
	}
	scanKeys();
}

void delay_vblanks(int n) {
	int i;
	for(i=0; i < n; i++)
		swiWaitForVBlank();
}

void flash_led(int n) {
    AvrFifoMessage msg;
	iprintf("flash_led(%i)\n", n);
	
	msg.type = LED_FLASH_MESSAGE;
	msg.LEDFlash.count = n;

	fifoSendDatamsg(FIFO_AVR, sizeof(msg), (u8*)&msg);
}

/*
int analog_read(uint8_t pin) {
    AvrFifoMessage msg;
    uint8_t buff[BUFFER_LENGTH];
//	iprintf("analog_read(%i)\n", pin);
	
	msg.type = ANALOG_READ_MESSAGE;
	msg.AnalogRead.pin = pin;

	fifoSendDatamsg(FIFO_AVR, sizeof(msg), (u8*)&msg);
	
	//while(!fifoCheckValue32(FIFO_AVR));
	while(!fifoCheckDatamsg(FIFO_AVR));

	fifoGetDatamsg(FIFO_AVR, BUFFER_LENGTH, buff);
	iprintf("0x%x 0x%x 0x%x 0x%x 0x%x \n", buff[0], buff[1], buff[2], buff[3], buff[4]);
	return buff[0] | (buff[1] << 8);
	//return (int)fifoGetValue32(FIFO_AVR); 
}
*/

int analog_read(uint8_t pin) {
	send_chars(2, ANALOG_READ_COMMAND, pin);
	swiDelay(20000);
	send_chars(3, NULL_COMMAND, 0, 0);
	if(spi_debug)
		iprintf("0x%x 0x%x 0x%x\n", incoming[0], incoming[1], incoming[2]);
	return (incoming[1] | (incoming[2] << 8)) & 0x3FF;
}

int ioport_get(uint8_t address) {
    AvrFifoMessage msg;

	msg.type = IO_PORT_GET_MESSAGE;
	msg.IOPortGetSet.address = address;

	fifoSendDatamsg(FIFO_AVR, sizeof(msg), (u8*)&msg);
	
	while(!fifoCheckValue32(FIFO_AVR));

	return (int)fifoGetValue32(FIFO_AVR); 
}

void ioport_set(uint8_t address, uint8_t val) {
    AvrFifoMessage msg;
	
	msg.type = IO_PORT_SET_MESSAGE;
	msg.IOPortGetSet.address = address;
	msg.IOPortGetSet.value = val;

	fifoSendDatamsg(FIFO_AVR, sizeof(msg), (u8*)&msg);
}

void avrValue32Handler(u32 value32, void * userdata) {
    iprintf("analog data: %d\n", value32);
}



void profile_commands(int reps) {
	AvrFifoMessage msg;
    time_t start, end;
    long int i;
    iprintf("profile_commands: %i reps\n", reps);
    /*
    start = time(NULL);
    for(i=0;i < (reps*10);i++) {
        swiDelay(10000);
    }
    end = time(NULL);
    iprintf("swiDelay(10000) x 10: %is\n", (int)(end - start));
    
    start = time(NULL);
    for(i=0;i <(reps*10);i++) {
        swiDelay(5000);
        swiDelay(5000);
    }
    end = time(NULL);
    iprintf("swiDelay(5000) x 20: %is\n", (int)(end - start));
    */
    start = time(NULL);
    for(i=0;i <reps;i++) {
        analog_read(0);
    }
    end = time(NULL);
    iprintf("analog_read: %is\n", (int)(end - start));
    
    start = time(NULL);
    for(i=0;i <reps;i++) {
        ioport_get(0x2B);
    }
    end = time(NULL);
    iprintf("ioport_get: %is\n", (int)(end - start));
    
    msg.type = PROFILE_MESSAGE;
	/*
	start = time(NULL);
	fifoSendDatamsg(FIFO_AVR, sizeof(msg), (u8*)&msg);
    while(!fifoCheckValue32(FIFO_AVR));
	fifoGetValue32(FIFO_AVR); 
	end = time(NULL);
    iprintf("ARM7 swiDelay(1e4) x 5e4: %is\n", (int)(end - start));
    */
}

void spi_control(uint16_t m1, uint16_t m2, uint8_t debug) {
	AvrFifoMessage msg;
	
	longwait = 1250 * m1;
	
	msg.type = CONFIG_MESSAGE;
	msg.SPIConfig.m1 = m1;
	msg.SPIConfig.m2 = m2;
	msg.SPIConfig.debug = debug;
	shortwait = 1 * msg.SPIConfig.m1;
	speed = m2;
	
	fifoSendDatamsg(FIFO_AVR, sizeof(msg), (u8*)&msg);
}

void init_graphics(void) {
	PrintConsole* defaultConsole;
	
	defaultConsole = consoleGetDefault();
	videoSetModeSub(MODE_0_2D);
	vramSetBankC(VRAM_C_SUB_BG); 

	consoleInit(NULL, defaultConsole->bgLayer, BgType_Text4bpp, 
				BgSize_T_256x256, defaultConsole->mapBase, 
				defaultConsole->gfxBase, false, true);
		
#define OVERVIEW_SCREEN 0		
	/* Graphics */
	PA_SetBgPalCol(OVERVIEW_SCREEN, 0, PA_RGB(31, 31, 31)); // Set the main screen color to white
	PA_Init16bitBg(OVERVIEW_SCREEN, 3);
	//PA_Draw16bitRect(OVERVIEW_SCREEN, 2, 2, 253, 189, PA_RGB(31, 0, 0));
}

void pwm_on(uint8_t output) {
	iprintf("PWM output %d on\n", output);
	send_chars(2, PWM_ON_COMMAND, output);
	swiDelay(20000);
}

void pwm_set(uint8_t output, uint8_t val) {
	iprintf("PWM output %d: %d\n", output, val);
	send_chars(3, PWM_SET_COMMAND, output, val);
	swiDelay(20000);
}

int main(void) {
	uint16_t m1, m2;
	int i;
	int val = 0;

	PA_Init();    // Initializes PA_Lib
	//PA_SwitchScreens();
	init_graphics();
	//powerOff(PM_BACKLIGHT_TOP);
	
	sysSetBusOwners(true, true);
	
	//iprintf("a = fast tranfers b = slow transfers\n");
	m1 = 0;
	m2 = 3;
	spi_debug = 0;
	spi_control(m1, m2, spi_debug);
	pwm_on(PWM0);

	iprintf("u/d & r/l adjust m1 & m2\n");
	iprintf("a = led b = analog\n");
	//iprintf("TIMER0_CR: 0x%x\n", TIMER0_CR);
	//iprintf("TIMER1_CR: 0x%x\n", TIMER1_CR);
	//iprintf("TIMER2_CR: 0x%x\n", TIMER2_CR);
	//iprintf("TIMER3_CR: 0x%x\n", TIMER3_CR);
	//fifoSetValue32Handler(FIFO_AVR, avrValue32Handler, 0);
	//iprintf("new func: 0x%x\n", (u32)fifo_value32_func[FIFO_AVR]);
	//soundEnable();
	
	//pause();
	i = 1;
	while(1) {
		scanKeys();
		if(Pad.Newpress.A) {
			//flash_led(i++);
			pwm_on(PWM0);
			pwm_on(PWM1);
			pwm_on(PWM5);
			send_chars(2, FLASH_LED_COMMAND, i++);
			if(i > 9)
				i = 1;
		
		}  else if(Pad.Newpress.B) {
			iprintf("analog data: %d\n", analog_read(0));
		} else if(Pad.Newpress.X) {
			sysSetBusOwners(true, false);
			iprintf("SPCR: 0x%x\n", ioport_get(0x4C) );
			sysSetBusOwners(true, true);
		} else if(Pad.Newpress.Y) {
			uint8_t val;
			sysSetBusOwners(true, false);
			val = ioport_get(0x2B);
			iprintf("PORTD: 0x%x\n", val);
			swiDelay(COMMAND_WAIT); // 0.5 ms
			val ^= (1 << 3);
			ioport_set(0x2B, val);
			sysSetBusOwners(true, true);
		} else if(Pad.Newpress.R) {
			if(spi_debug)
				spi_debug = false;
			else
				spi_debug = true;
			spi_control(m1, m2, spi_debug);
		} else if(Pad.Newpress.Start && Pad.Held.Select) {
		    //profile_commands(10000);
		} else if(Pad.Newpress.Up) {
			if (Pad.Held.L) m1 += 1000;
			else if(Pad.Held.A) m1 += 10;
			else m1 += 1;
			spi_control(m1, m2, spi_debug);
			iprintf("m1: %d\n", m1);
		} else if(Pad.Newpress.Down) {
			if (Pad.Held.L) m1 -= 1000;
			else if(Pad.Held.A) m1 -= 10;
			else m1 -= 1;
			spi_control(m1, m2, spi_debug);
			iprintf("m1: %d\n", m1);
		} else if(Pad.Newpress.Right && m2 < 3) {
			m2 += 1;
			spi_control(m1, m2, spi_debug);
			iprintf("m2: %d\n", m2);
		} else if(Pad.Newpress.Left && m2 > 0) {
			m2 -= 1;
			spi_control(m1, m2, spi_debug);
			iprintf("m2: %d\n", m2);
		}
		if(Stylus.Held) {
			pwm_set(PWM0, Stylus.X);
			pwm_set(PWM1, Stylus.X);
			pwm_set(PWM5, Stylus.X);
		}
		//PA_SetBgPalCol(OVERVIEW_SCREEN, 0, PA_RGB(31, 31, 31)); // Set the bottom screen color to white
		
		PA_WaitForVBL();
		//val = analog_read(0) >> 2;
		//PA_Draw16bitRect(OVERVIEW_SCREEN, val, 0, 255, 10, PA_RGB(31, 31, 31));
		//PA_Draw16bitRect(OVERVIEW_SCREEN, 0, 0, val, 10, PA_RGB(31, 0, 0));
		//val = analog_read(0) >> 4;
		//val = (val + 1) & 0x3F;
		//PA_Draw16bitRect(OVERVIEW_SCREEN, 0, 63-val, 10, 0, PA_RGB(31, 31, 31));
		//PA_Draw16bitRect(OVERVIEW_SCREEN, 0, 63, 10, 63-val, PA_RGB(31, 0, 0));
	}

	return 0;
}
