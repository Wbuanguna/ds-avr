
/*
Written by Windell Oskay, http://www.evilmadscientist.com/

Copyright 2009 Windell H. Oskay
Distributed under the terms of the GNU General Public License, please see below.

Additional license terms may be available; please contact us for more information.

More information about this project is at 
http://www.evilmadscientist.com/article.php/peggy

 This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


*/

#include <avr/io.h>


unsigned char serialCheckRxComplete(void)
{
	return( UCSR0A & _BV(RXC0)) ;		// nonzero if serial data is available to read.
}

unsigned char serialCheckTxReady(void)
{
	return( UCSR0A & _BV(UDRE0) ) ;		// nonzero if transmit register is ready to receive new data.
}

unsigned char serialRead(void)
{
	while (serialCheckRxComplete() == 0)		// While data is NOT available to read
	{;;} 
	return UDR0;
}

void serialWrite(unsigned char DataOut)
{
	while (serialCheckTxReady() == 0)		// while NOT ready to transmit 
	{;;} 
	UDR0 = DataOut;
}
