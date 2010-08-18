
#define AUXSPIDATA     CARD_EEPDATA

#define AUXSPIHOLD (1<<6) // 0x40
#define AUXSPIBUSY (1<<7) // 0x80
#define AUXSPIMODE (1<<13) // 0x2000
#define AUXSPIENABLE (1<<15) // 0x8000

/*
#define SPI_CORE 7

// HAVE_SPI_CONTROL = whether or not the core being compiled has direct control 
// of the spi bus or needs pass data onto the other core
#if SPI_CORE == 7 && defined(ARM7)
	#define HAVE_SPI_CONTROL 1
#elif SPI_CORE == 9 && defined(ARM9)
	#define HAVE_SPI_CONTROL 1
#else
	#define HAVE_SPI_CONTROL 0
#endif
*/

#define CARD_WaitBusy()   while (CARD_CR1 & AUXSPIBUSY);

// enables SPI bus at 4.1MHz
//#define SPI_On() CARD_CR1 = /*E*/0x8000 | /*SEL*/0x2000 | /*MODE*/0x40; // | (1<<1)| (1<<0);
// enables SPI bus at 512KHz
//#define SPI_On() CARD_CR1 = /*E*/0x8000 | /*SEL*/0x2000 | /*MODE*/0x40  | (1<<1)| (1<<0);
//#define AUXSPION() CARD_CR1 = AUXSPIENABLE | AUXSPIMODE  | AUXSPIHOLD | (1<<1)| (1<<0);


#define AUXSPION() CARD_CR1 = AUXSPIENABLE | AUXSPIMODE  | AUXSPIHOLD | (speed)

#define AUXSPIHOLDON() CARD_CR1 |= AUXSPIHOLD
#define AUXSPIHOLDOFF() CARD_CR1 &= ~AUXSPIHOLD

// disables SPI bus
#define AUXSPIOFF() CARD_CR1 = 0
