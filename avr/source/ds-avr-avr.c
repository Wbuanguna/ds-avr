
/*
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


#define F_CPU 16000000UL
#define BAUD_RATE 9600
#define MYUBRR 0x0067

#include <util/delay.h>

#include <stdarg.h>
#include <stdio.h>

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/wdt.h> 

#include "spi.h"
#include "uart.h"
#include "ds-avr.h"

#include "pwm.c"

#define LED_PIN PORTD3

#define DEBUG 0


uint8_t have_data = 0;

uint8_t incoming_bufs[2][BUFFER_LENGTH];
uint8_t outgoing_bufs[2][BUFFER_LENGTH];

uint8_t *incoming = incoming_bufs[0];
uint8_t *cincoming = incoming_bufs[1]; // Completed incoming

uint8_t *outgoing = outgoing_bufs[1];
uint8_t *coutgoing = outgoing_bufs[0]; // Completed outgoing

uint8_t received=0;
uint8_t transmitted = 0;
short int cmd_length=0;
char sending = 0;
uint8_t pwm_pins_on = 0;

int __attribute__ ((format (printf,1, 2))) serial_printf (const char *fmt, ...) 
{
    va_list args;
    uint8_t i, l;
    char printbuffer[32];

    va_start (args, fmt);

    /* For this to work, printbuffer must be larger than
    * anything we ever want to print.
    */
    l = vsnprintf (printbuffer, 32, fmt, args);
    va_end (args);
    serialWrite(l);
    /* Print the string */
    for(i=0; i < l; i++) {
        serialWrite(printbuffer[i]);
    }
    return 0;
} 

// flash led that's connected to pin PD7
void flash_led(int count, int l)
{
    int i;
    DDRD |= _BV(LED_PIN);
    for (i=0; i<count*2; i++) {
        PORTD ^= _BV(LED_PIN);
        _delay_ms(l);
    }
}

void send_chars(int length, ...)
{
    va_list ap;
    int8_t j;
    va_start(ap, length); //Requires the last fixed parameter (to get the address)
    for(j=0; j<length; j++)
        outgoing[j] = va_arg(ap, int); //Requires the type to cast to. Increments ap to the next argument.
    va_end(ap);
    transmitted = 0;
    SPDR = 0xE1;
#if DEBUG
    serial_printf("Sending: %i:", length);
    for(j = 0; j < length; j++) {
        serial_printf(" 0x%x", outgoing[j]);
    }
    serialWrite('\n');
#endif
}

// http://www.avrtutor.com/tutorial/thermo/crc.php
uint8_t CRC8(uint8_t input, uint8_t seed)
{
    uint8_t i, feedback;

    for (i=0; i<8; i++)
    {
        feedback = ((seed ^ input) & 0x01);
        if (!feedback) seed >>= 1;
        else
        {
            seed ^= 0x18;
            seed >>= 1;
            seed |= 0x80;
        }
        input >>= 1;
    }

    return seed;   
}

int analog_read(uint8_t pin) {
    uint8_t low, high;
    // Set channel
    ADMUX = _BV(REFS0) | (pin & 0x0f);
    // Start conversion
    ADCSRA |= _BV(ADSC);
    // Wait for it to complete
    while (bit_is_set(ADCSRA, ADSC));
    // Get and return data
    low = ADCL;
    high = ADCH;
    return (high << 8) | low;
}

inline void swap_buffers(void) {
    uint8_t *tmp;
    tmp = cincoming;
    cincoming = incoming;
    incoming = tmp;
    
    tmp = outgoing;
    outgoing = coutgoing;
    coutgoing = tmp;
    
    have_data = received;
}

void pwm_on_command(void) {
    if(cincoming[2] == PWM0 || cincoming[2] == PWM1) {
        if(bit_is_clear(pwm_pins_on, PWM0) && bit_is_clear(pwm_pins_on, PWM1)) {
            pwmInit56();
        }
    }
    if(cincoming[2] == PWM0) {
        DDRD |= _BV(PD6);
        pwmOn6();
        pwm_pins_on |= _BV(PWM0);
        pwmSet6(0);
    } else if(cincoming[2] == PWM1) {
        DDRD |= _BV(PD5);
        pwmOn5();
        pwm_pins_on |= _BV(PWM1);
        pwmSet5(0);
    } else if(cincoming[2] == PWM5) {
        pwmInit311();
        DDRD |= _BV(PD3);
        pwmOn3();
        pwm_pins_on |= _BV(PWM5);
        pwmSet3(0);
    }
}

void pwm_set_command(void) {
    if(cincoming[2] == PWM0) {
        pwmSet6(cincoming[3]);
    } else if(cincoming[2] == PWM1) {
        pwmSet5(cincoming[3]);
    } else if(cincoming[2] == PWM5) {
        pwmSet3(cincoming[3]);
    }
}

void pwm_off_command(void) {
    if(cincoming[2] == PWM0) {
        //DDRD |= _BV(PD6);
        pwmOff6();
        pwm_pins_on &= ~_BV(PWM0);
        //pwmSet6(0);
    } else if(cincoming[2] == PWM1) {
        //DDRD |= _BV(PD5);
        pwmOff5();
        pwm_pins_on &= ~_BV(PWM1);
       // pwmSet5(0);
    } else if(cincoming[2] == PWM5) {
        pwmOff311();
        //DDRD |= _BV(PD3);
        pwmOff3();
        pwm_pins_on &= ~_BV(PWM5);
        //pwmSet3(0);
    }
    if(cincoming[2] == PWM0 || cincoming[2] == PWM1) {
        if(bit_is_clear(pwm_pins_on, PWM0) && bit_is_clear(pwm_pins_on, PWM1)) {
            pwmOff56();
        }
    }
}

