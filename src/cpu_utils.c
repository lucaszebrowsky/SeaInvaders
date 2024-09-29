#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cpu_utils.h"
#include "cpu.h"
#include "bus.h"

uint8_t *get_reg8(cpu_t *cpu, int n)
{
	switch (n) {
	case 0x0:
		return &cpu->BC.highByte;
	case 0x1:
		return &cpu->BC.lowByte;
	case 0x2:
		return &cpu->DE.highByte;
	case 0x3:
		return &cpu->DE.lowByte;
	case 0x4:
		return &cpu->HL.highByte;
	case 0x5:
		return &cpu->HL.lowByte;
	case 0x6:
		return getAddressPointer(cpu->HL.reg);
	case 0x7:
		return &cpu->AF.highByte;
	default:
		printf("get_reg8: invalid number: %x\n", n);
		exit(EXIT_FAILURE);
		return NULL;
	}
}

uint16_t *get_reg16(cpu_t *cpu, int n)
{
	switch (n) {
	case 0x0:
		return &cpu->BC.reg;
	case 0x1:
		return &cpu->DE.reg;
	case 0x2:
		return &cpu->HL.reg;
	case 0x3:
		return &cpu->SP;
	default:
		printf("get_reg16: invalid number: %x\n", n);
		exit(EXIT_FAILURE);
		return NULL;
	}
}
