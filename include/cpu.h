#pragma once
#include <stdint.h>

typedef union _reg {
	struct {
		uint8_t lowByte;
		uint8_t highByte;
	};
	uint16_t reg;
} reg_t;

typedef struct cpu {
	reg_t AF; // Register Pair A and Flags
	reg_t BC; // Register Pair B and C
	reg_t DE; // Register Pair D and E
	reg_t HL; // Register Pair H and L
	uint16_t SP; // Stack Pointer
	uint16_t PC; // Programm Counter
	uint8_t opcode;
	uint8_t io_port[0x8];
	uint8_t interrupt;
	uint8_t interrupt_enabled;
} cpu_t;

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

// Initialize the CPU Registers
void initCPU(cpu_t *cpu);

// Fetch next instruction, execute it and return the cycles it took
uint8_t executeNextInstruction(cpu_t *cpu);

// Set the Interrupt Subroutine (Interrupts need to be enabled)
void setInterruptRoutine(cpu_t *cpu, uint8_t interrupt);
