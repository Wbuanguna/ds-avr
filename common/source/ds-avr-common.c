#include <stdint.h> 

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
