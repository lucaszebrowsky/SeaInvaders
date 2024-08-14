#include "shift_register.h"
#include <stdint.h>
// #include <stdio.h>
// #include <stdio.h>
// #include <stdlib.h>

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

// TODO: This needs to be looked at
uint8_t getShiftRegister(void)
{
	uint16_t result = shiftRegister >> (8 - shiftOffset);
	// printf("Shift Register: %x, Offset: %x, Result: %x\n", shiftRegister,
	// 	   shiftOffset, result);
	return (uint8_t)(result & 0xFF);

	// uint16_t mask = 0xFF00 >> shiftOffset;

	// uint16_t result = shiftRegister & mask;

	// if (shiftRegister != 0 && shiftOffset != 0) {
	// 	printf("Data: %x, Offset: %x, result: %x (Calc: %x)\n", shiftRegister,
	// 		   shiftOffset, result, (uint8_t)(result >> (8 - shiftOffset)));
	// 	exit(1);
	// }

	// return (uint8_t)(result >> (8 - shiftOffset));
}
