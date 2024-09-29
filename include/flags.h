#pragma once

#include "cpu.h"
#include <stdint.h>

/*
 *   Flags
 *
 *   Bit:    7 6 5 4 3 2 1 0
 *   Flag:   S Z 0 A 0 P 1 C
 *
 *   S - Sign Flag
 *   Z - Zero Flag
 *   0 - Not used, always zero
 *   A - also called AC, Auxiliary Carry Flag
 *   0 - Not used, always zero
 *   P - Parity Flag
 *   1 - Not used, always one
 *   C - Carry Flag
 */
enum FLAGS {
	CARRY = 0x1,
	PARITY = 0x4,
	AUXCARRY = 0x10,
	ZERO = 0x40,
	SIGN = 0x80
};

void handle_zero(cpu_t *cpu, uint8_t value);
void handle_parity(cpu_t *cpu, uint8_t byte);
void handle_sign(cpu_t *cpu, uint8_t byte);

void handle_carry8(cpu_t *cpu, uint8_t byte1, uint8_t byte2,
				   uint8_t isSubtraction);
void handle_carry16(cpu_t *cpu, uint16_t word1, uint16_t word2,
					uint8_t isSubtraction);

void handle_halfcarry8(cpu_t *cpu, uint8_t byte1, uint8_t byte2,
					   uint8_t isSubtraction);

// void handle_halfcarry16(cpu_t *cpu, uint16_t word1, uint16_t word2,
// uint8_t isSubtraction);

void set_flag(cpu_t *cpu, enum FLAGS flag);
void clear_flag(cpu_t *cpu, enum FLAGS flag);
