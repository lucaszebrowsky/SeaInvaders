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
	uint8_t io_port[0x8];
	reg_t AF; // Register Pair A and Flags
	reg_t BC; // Register Pair B and C
	reg_t DE; // Register Pair D and E
	reg_t HL; // Register Pair H and L
	uint16_t SP; // Stack Pointer
	uint16_t PC; // Programm Counter
	uint8_t opcode;
	uint8_t interrupt;
	uint8_t interrupt_enabled;
} cpu_t;

// Initialize the CPU Registers
void initCPU(cpu_t *cpu);

// Fetch next instruction, execute it and return the cycles it took
uint8_t step(cpu_t *cpu);

// Set the Interrupt Subroutine (Interrupts need to be enabled)
void setInterruptRoutine(cpu_t *cpu, uint8_t interrupt);
