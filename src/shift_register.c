#include "shift_register.h"
#include <stdint.h>

static uint16_t shiftRegister = 0;
static uint8_t shiftOffset = 0;

void setShiftRegister(uint8_t data)
{
	shiftRegister = (shiftRegister >> 8) | (data << 8);
}

void setShiftOffset(uint8_t offset)
{
	shiftOffset = offset & 0x7;
}

uint8_t getShiftRegister(void)
{
	uint16_t result = shiftRegister >> (8 - shiftOffset);
	return (uint8_t)(result & 0xFF);
}
