#define BUFFER_LENGTH 20

#define NULL_COMMAND 0x00
#define FLASH_LED_COMMAND 0x01


#define ANALOG_READ_COMMAND 0x03
#define PING_COMMAND 0x04
#define READ_INCREMENT_COMMAND 0x5

#define IO_REG_GET_COMMAND 0x11
#define IO_REG_SET_COMMAND 0x12


#define PWM_ON_COMMAND  0x19
#define PWM_SET_COMMAND 0x20
#define PWM_OFF_COMMAND 0x21

enum {
    PWM0, //OC0A pin 6
    PWM1, //OC0B pin 5
    PWM2, //OC1A pin 9 (16 bit) Note: pin used for SPI
    PWM3, //OC1B pin 10 (16 bit) Note: pin used for SPI
    PWM4, //OC2A pin 11  Note: pin used for SPI
    PWM5, //OC2B pin 3
};