// parse the data received from the other device
void parse_message(uint8_t length)
{
    uint8_t check;
    int i;

#if DEBUG
    serial_printf("Recieved: %i:", length);
    for(i = 0; i < length; i++) {
        serial_printf(" 0x%x", cincoming[i]);
    }
    serialWrite('\n');
#endif
    if(cincoming[0] != 0xDE || length < 3 /* incoming[received-1] != 0xAD*/) {
        serial_printf("Error: invalid command\n");
        send_chars(1, cincoming[0]);
        return;
    }
    check = 0xFF;
    for(i = 0; i < length - 1; i++) {
        check = CRC8(cincoming[i], check);
    }
#if DEBUG
   serial_printf("Check: 0x%x\n", check);
#endif
    if(check != cincoming[length - 1]) {
        serial_printf("Error: invalid CRC\n");
        return;
    }
    switch(cincoming[1]) {
        case NULL_COMMAND:
            //flash_led(2, 50);
            break;
        case FLASH_LED_COMMAND:
            flash_led(cincoming[2], 200);
            break;
        case ANALOG_READ_COMMAND:
            PORTD |= _BV(LED_PIN);
            //i = 1337;
            i = analog_read(cincoming[2]);
            send_chars(2, i & 0xff, i >> 8);
            PORTD &= ~_BV(LED_PIN);
            break;
        case READ_INCREMENT_COMMAND:
            transmitted += cincoming[2];
            if(transmitted >= BUFFER_LENGTH)
                transmitted = 0;
            SPDR = outgoing[transmitted];
            //flash_led(3, 50);
            break;
        case IO_REG_GET_COMMAND:
            if(cincoming[2] < 0x20 || cincoming[2] > 0xFF)
                break;
            send_chars(1, _MMIO_BYTE(cincoming[2]) );
            break;
        case IO_REG_SET_COMMAND:
            if(cincoming[2] < 0x20 || cincoming[2] > 0xFF)
                break;
            _MMIO_BYTE(cincoming[2]) = cincoming[3];
            break;
        case PWM_ON_COMMAND:
            pwm_on_command();
            break;
        case PWM_SET_COMMAND:
            pwm_set_command();
            break;
        case PWM_OFF_COMMAND:
            pwm_off_command();
            break;
        default:
            flash_led(2, 500);
    }
    cli();
    if(received == 0) { // Swap if another transmission hasn't been started yet
      swap_buffers();
    }
    sei();
}
//PCINT2
// called by the SPI system when there is data ready.
ISR(SPI_STC_vect)
{
    /*if((incoming[received] = SPDR) == 0xAD)
        return;*/
    incoming[received] = SPDR;
    SPDR = coutgoing[received];
    received++;
    
    PCMSK0 |= _BV(PCINT20);
    /*
    while(1) {
        //Poll Status Register SPIF bit until true
        while(SPSR, SPIF) && bit_is_clear(PINB, SPI_SS_PIN) );

        if(bit_is_set(SPSR, SPIF)) {
            incoming[received] = SPDR;
            SPDR = coutgoing[received]; 
            received++;      
            if(received >= (BUFFER_LENGTH-1)) {
                break;
            }
        } else {
            break;
        }
    }*/
    if(received >= (BUFFER_LENGTH-1)) {
        swap_buffers();
        PCMSK0 &= ~_BV(PCINT20);
        received = 0;
    }
    return;
}

ISR(PCINT0_vect) {
    if(bit_is_set(PINB, SPI_SS_PIN)) {
        swap_buffers();
        //PCICR &= ~PCIE0;
        received = 0;
        return;
    }
}


inline void idle(void) {
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_mode();
}

int main(void) {  
    DDRD |= _BV(LED_PIN);
    
    // Setup USART
    /* Clean start slate */
    UCSR0A = 0;
    UCSR0B = 0;
    UCSR0C = (3<<UCSZ00);
    /* Set baud rate */ 
	UBRR0H = (unsigned char)(MYUBRR>>8); 
	UBRR0L = (unsigned char) MYUBRR; 
	/* Enable receiver and transmitter   */
	UCSR0B = (1<<RXEN0)|(1<<TXEN0); 
	/* Frame format: 8data, No parity, 1stop bit */ 
	UCSR0C = (3<<UCSZ00);  

    // Setup ADC
    ADCSRA |= _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0); 
    
    // Reduce power, turn off TWI (I2C), Timer/Counter2, Timer/Counter0, 
    // Timer/Counter1, Analog Comparator
    PRR |= _BV(PRTWI); // | _BV(PRTIM2) | _BV(PRTIM0) | _BV(PRTIM1);
    ACSR |= _BV(ACD);

    // Setup SPI
    DDRB &= ~_BV(SPI_MOSI_PIN); // input
    DDRB |= _BV(SPI_MISO_PIN); // output
    DDRB &= ~_BV(SPI_SS_PIN); // input
    DDRB &= ~_BV(SPI_SCK_PIN);// input    
    /* Interrupt & spi enabled, MSB first, Slave, clock idle high, sample 
       trailing, F_CPU/4 */
    SPCR = (1 << SPIE) | (1 << SPE) | (0 << DORD) | (0 << MSTR) | (1 << CPOL) |
           (1 << CPHA) | (0 << SPR1) | (0 << SPR0);
    // Enable slave select pin change interrupt
    PCICR |= _BV(PCIE0);

    // flash LED at start to indicate were ready
    flash_led(1, 100);
    sei();
    serial_printf("Hello? Can anybody hear me?\n");
    
    while(1) {
        cli();
        if(have_data) {
            sei();
            parse_message(have_data);
            have_data = 0;
        }
        sei();
    }

    return 0;
}

