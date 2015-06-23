/* 
 * Copyright (c) 2009 Andrew Smallbone <andrew@rocketnumbernine.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <spi.h>

#ifdef __cplusplus
extern "C"{
#endif

#ifdef __ARDUINO__
#include <wiring.h>
#endif

void setup_spi(unsigned char mode, int dord, int interrupt, unsigned char clock)
{
#ifdef __ARDUINO__
  if (clock == SPI_SLAVE) { // if slave SS is input (unused if master)
    pinMode(SPI_MOSI_PIN, INPUT);
    pinMode(SPI_MISO_PIN, OUTPUT);
    pinMode(SPI_SCK_PIN, INPUT);
    pinMode(SPI_SS_PIN, INPUT);
  } else {
    pinMode(SPI_MOSI_PIN, OUTPUT);
    pinMode(SPI_MISO_PIN, INPUT);
    pinMode(SPI_SCK_PIN, OUTPUT);
    pinMode(SPI_SS_PIN, OUTPUT);
  }
#else
  // specify pin directions for SPI pins on port B
  if (clock == SPI_SLAVE) { // if slave SS and SCK is input
    DDRB &= ~(1<<SPI_MOSI_PIN); // input
    DDRB |= (1<<SPI_MISO_PIN); // output
    DDRB &= ~(1<<SPI_SS_PIN); // input
    DDRB &= ~(1<<SPI_SCK_PIN);// input
  } else {
    DDRB |= (1<<SPI_MOSI_PIN); // output
    DDRB &= ~(1<<SPI_MISO_PIN); // input
    DDRB |= (1<<SPI_SCK_PIN);// output
    DDRB |= (1<<SPI_SS_PIN);// output
  }
#endif
  SPCR = ((interrupt ? 1 : 0)<<SPIE) // interrupt enabled
    | (1<<SPE) // enable SPI
    | (dord<<DORD) // LSB or MSB
    | (((clock != SPI_SLAVE) ? 1 : 0) <<MSTR) // Slave or Master
    | ((mode & 0x02) <<CPOL) | ((mode & 0x01) <<CPHA) // clock timing mode
    | ((clock & 0x02) << SPR1) | ((clock & 0x01) << SPR0); // cpu clock divisor
  SPSR |= ((clock & 0x04) << SPI2X); // clock divisor
}

void disable_spi()
{
  SPCR = 0;
}

unsigned char send_spi(unsigned char out)
{
  SPDR = out;
  while (!(SPSR & (1<<SPIF)));
  return SPDR;
}

unsigned char received_from_spi(unsigned char data)
{
  SPDR = data;
  return SPDR;
}
