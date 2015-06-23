
#include "ds-spi.h"

#define WAIT_CYCLES 740 // 740 = 44 us

#ifdef ARM9
#define WAIT_1MS 16667L
#define COMMAND_WAIT 8750L
#elif defined(ARM7)
#define COMMAND_WAIT 8750L
#define WAIT_1MS 8333L
#endif


#define FIFO_AVR FIFO_USER_06


typedef enum {
	PROFILE_MESSAGE     = 0x2343,
	CONFIG_MESSAGE      = 0x2344,
	LED_FLASH_MESSAGE   = 0x2345,
	ANALOG_READ_MESSAGE = 0x2346,
	IO_PORT_GET_MESSAGE = 0x2347,
	IO_PORT_SET_MESSAGE = 0x2348,
	PWM_MESSAGE         = 0x2349,
} AvrFifoMessageType;

typedef enum {
	PWM_ON,
	PWM_OFF,
	PWM_SET,
} AvrPwmCommand;



typedef struct AvrFifoMessage {
	uint16_t type;

	union {
		struct {
			uint16_t m1, m2;
			uint8_t debug;
		} SPIConfig;
	
		struct {
			uint8_t count;
		} LEDFlash;
		
		struct {
			uint8_t pin;
		} AnalogRead;
		
		struct {
		    uint8_t address;
		    uint8_t value;
		} IOPortGetSet;
		
		struct {
			uint8_t command;
			uint8_t output;
			uint8_t mode;
			uint16_t value;
		} PWM;
	};

} ALIGN(4) AvrFifoMessage;

/* ds-avr-common.c */
uint8_t CRC8(uint8_t input, uint8_t seed);

/* ds-spi.c */
extern int shortwait;
extern uint8_t incoming[BUFFER_LENGTH];
extern uint8_t speed;
extern uint8_t spi_debug;
uint8_t send_chars(int length, ...);
uint8_t read_increment(uint8_t n);
