#include <stdio.h>

#include "cpu.h"
#include "shift_register.h"

static uint16_t shiftRegister = 0;
static uint8_t shiftOffset = 0;

void setShiftRegister(uint8_t data)
{
	shiftRegister = shiftRegister >> (8 - shiftOffset) | data << 8;
}

void setShiftOffset(uint8_t offset)
{
	shiftOffset = offset;
}

uint8_t getShiftRegister(void)
{
	return shiftRegister >> 8;
}
