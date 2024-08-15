#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"
#include "bus.h"
#include "shift_register.h"

#define CPU_CRASH(cpu) cpu_crash(cpu, __FILE__, __LINE__)

void initCPU(cpu_t *cpu)
{
	cpu->AF.reg = 0x2; // Bit 1 of Flag-Register is always set to 1
	cpu->BC.reg = 0;
	cpu->DE.reg = 0;
	cpu->HL.reg = 0;

	cpu->SP = 0;
	// cpu->PC = 0;
	cpu->PC = 0x100;

	cpu->interrupt = 0;

	for (int i = 0; i < 8; i++) {
		cpu->io_port[i] = 0;
	}

	cpu->io_port[0] = 0xE;
	cpu->io_port[1] |= (1 << 3);
}

// Exit the program and print the last cpu state to the console
void cpu_crash(cpu_t *cpu, char *file, int line)
{
	fprintf(stderr, "Emulator crashed in %s at line %d!\nLast CPU state:\n",
			file, line);
	fprintf(stderr, "A: %02x\tF: %02x\n", cpu->AF.highByte, cpu->AF.lowByte);
	fprintf(stderr, "B: %02x\tC: %02x\n", cpu->BC.highByte, cpu->BC.lowByte);
	fprintf(stderr, "D: %02x\tE: %02x\n", cpu->DE.highByte, cpu->DE.lowByte);
	fprintf(stderr, "H: %02x\tL: %02x\n", cpu->HL.highByte, cpu->HL.lowByte);
	fprintf(stderr, "PC: %04x\tSP: %04x\n", cpu->PC, cpu->SP);
	fprintf(stderr, "Opcode: %02x\n", cpu->opcode);
	exit(EXIT_FAILURE);
}

// Set the specified Flag if the condition is true
void setFlag(cpu_t *cpu, uint8_t condition, enum FLAGS flag)
{
	if (condition) {
		cpu->AF.lowByte |= flag;
	} else {
		cpu->AF.lowByte &= ~flag;
	}
}

// Count the number of bits that are set,
// returns 0 on odd, 1 on even
uint8_t parity(uint8_t byte)
{
	uint8_t count = 0;

	for (int i = 0; i < 8; i++) {
		if (byte & (1 << i)) {
			count++;
		}
	}

	return !(count % 2);
}

// No Operation
uint8_t NOP(cpu_t *cpu)
{
	cpu->PC++;
	return 4;
}

// Increase Register or Memory by 1, flags affected
uint8_t INR(cpu_t *cpu)
{
	uint8_t *result = NULL;

	switch (cpu->opcode) {
	case 0x04:
		result = &cpu->BC.highByte;
		break;
	case 0x0C:
		result = &cpu->BC.lowByte;
		break;
	case 0x14:
		result = &cpu->DE.highByte;
		break;
	case 0x1C:
		result = &cpu->DE.lowByte;
		break;
	case 0x24:
		result = &cpu->HL.highByte;
		break;
	case 0x2C:
		result = &cpu->HL.lowByte;
		break;
	case 0x34:
		result = getAddressPointer(cpu->HL.reg);
		break;
	case 0x3C:
		result = &cpu->AF.highByte;
		break;

	default:
		CPU_CRASH(cpu);
		break;
	}

	setFlag(cpu, ((*result & 0x8) != ((*result + 1) & 0x8)), AUXCARRY);

	(*result)++;

	setFlag(cpu, (*result & 0x80), SIGN);
	setFlag(cpu, (*result == 0), ZERO);
	setFlag(cpu, parity(*result), PARITY);

	cpu->PC++;
	return (cpu->opcode == 0x34 ? 10 : 5);
}

// Increase Register by 1, flags NOT affected
uint8_t INX(cpu_t *cpu)
{
	switch (cpu->opcode) {
	case 0x03:
		cpu->BC.reg++;
		break;
	case 0x13:
		cpu->DE.reg++;
		break;
	case 0x23:
		cpu->HL.reg++;
		break;
	case 0x33:
		cpu->SP++;
		break;

	default:
		CPU_CRASH(cpu);
		break;
	}

	cpu->PC++;
	return 5;
}

// Decrease Register by 1
uint8_t DCR(cpu_t *cpu)
{
	uint8_t *result = NULL;

	switch (cpu->opcode) {
	case 0x05:
		result = &cpu->BC.highByte;
		break;
	case 0x0D:
		result = &cpu->BC.lowByte;
		break;
	case 0x15:
		result = &cpu->DE.highByte;
		break;
	case 0x1D:
		result = &cpu->DE.lowByte;
		break;
	case 0x25:
		result = &cpu->HL.highByte;
		break;
	case 0x2D:
		result = &cpu->HL.lowByte;
		break;
	case 0x35:
		result = getAddressPointer(cpu->HL.reg);
		break;
	case 0x3D:
		result = &cpu->AF.highByte;
		break;

	default:
		CPU_CRASH(cpu);
		break;
	}

	setFlag(cpu, ((*result & 0x8) != ((*result - 1) & 0x8)), AUXCARRY);

	(*result)--;

	setFlag(cpu, ((*result) & 0x80), SIGN);
	setFlag(cpu, ((*result) == 0), ZERO);
	setFlag(cpu, parity(*result), PARITY);

	cpu->PC++;
	return (cpu->opcode == 0x35 ? 10 : 5);
}

// Decrease Register by 1, flags NOT affected
uint8_t DCX(cpu_t *cpu)
{
	switch (cpu->opcode) {
	case 0x0B:
		cpu->BC.reg--;
		break;
	case 0x1B:
		cpu->DE.reg--;
		break;
	case 0x2B:
		cpu->HL.reg--;
		break;
	case 0x3B:
		cpu->SP--;
		break;

	default:
		CPU_CRASH(cpu);
		break;
	}

	cpu->PC++;
	return 5;
}

// XOR the Carry Bit
uint8_t CMC(cpu_t *cpu)
{
	cpu->AF.lowByte ^= CARRY;
	cpu->PC++;
	return 4;
}

// Complement the Accumulator
uint8_t CMA(cpu_t *cpu)
{
	cpu->AF.highByte = ~cpu->AF.highByte;
	cpu->PC++;
	return 4;
}

// Store Accumulator at (BC), (DE)  or given address (STA and STAX)
uint8_t STA(cpu_t *cpu)
{
	switch (cpu->opcode) {
	case 0x02:
		// STAX B
		writeByteToMemory(cpu->AF.highByte, cpu->BC.reg);
		break;
	case 0x12:
		// STAX D
		writeByteToMemory(cpu->AF.highByte, cpu->DE.reg);
		break;
	case 0x32:
		// STA a16
		cpu->PC++;
		uint8_t lowByte = readMemoryValue(cpu->PC);
		cpu->PC++;
		uint8_t highByte = readMemoryValue(cpu->PC);

		uint16_t address = highByte << 8 | lowByte;

		writeByteToMemory(cpu->AF.highByte, address);
		break;

	default:
		CPU_CRASH(cpu);
		break;
	}

	cpu->PC++;
	return (cpu->opcode == 0x32 ? 13 : 7);
}

// Load next 2 Bytes into a Register Pair
uint8_t LXI(cpu_t *cpu)
{
	cpu->PC++;
	uint8_t lowByte = readMemoryValue(cpu->PC);
	cpu->PC++;
	uint8_t highByte = readMemoryValue(cpu->PC);

	switch (cpu->opcode) {
	case 0x01:
		//LXI B,d16
		cpu->BC.reg = highByte << 8 | lowByte;
		break;
	case 0x11:
		// LXI D,d16
		cpu->DE.reg = highByte << 8 | lowByte;
		break;
	case 0x21:
		// LXI H,d16
		cpu->HL.reg = highByte << 8 | lowByte;
		break;
	case 0x31:
		// LXI SP,d16
		cpu->SP = highByte << 8 | lowByte;
		break;

	default:
		CPU_CRASH(cpu);
		break;
	}

	cpu->PC++;
	return 10;
}

// Move next byte into Register or memory location
uint8_t MVI(cpu_t *cpu)
{
	cpu->PC++;
	uint8_t value = readMemoryValue(cpu->PC);

	switch (cpu->opcode) {
	case 0x06:
		// MVI B, d8
		cpu->BC.highByte = value;
		break;
	case 0x0E:
		// MVI C, d8
		cpu->BC.lowByte = value;
		break;
	case 0x16:
		// MVI D, d8
		cpu->DE.highByte = value;
		break;
	case 0x1E:
		// MVI E, d8
		cpu->DE.lowByte = value;
		break;
	case 0x26:
		// MVI H, d8
		cpu->HL.highByte = value;
		break;
	case 0x2E:
		// MVI L, d8
		cpu->HL.lowByte = value;
		break;
	case 0x36:
		// MVI M, d8
		writeByteToMemory(value, cpu->HL.reg);
		break;
	case 0x3E:
		// MVI A, d8
		cpu->AF.highByte = value;
		break;

	default:
		CPU_CRASH(cpu);
		break;
	}

	cpu->PC++;
	return (cpu->opcode == 0x36 ? 10 : 7);
}

// Load byte at (BC),(DE) or given address into Accumulator (LDA and LDAX)
uint8_t LDA(cpu_t *cpu)
{
	uint8_t value = 0;

	switch (cpu->opcode) {
	case 0x0A:
		value = readMemoryValue(cpu->BC.reg);
		break;
	case 0x1A:
		value = readMemoryValue(cpu->DE.reg);
		break;
	case 0x3A:
		cpu->PC++;
		uint8_t lowByte = readMemoryValue(cpu->PC);
		cpu->PC++;
		uint8_t highByte = readMemoryValue(cpu->PC);

		uint16_t address = highByte << 8 | lowByte;

		value = readMemoryValue(address);
		break;

	default:
		CPU_CRASH(cpu);
		break;
	}

	cpu->AF.highByte = value;
	cpu->PC++;
	return (cpu->opcode == 0x3A ? 13 : 7);
}

// Load next 2 Bytes into HL
uint8_t LHLD(cpu_t *cpu)
{
	cpu->PC++;
	uint8_t lowByte = readMemoryValue(cpu->PC);
	cpu->PC++;
	uint8_t highByte = readMemoryValue(cpu->PC);

	uint16_t address = highByte << 8 | lowByte;

	cpu->HL.lowByte = readMemoryValue(address);
	cpu->HL.highByte = readMemoryValue(address + 1);

	cpu->PC++;
	return 16;
}

// Set Carry Flag
uint8_t STC(cpu_t *cpu)
{
	cpu->AF.lowByte |= CARRY;
	cpu->PC++;
	return 4;
}

// Store L in memory at address and H at address + 1
uint8_t SHLD(cpu_t *cpu)
{
	cpu->PC++;
	uint8_t lowByte = readMemoryValue(cpu->PC);
	cpu->PC++;
	uint8_t highByte = readMemoryValue(cpu->PC);

	uint16_t address = highByte << 8 | lowByte;

	writeByteToMemory(cpu->HL.lowByte, address);
	writeByteToMemory(cpu->HL.highByte, address + 1);

	cpu->PC++;
	return 16;
}

// Move register or memory content into another register or memory address
uint8_t MOV(cpu_t *cpu)
{
	switch (cpu->opcode) {
	case 0x40:
		// MOV B,B
		cpu->BC.highByte = cpu->BC.highByte;
		break;
	case 0x41:
		// MOV B,C
		cpu->BC.highByte = cpu->BC.lowByte;
		break;
	case 0x42:
		// MOV B,D
		cpu->BC.highByte = cpu->DE.highByte;
		break;
	case 0x43:
		// MOV B,E
		cpu->BC.highByte = cpu->DE.lowByte;
		break;
	case 0x44:
		// MOV B,H
		cpu->BC.highByte = cpu->HL.highByte;
		break;
	case 0x45:
		// MOV B,L
		cpu->BC.highByte = cpu->HL.lowByte;
		break;
	case 0x46:
		// MOV B,M
		cpu->BC.highByte = readMemoryValue(cpu->HL.reg);
		break;
	case 0x47:
		// MOV B,A
		cpu->BC.highByte = cpu->AF.highByte;
		break;
	case 0x48:
		// MOV C,B
		cpu->BC.lowByte = cpu->BC.highByte;
		break;
	case 0x49:
		// MOV C,C
		cpu->BC.lowByte = cpu->BC.lowByte;
		break;
	case 0x4A:
		// MOV C,D
		cpu->BC.lowByte = cpu->DE.highByte;
		break;
	case 0x4B:
		// MOV C,E
		cpu->BC.lowByte = cpu->DE.lowByte;
		break;
	case 0x4C:
		// MOV C,H
		cpu->BC.lowByte = cpu->HL.highByte;
		break;
	case 0x4D:
		// MOV C,L
		cpu->BC.lowByte = cpu->HL.lowByte;
		break;
	case 0x4E:
		// MOV C,M
		cpu->BC.lowByte = readMemoryValue(cpu->HL.reg);
		break;
	case 0x4F:
		// MOV C,A
		cpu->BC.lowByte = cpu->AF.highByte;
		break;
	case 0x50:
		// MOV D,B
		cpu->DE.highByte = cpu->BC.highByte;
		break;
	case 0x51:
		// MOV D,C
		cpu->DE.highByte = cpu->BC.lowByte;
		break;
	case 0x52:
		// MOV D,D
		cpu->DE.highByte = cpu->DE.highByte;
		break;
	case 0x53:
		// MOV D,E
		cpu->DE.highByte = cpu->DE.lowByte;
		break;
	case 0x54:
		// MOV D,H
		cpu->DE.highByte = cpu->HL.highByte;
		break;
	case 0x55:
		// MOV D,L
		cpu->DE.highByte = cpu->HL.lowByte;
		break;
	case 0x56:
		// MOV D,M
		cpu->DE.highByte = readMemoryValue(cpu->HL.reg);
		break;
	case 0x57:
		// MOV D,A
		cpu->DE.highByte = cpu->AF.highByte;
		break;
	case 0x58:
		// MOV E,B
		cpu->DE.lowByte = cpu->BC.highByte;
		break;
	case 0x59:
		// MOV E,C
		cpu->DE.lowByte = cpu->BC.lowByte;
		break;
	case 0x5A:
		// MOV E,D
		cpu->DE.lowByte = cpu->DE.highByte;
		break;
	case 0x5B:
		// MOV E,E
		cpu->DE.lowByte = cpu->DE.lowByte;
		break;
	case 0x5C:
		// MOV E,H
		cpu->DE.lowByte = cpu->HL.highByte;
		break;
	case 0x5D:
		// MOV E,L
		cpu->DE.lowByte = cpu->HL.lowByte;
		break;
	case 0x5E:
		// MOV E,M
		cpu->DE.lowByte = readMemoryValue(cpu->HL.reg);
		break;
	case 0x5F:
		// MOV E,A
		cpu->DE.lowByte = cpu->AF.highByte;
		break;
	case 0x60:
		// MOV H,B
		cpu->HL.highByte = cpu->BC.highByte;
		break;
	case 0x61:
		// MOV H,C
		cpu->HL.highByte = cpu->BC.lowByte;
		break;
	case 0x62:
		// MOV H,D
		cpu->HL.highByte = cpu->DE.highByte;
		break;
	case 0x63:
		// MOV H,E
		cpu->HL.highByte = cpu->DE.lowByte;
		break;
	case 0x64:
		// MOV H,H
		cpu->HL.highByte = cpu->HL.highByte;
		break;
	case 0x65:
		// MV H,L
		cpu->HL.highByte = cpu->HL.lowByte;
		break;
	case 0x66:
		// MOV H,M
		cpu->HL.highByte = readMemoryValue(cpu->HL.reg);
		break;
	case 0x67:
		// MOV H,A
		cpu->HL.highByte = cpu->AF.highByte;
		break;
	case 0x68:
		// MOV L,B
		cpu->HL.lowByte = cpu->BC.highByte;
		break;
	case 0x69:
		// MOV L,C
		cpu->HL.lowByte = cpu->BC.lowByte;
		break;
	case 0x6A:
		// MOV L,D
		cpu->HL.lowByte = cpu->DE.highByte;
		break;
	case 0x6B:
		// MOV L,E
		cpu->HL.lowByte = cpu->DE.lowByte;
		break;
	case 0x6C:
		// MOV L,H
		cpu->HL.lowByte = cpu->HL.highByte;
		break;
	case 0x6D:
		// MOV L,L
		cpu->HL.lowByte = cpu->HL.lowByte;
		break;
	case 0x6E:
		// MOV L,M
		cpu->HL.lowByte = readMemoryValue(cpu->HL.reg);
		break;
	case 0x6F:
		// MOV L,A
		cpu->HL.lowByte = cpu->AF.highByte;
		break;
	case 0x70:
		// MOV M,B
		writeByteToMemory(cpu->BC.highByte, cpu->HL.reg);
		break;
	case 0x71:
		// MOV M,C
		writeByteToMemory(cpu->BC.lowByte, cpu->HL.reg);
		break;
	case 0x72:
		// MOV M,D
		writeByteToMemory(cpu->DE.highByte, cpu->HL.reg);
		break;
	case 0x73:
		// MOV M,E
		writeByteToMemory(cpu->DE.lowByte, cpu->HL.reg);
		break;
	case 0x74:
		// MOV M,H
		writeByteToMemory(cpu->HL.highByte, cpu->HL.reg);
		break;
	case 0x75:
		// MOV M,L
		writeByteToMemory(cpu->HL.lowByte, cpu->HL.reg);
		break;
	case 0x77:
		// MOV M,A
		writeByteToMemory(cpu->AF.highByte, cpu->HL.reg);
		break;
	case 0x78:
		// MOV A,B
		cpu->AF.highByte = cpu->BC.highByte;
		break;
	case 0x79:
		// MOV A,C
		cpu->AF.highByte = cpu->BC.lowByte;
		break;
	case 0x7A:
		// MOV A,D
		cpu->AF.highByte = cpu->DE.highByte;
		break;
	case 0x7B:
		// MOV A,E
		cpu->AF.highByte = cpu->DE.lowByte;
		break;
	case 0x7C:
		// MOV A,H
		cpu->AF.highByte = cpu->HL.highByte;
		break;
	case 0x7D:
		// MOV A,L
		cpu->AF.highByte = cpu->HL.lowByte;
		break;
	case 0x7E:
		// A,M
		cpu->AF.highByte = readMemoryValue(cpu->HL.reg);
		break;
	case 0x7F:
		// MOV A,A
		cpu->AF.highByte = cpu->AF.highByte;
		break;

	default:
		CPU_CRASH(cpu);
		break;
	}

	uint8_t cycles_lookup[64] = {
		5, 5, 5, 5, 5, 5, 7, 5, 5, 5, 5, 5, 5, 5, 7, 5, 5, 5, 5, 5, 5, 5,
		7, 5, 5, 5, 5, 5, 5, 5, 7, 5, 5, 5, 5, 5, 5, 5, 7, 5, 5, 5, 5, 5,
		5, 5, 7, 5, 7, 7, 7, 7, 7, 7, 0, 7, 5, 5, 5, 5, 5, 5, 7, 5,
	};

	cpu->PC++;
	return cycles_lookup[cpu->opcode - 0x40];
}

// Add content of Register to Accumulator
uint8_t ADD(cpu_t *cpu)
{
	uint8_t value = 0;

	switch (cpu->opcode) {
	case 0x80:
		// ADD B
		value = cpu->BC.highByte;
		break;
	case 0x81:
		// ADD C
		value = cpu->BC.lowByte;
		break;
	case 0x82:
		// ADD D
		value = cpu->DE.highByte;
		break;
	case 0x83:
		// ADD E
		value = cpu->DE.lowByte;
		break;
	case 0x84:
		// ADD H
		value = cpu->HL.highByte;
		break;
	case 0x85:
		// ADD L
		value = cpu->HL.lowByte;
		break;
	case 0x86:
		// ADD M
		value = readMemoryValue(cpu->HL.reg);
		break;
	case 0x87:
		// ADD A
		value = cpu->AF.highByte;
		break;

	default:
		CPU_CRASH(cpu);
		break;
	}

	setFlag(cpu, ((cpu->AF.highByte + value) > 0xFF), CARRY);
	setFlag(cpu,
			((cpu->AF.highByte & 0x8) != ((cpu->AF.highByte + value) & 0x8)),
			AUXCARRY);

	cpu->AF.highByte += value;

	setFlag(cpu, cpu->AF.highByte == 0, ZERO);
	setFlag(cpu, cpu->AF.highByte & 0x80, SIGN);
	setFlag(cpu, parity(cpu->AF.highByte), PARITY);

	cpu->PC++;
	return (cpu->opcode == 0x86) ? 7 : 4;
}

// Add next Byte to Accumulator
uint8_t ADI(cpu_t *cpu)
{
	cpu->PC++;
	uint8_t value = readMemoryValue(cpu->PC);

	setFlag(cpu, ((cpu->AF.highByte + value) > 0xFF), CARRY);
	setFlag(cpu,
			((cpu->AF.highByte & 0x8) != ((cpu->AF.highByte + value) & 0x8)),
			AUXCARRY);

	printf("Before ADI A: %x, Val: %x, F: %x\n", cpu->AF.highByte, value,
		   cpu->AF.lowByte);
	cpu->AF.highByte += value;
	printf("After ADI A: %x, Val: %x, F: %x\n", cpu->AF.highByte, value,
		   cpu->AF.lowByte);
	printf("Condidtion: %x\n", parity(cpu->AF.highByte));

	setFlag(cpu, cpu->AF.highByte == 0, ZERO);
	setFlag(cpu, cpu->AF.highByte & 0x80, SIGN);
	setFlag(cpu, parity(cpu->AF.highByte), PARITY);

	cpu->PC++;
	return 7;
}

// Add next byte and carry to Accumulator
uint8_t ACI(cpu_t *cpu)
{
	cpu->PC++;
	uint8_t val = readMemoryValue(cpu->PC);
	uint8_t carry = cpu->AF.lowByte & 1;

	setFlag(cpu, ((cpu->AF.highByte + val + carry) > 0xFF), CARRY);
	setFlag(cpu,
			((cpu->AF.highByte & 0x8) !=
			 ((cpu->AF.highByte + val + carry) & 0x8)),
			AUXCARRY);

	cpu->AF.highByte = cpu->AF.highByte + val + carry;

	setFlag(cpu, cpu->AF.highByte == 0, ZERO);
	setFlag(cpu, cpu->AF.highByte & 0x80, SIGN);
	setFlag(cpu, parity(cpu->AF.highByte), PARITY);

	cpu->PC++;
	return 7;
}

// Add content of Register and carry-bit to Accumulator
uint8_t ADC(cpu_t *cpu)
{
	uint8_t carry = cpu->AF.lowByte & 1;
	uint8_t value = 0;

	switch (cpu->opcode) {
	case 0x88:
		// ADC B
		value = cpu->BC.highByte;
		break;
	case 0x89:
		// ADC C
		value = cpu->BC.lowByte;
		break;
	case 0x8A:
		// ADC D
		value = cpu->DE.highByte;
		break;
	case 0x8B:
		// ADC E
		value = cpu->DE.lowByte;
		break;
	case 0x8C:
		// ADC H
		value = cpu->HL.highByte;
		break;
	case 0x8D:
		// ADC L
		value = cpu->HL.lowByte;
		break;
	case 0x8E:
		// ADC M
		value = readMemoryValue(cpu->HL.reg);
		break;
	case 0x8F:
		// ADC A
		value = cpu->AF.highByte;
		break;
	default:
		CPU_CRASH(cpu);
		break;
	}

	setFlag(cpu, ((cpu->AF.highByte + value + carry) > 0xFF), CARRY);
	setFlag(cpu,
			((cpu->AF.highByte & 0x8) !=
			 ((cpu->AF.highByte + value + carry) & 0x8)),
			AUXCARRY);

	cpu->AF.highByte = cpu->AF.highByte + value + carry;

	setFlag(cpu, cpu->AF.highByte == 0, ZERO);
	setFlag(cpu, cpu->AF.highByte & 0x80, SIGN);
	// setFlag(cpu, !(cpu->AF.highByte & 0x1), PARITY);
	setFlag(cpu, parity(cpu->AF.highByte), PARITY);

	cpu->PC++;
	return (cpu->opcode == 0x8E) ? 7 : 4;
}

// Bitwise AND Accumulator with content of register or memory
uint8_t ANA(cpu_t *cpu)
{
	uint8_t value = 0;

	switch (cpu->opcode) {
	case 0xA0:
		// ANA B
		value = cpu->BC.highByte;
		break;
	case 0xA1:
		// ANA C
		value = cpu->BC.lowByte;
		break;
	case 0xA2:
		// ANA D
		value = cpu->DE.highByte;
		break;
	case 0xA3:
		// ANA E
		value = cpu->DE.lowByte;
		break;
	case 0xA4:
		// ANA H
		value = cpu->HL.highByte;
		break;
	case 0xA5:
		// ANA L
		value = cpu->HL.lowByte;
		break;
	case 0xA6:
		// ANA M
		value = readMemoryValue(cpu->HL.reg);
		break;
	case 0xA7:
		// ANA A
		value = cpu->AF.highByte;
		break;

	default:
		CPU_CRASH(cpu);
		break;
	}

	setFlag(cpu,
			((cpu->AF.highByte & 0x8) != ((cpu->AF.highByte & value) & 0x8)),
			AUXCARRY);

	cpu->AF.highByte &= value;

	setFlag(cpu, 0, CARRY);
	setFlag(cpu, cpu->AF.highByte == 0, ZERO);
	setFlag(cpu, cpu->AF.highByte & 0x80, SIGN);
	setFlag(cpu, parity(cpu->AF.highByte), PARITY);

	cpu->PC++;
	return (cpu->opcode == 0xA6) ? 7 : 4;
}

// Bitwise And Accumulator with next Byte
uint8_t ANI(cpu_t *cpu)
{
	cpu->PC++;
	uint8_t val = readMemoryValue(cpu->PC);
	cpu->AF.highByte &= val;

	setFlag(cpu, 0, CARRY);
	setFlag(cpu, 0, AUXCARRY);
	setFlag(cpu, cpu->AF.highByte == 0, ZERO);
	setFlag(cpu, cpu->AF.highByte & 0x80, SIGN);
	setFlag(cpu, parity(cpu->AF.highByte), PARITY);

	cpu->PC++;
	return 7;
}

// Bitwise XOR Accumulator with content of register or memory
uint8_t XRA(cpu_t *cpu)
{
	uint8_t value = 0;

	switch (cpu->opcode) {
	case 0xA8:
		// XRA B
		value = cpu->BC.highByte;
		break;
	case 0xA9:
		// XRA C
		value = cpu->BC.lowByte;
		break;
	case 0xAA:
		// XRA D
		value = cpu->DE.highByte;
		break;
	case 0xAB:
		// XRA E
		value = cpu->DE.lowByte;
		break;
	case 0xAC:
		// XRA H
		value = cpu->HL.highByte;
		break;
	case 0xAD:
		// XRA L
		value = cpu->HL.lowByte;
		break;
	case 0xAE:
		// XRA M
		value = readMemoryValue(cpu->HL.reg);
		break;
	case 0xAF:
		// XRA A
		value = cpu->AF.highByte;
		break;

	default:
		CPU_CRASH(cpu);
		break;
	}

	cpu->AF.highByte ^= value;

	setFlag(cpu, 0, CARRY);
	setFlag(cpu, 0, AUXCARRY);
	setFlag(cpu, cpu->AF.highByte == 0, ZERO);
	setFlag(cpu, cpu->AF.highByte & 0x80, SIGN);
	setFlag(cpu, parity(cpu->AF.highByte), PARITY);

	cpu->PC++;
	return (cpu->opcode == 0xAE) ? 7 : 4;
}

// Bitwise XOR Accumulator with next byte
uint8_t XRI(cpu_t *cpu)
{
	cpu->PC++;
	uint8_t val = readMemoryValue(cpu->PC);

	cpu->AF.highByte ^= val;

	setFlag(cpu, 0, CARRY);
	setFlag(cpu, 0, AUXCARRY);
	setFlag(cpu, cpu->AF.highByte == 0, ZERO);
	setFlag(cpu, cpu->AF.highByte & 0x80, SIGN);
	setFlag(cpu, parity(cpu->AF.highByte), PARITY);

	cpu->PC++;
	return 7;
}

// Bitwise OR Accumulator with content of register or memory
uint8_t ORA(cpu_t *cpu)
{
	uint8_t value = 0;

	switch (cpu->opcode) {
	case 0xB0:
		// ORA B
		value = cpu->BC.highByte;
		break;
	case 0xB1:
		// ORA C
		value = cpu->BC.lowByte;
		break;
	case 0xB2:
		// ORA D
		value = cpu->DE.highByte;
		break;
	case 0xB3:
		// ORA E
		value = cpu->DE.lowByte;
		break;
	case 0xB4:
		// ORA H
		value = cpu->HL.highByte;
		break;
	case 0xB5:
		// ORA L
		value = cpu->HL.lowByte;
		break;
	case 0xB6:
		// ORA M
		value = readMemoryValue(cpu->HL.reg);
		break;
	case 0xB7:
		// ORA A
		value = cpu->AF.highByte;
		break;

	default:
		CPU_CRASH(cpu);
		break;
	}

	cpu->AF.highByte |= value;

	setFlag(cpu, 0, CARRY);
	setFlag(cpu, 0, AUXCARRY);
	setFlag(cpu, cpu->AF.highByte == 0, ZERO);
	setFlag(cpu, cpu->AF.highByte & 0x80, SIGN);
	setFlag(cpu, parity(cpu->AF.highByte), PARITY);

	cpu->PC++;
	return (cpu->opcode == 0xB6) ? 7 : 4;
}

// Bitwise OR Accumulator with next Byte
uint8_t ORI(cpu_t *cpu)
{
	cpu->PC++;
	uint8_t val = readMemoryValue(cpu->PC);

	cpu->AF.highByte |= val;

	setFlag(cpu, 0, CARRY);
	setFlag(cpu, 0, AUXCARRY);
	setFlag(cpu, cpu->AF.highByte == 0, ZERO);
	setFlag(cpu, cpu->AF.highByte & 0x80, SIGN);
	setFlag(cpu, parity(cpu->AF.highByte), PARITY);

	cpu->PC++;
	return 7;
}

// Halt the CPU TODO: IMPLEMENTION
uint8_t HLT(cpu_t *cpu)
{
	CPU_CRASH(cpu);
	cpu->PC++;
	return 7;
}

// Add the content of BC,DE,HL or SP to HL
uint8_t DAD(cpu_t *cpu)
{
	uint16_t word = 0;

	switch (cpu->opcode) {
	case 0x09:
		// DAD B
		word = cpu->BC.reg;
		break;
	case 0x19:
		// DAD D
		word = cpu->DE.reg;
		break;
	case 0x29:
		// DAD H
		word = cpu->HL.reg;
		break;
	case 0x39:
		// DAD SP
		word = cpu->SP;
		break;

	default:
		CPU_CRASH(cpu);
		break;
	}

	setFlag(cpu, ((cpu->HL.reg + word) > 0xFFFF), CARRY);
	cpu->HL.reg += word;
	cpu->PC++;
	return 10;
}

// Left shift Accumulator, Bit 7 will be transfered to Bit0 and Carry
uint8_t RLC(cpu_t *cpu)
{
	uint8_t bit7 = cpu->AF.highByte >> 7;

	cpu->AF.highByte = (cpu->AF.highByte << 1) | bit7;

	setFlag(cpu, bit7, CARRY);

	cpu->PC++;
	return 4;
}

// Right shift Accumulator, Bit 0 will be transfered to Bit7 and Carry
uint8_t RRC(cpu_t *cpu)
{
	uint8_t bit0 = cpu->AF.highByte & 1;

	cpu->AF.highByte = (cpu->AF.highByte >> 1) | (bit0 << 7);

	setFlag(cpu, bit0, CARRY);

	cpu->PC++;
	return 4;
}

// Left shift Accumulator, Bit 7 will be transfered to carry and original carry
// will be transfered to bit0
uint8_t RAL(cpu_t *cpu)
{
	uint8_t carry = cpu->AF.lowByte & 1;
	uint8_t bit7 = cpu->AF.highByte >> 7;

	cpu->AF.highByte = (cpu->AF.highByte << 1) | carry;

	setFlag(cpu, bit7, CARRY);

	cpu->PC++;
	return 4;
}

// Shift Accumultor to right, Bit0 will be transfered to carry, original carry
// will be transfered to bit7
uint8_t RAR(cpu_t *cpu)
{
	uint8_t carry = cpu->AF.lowByte & 1;
	uint8_t bit0 = cpu->AF.highByte & 1;

	cpu->AF.highByte = (cpu->AF.highByte >> 1) | (carry << 7);

	setFlag(cpu, bit0, CARRY);

	cpu->PC++;
	return 4;
}

// Sub content of Register from Accumulator
uint8_t SUB(cpu_t *cpu)
{
	uint8_t value = 0;

	switch (cpu->opcode) {
	case 0x90:
		// SUB B
		value = cpu->BC.highByte;
		break;
	case 0x91:
		// SUB C
		value = cpu->BC.lowByte;
		break;
	case 0x92:
		// SUB D
		value = cpu->DE.highByte;
		break;
	case 0x93:
		// SUB E
		value = cpu->DE.lowByte;
		break;
	case 0x94:
		// SUB H
		value = cpu->HL.highByte;
		break;
	case 0x95:
		// SUB L
		value = cpu->HL.lowByte;
		break;
	case 0x96:
		// SUB M
		value = readMemoryValue(cpu->HL.reg);
		break;
	case 0x97:
		// SUB A
		value = cpu->AF.highByte;
		break;

	default:
		CPU_CRASH(cpu);
		break;
	}

	setFlag(cpu, (cpu->AF.highByte < value), CARRY);
	setFlag(cpu,
			((cpu->AF.highByte & 0x8) != ((cpu->AF.highByte - value) & 0x8)),
			AUXCARRY);

	cpu->AF.highByte -= value;

	setFlag(cpu, cpu->AF.highByte == 0, ZERO);
	setFlag(cpu, cpu->AF.highByte & 0x80, SIGN);
	setFlag(cpu, parity(cpu->AF.highByte), PARITY);

	cpu->PC++;

	return (cpu->opcode == 0x96) ? 7 : 4;
}

// Sub content of Register and carry from Accumulator
uint8_t SBB(cpu_t *cpu)
{
	uint8_t carry = cpu->AF.lowByte & 1;
	uint8_t value = 0;

	switch (cpu->opcode) {
	case 0x98:
		// SBB B
		value = cpu->BC.highByte;
		break;
	case 0x99:
		// SBB C
		value = cpu->BC.lowByte;
		break;
	case 0x9A:
		// SBB D
		value = cpu->DE.highByte;
		break;
	case 0x9B:
		// SBB E
		value = cpu->DE.lowByte;
		break;
	case 0x9C:
		// SBB H
		value = cpu->HL.highByte;
		break;
	case 0x9D:
		// SBB L
		value = cpu->HL.lowByte;
		break;
	case 0x9E:
		// SBB M
		value = readMemoryValue(cpu->HL.reg);
		break;
	case 0x9F:
		// SBB A
		value = cpu->AF.highByte;
		break;

	default:
		CPU_CRASH(cpu);
		break;
	}

	setFlag(cpu, (cpu->AF.highByte < (value + carry)), CARRY);
	setFlag(cpu,
			((cpu->AF.highByte & 0x8) !=
			 ((cpu->AF.highByte - value - carry) & 0x8)),
			AUXCARRY);

	cpu->AF.highByte = cpu->AF.highByte - value - carry;

	setFlag(cpu, cpu->AF.highByte == 0, ZERO);
	setFlag(cpu, cpu->AF.highByte & 0x80, SIGN);
	setFlag(cpu, parity(cpu->AF.highByte), PARITY);

	cpu->PC++;

	return (cpu->opcode == 0x9E) ? 7 : 4;
}

// Subtract the next byte from Accumulator
uint8_t SUI(cpu_t *cpu)
{
	cpu->PC++;
	uint8_t val = readMemoryValue(cpu->PC);

	setFlag(cpu, (cpu->AF.highByte < val), CARRY);
	setFlag(cpu, ((cpu->AF.highByte & 0x8) != ((cpu->AF.highByte - val) & 0x8)),
			AUXCARRY);

	cpu->AF.highByte = cpu->AF.highByte - val;

	setFlag(cpu, cpu->AF.highByte == 0, ZERO);
	setFlag(cpu, cpu->AF.highByte & 0x80, SIGN);
	setFlag(cpu, parity(cpu->AF.highByte), PARITY);

	cpu->PC++;
	return 7;
}

// Subtract the next byte and carry from Accumulator
uint8_t SBI(cpu_t *cpu)
{
	cpu->PC++;
	uint8_t val = readMemoryValue(cpu->PC);
	uint8_t carry = cpu->AF.lowByte & 1;

	setFlag(cpu, (cpu->AF.highByte < (val + carry)), CARRY);
	setFlag(cpu,
			((cpu->AF.highByte & 0x8) !=
			 ((cpu->AF.highByte - val - carry) & 0x8)),
			AUXCARRY);

	cpu->AF.highByte = cpu->AF.highByte - val - carry;

	setFlag(cpu, cpu->AF.highByte == 0, ZERO);
	setFlag(cpu, cpu->AF.highByte & 0x80, SIGN);
	setFlag(cpu, parity(cpu->AF.highByte), PARITY);

	cpu->PC++;
	return 7;
}

// Compare Accumulator to content of register or memory address
uint8_t CMP(cpu_t *cpu)
{
	uint8_t value = 0;

	switch (cpu->opcode) {
	case 0xB8:
		// CMP B
		value = cpu->BC.highByte;
		break;
	case 0xB9:
		// CMP C
		value = cpu->BC.lowByte;
		break;
	case 0xBA:
		// CMP D
		value = cpu->DE.highByte;
		break;
	case 0xBB:
		// CMP E
		value = cpu->DE.lowByte;
		break;
	case 0xBC:
		// CMP H
		value = cpu->HL.highByte;
		break;
	case 0xBD:
		// CMP L
		value = cpu->HL.lowByte;
		break;
	case 0xBE:
		// CMP M
		value = readMemoryValue(cpu->HL.reg);
		break;
	case 0xBF:
		// CMP A
		value = cpu->AF.highByte;
		break;
	default:
		CPU_CRASH(cpu);
		break;
	}

	setFlag(cpu, (cpu->AF.highByte < value), CARRY);
	setFlag(cpu,
			((cpu->AF.highByte & 0x8) != ((cpu->AF.highByte - value) & 0x8)),
			AUXCARRY);

	uint8_t result = cpu->AF.highByte - value;

	setFlag(cpu, result == 0, ZERO);
	setFlag(cpu, result & 0x80, SIGN);
	setFlag(cpu, parity(result), PARITY);

	cpu->PC++;
	return (cpu->opcode == 0xBE) ? 7 : 4;
}

uint8_t CPI(cpu_t *cpu)
{
	cpu->PC++;
	uint8_t val = readMemoryValue(cpu->PC);

	setFlag(cpu, (cpu->AF.highByte < val), CARRY);
	setFlag(cpu, ((cpu->AF.highByte & 0x8) != ((cpu->AF.highByte - val) & 0x8)),
			AUXCARRY);

	uint8_t result = cpu->AF.highByte - val;

	setFlag(cpu, result == 0, ZERO);
	setFlag(cpu, result & 0x80, SIGN);
	setFlag(cpu, parity(result), PARITY);

	cpu->PC++;
	return 7;
}

// Call Subroutine, next 2 Bytes provide the address
uint8_t CALL(cpu_t *cpu)
{
	// CPUDIAG
	uint16_t foo = readMemoryValue(cpu->PC + 2) | readMemoryValue(cpu->PC + 1);
	if (foo == 5) {
		if (cpu->BC.lowByte == 9) {
			uint16_t offset = cpu->DE.reg;
			uint8_t byte = 0;
			while ((byte = readMemoryValue(offset++)) != '$') {
				printf("%c", byte);
			}
			printf("\n");
		}
	} else if (foo == 0) {
		exit(0);
	} else {
		//CPUDIAG

		cpu->PC++;
		uint8_t lowByte = readMemoryValue(cpu->PC);
		cpu->PC++;
		uint8_t highByte = readMemoryValue(cpu->PC);
		cpu->PC++; // TODO: Check this

		cpu->SP--;
		writeByteToMemory(cpu->PC >> 8, cpu->SP);
		cpu->SP--;
		writeByteToMemory(cpu->PC & 0xFF, cpu->SP);

		cpu->PC = highByte << 8 | lowByte;
	}
	return 17;
}

// TODO: Check the Condition logic!
// TODO: IMPORTANT
// Call Subroutine if Condition is true
uint8_t CALLC(cpu_t *cpu)
{
	uint8_t condition = 0;

	switch (cpu->opcode) {
	case 0xC4:
		// Not Zero
		condition = !(cpu->AF.lowByte & ZERO);
		break;
	case 0xCC:
		// Zero
		condition = cpu->AF.lowByte & ZERO;
		break;
	case 0xD4:
		// Not Carry set
		condition = !(cpu->AF.lowByte & CARRY);
		break;
	case 0xDC:
		// Carry set
		condition = cpu->AF.lowByte & CARRY;
		break;
	case 0xE4:
		condition = !(cpu->AF.lowByte & PARITY);
		break;
	case 0xEC:
		condition = cpu->AF.lowByte & PARITY;
		break;
	case 0xF4:
		condition = !(cpu->AF.lowByte & SIGN);
		break;
	case 0xFC:
		condition = cpu->AF.lowByte & SIGN;
		break;

	default:
		CPU_CRASH(cpu);
		break;
	}

	if (!condition) {
		cpu->PC += 3;
		return 11;
	}

	cpu->PC++;
	uint8_t lowByte = readMemoryValue(cpu->PC);
	cpu->PC++;
	uint8_t highByte = readMemoryValue(cpu->PC);

	cpu->SP--;
	writeByteToMemory(cpu->PC >> 8, cpu->SP);
	cpu->SP--;
	writeByteToMemory(cpu->PC & 0xFF, cpu->SP);

	cpu->PC = highByte << 8 | lowByte;
	return 17;
}

// Call Subroutine
uint8_t RST(cpu_t *cpu)
{
	cpu->SP--;
	writeByteToMemory(cpu->PC >> 8, cpu->SP);
	cpu->SP--;
	writeByteToMemory(cpu->PC & 0xFF, cpu->SP);

	cpu->PC = cpu->opcode - 0xC7;
	return 11;
}

// Return from Subroutine
uint8_t RET(cpu_t *cpu)
{
	uint8_t lowByte = readMemoryValue(cpu->SP);
	cpu->SP++;
	uint8_t highByte = readMemoryValue(cpu->SP);
	cpu->SP++;

	cpu->PC = highByte << 8 | lowByte;

	return 10;
}

/*
	D A A (Decimal Adjust Accumulator)
	The eight-bit number in the accumulator is adjusted
	to form two four-bit Binary-Coded-Decimal digits by
	the following process:
	1. If the value of the least significant 4 bits of the
	accumulator is greater than 9 or if the AC flag
	is set, 6 is added to the accumulator.
	2. If the value of the most significant 4 bits of the
	accumulator is now greater than 9, or if the C Y
	flag is set, 6 is added to the most significant 4
	bits of the accumulator.
*/
uint8_t DAA(cpu_t *cpu)
{
	if (cpu->AF.lowByte & AUXCARRY || (cpu->AF.highByte & 0x0F) > 9) {
		cpu->AF.highByte += 6;
	}

	if (cpu->AF.lowByte & CARRY || ((cpu->AF.highByte >> 4) > 9)) {
		cpu->AF.highByte += (6 << 4);
	}

	cpu->PC++;
	return 4;
}

// TODO: Check the Condition logic!
// Ret if Condition is true. Grouped Instructions
uint8_t RETC(cpu_t *cpu)
{
	uint8_t condition = 0;

	switch (cpu->opcode) {
	case 0xC0: // Not Zero
		condition = !(cpu->AF.lowByte & ZERO);
		break;
	case 0xC8: // Zero set
		condition = cpu->AF.lowByte & ZERO;
		break;
	case 0xD0: // Not Carry set
		condition = !(cpu->AF.lowByte & CARRY);
		break;
	case 0xD8: // Carry set
		condition = cpu->AF.lowByte & CARRY;
		break;
	case 0xE0: // Parity odd, P not set
		condition = !(cpu->AF.lowByte & PARITY);
		break;
	case 0xE8: // Parity even, P set
		condition = cpu->AF.lowByte & PARITY;
		break;
	case 0xF0: // on Plus, SIGN not set
		condition = !(cpu->AF.lowByte & SIGN);
		break;
	case 0xF8: // on Minus, SIGN  set
		condition = cpu->AF.lowByte & SIGN;
		break;

	default:
		CPU_CRASH(cpu);
		break;
	}

	if (!condition) {
		cpu->PC++;
		return 5;
	}

	uint8_t lowByte = readMemoryValue(cpu->SP);
	cpu->SP++;
	uint8_t highByte = readMemoryValue(cpu->SP);
	cpu->SP++;

	cpu->PC = highByte << 8 | lowByte;
	return 11;
}

// Pop(get back) register pair from stack
uint8_t POP(cpu_t *cpu)
{
	uint8_t lowByte = readMemoryValue(cpu->SP);
	cpu->SP++;
	uint8_t highByte = readMemoryValue(cpu->SP);
	cpu->SP++;

	switch (cpu->opcode) {
	case 0xC1:
		cpu->BC.reg = highByte << 8 | lowByte;
		break;
	case 0xD1:
		cpu->DE.reg = highByte << 8 | lowByte;
		break;
	case 0xE1:
		cpu->HL.reg = highByte << 8 | lowByte;
		break;
	case 0xF1:
		cpu->AF.reg = highByte << 8 | lowByte;
		break;

	default:
		CPU_CRASH(cpu);
		break;
	}
	cpu->PC++;
	return 10;
}

// Push the contents of BC,DE,HL or AF to stack
uint8_t PUSH(cpu_t *cpu)
{
	reg_t reg = { 0 };

	switch (cpu->opcode) {
	case 0xC5:
		reg = cpu->BC;
		break;
	case 0xD5:
		reg = cpu->DE;
		break;
	case 0xE5:
		reg = cpu->HL;
		break;
	case 0xF5:
		reg = cpu->AF;
		break;

	default:
		CPU_CRASH(cpu);
		break;
	}

	cpu->SP--;
	writeByteToMemory(reg.highByte, cpu->SP);
	cpu->SP--;
	writeByteToMemory(reg.lowByte, cpu->SP);

	cpu->PC++;
	return 11;
}

// Jump on Condition
// TODO: NEEDS CHECKING
uint8_t JMPC(cpu_t *cpu)
{
	uint8_t condition = 0;

	switch (cpu->opcode) {
	case 0xC2: // Not Zero
		condition = !(cpu->AF.lowByte & ZERO);
		break;
	case 0xCA: // Zero set
		condition = cpu->AF.lowByte & ZERO;
		break;
	case 0xD2: // Not Carry
		condition = !(cpu->AF.lowByte & CARRY);
		break;
	case 0xDA: // Carry set
		condition = cpu->AF.lowByte & CARRY;
		break;
	case 0xE2: // Parity odd, P not set
		condition = !(cpu->AF.lowByte & PARITY);
		break;
	case 0xEA: // Parity even, P set
		condition = cpu->AF.lowByte & PARITY;
		break;
	case 0xF2: // on Plus
		condition = !(cpu->AF.lowByte & SIGN);
		break;
	case 0xFA: // on Minus
		condition = cpu->AF.lowByte & SIGN;
		break;

	default:
		CPU_CRASH(cpu);
		break;
	}

	if (!condition) {
		cpu->PC += 3;
		return 3;
	}

	cpu->PC++;
	uint8_t lowByte = readMemoryValue(cpu->PC);
	cpu->PC++;
	uint8_t highByte = readMemoryValue(cpu->PC);

	cpu->PC = highByte << 8 | lowByte;

	return 10;
}

// Jump to address
uint8_t JMP(cpu_t *cpu)
{
	cpu->PC++;
	uint8_t lowByte = readMemoryValue(cpu->PC);
	cpu->PC++;
	uint8_t highByte = readMemoryValue(cpu->PC);

	cpu->PC = highByte << 8 | lowByte;
	return 10;
}

// TODO: Check where Accumulator would be written to
uint8_t OUT(cpu_t *cpu)
{
	cpu->PC++;
	uint8_t port = readMemoryValue(cpu->PC);
	printf("OUT: Port: %02x at PC: %04x \n", port, cpu->PC);

	switch (port) {
	case 2:
		printf("Shift Offset!\n");
		setShiftOffset(cpu->AF.highByte);
		//exit(1);
		break;
	case 3:
		printf("OUT (File: %s, Line: %d): Sound not implemented yet\n",
			   __FILE__, __LINE__);
		// cpu->io_port[3] = cpu->AF.highByte;
		break;
	case 4:
		setShiftRegister(cpu->AF.highByte);
		printf("Shift Register!\n");
		//exit(1);
		break;
	case 5:
		printf("OUT (File: %s, Line: %d): Sound not implemented yet\n",
			   __FILE__, __LINE__);
		// cpu->io_port[5] = cpu->AF.highByte;
		break;
	case 6: // Watchdog
		// cpu->io_port[6] = cpu->AF.highByte;
		break;

	default:
		fprintf(stderr, "Unkown/Invalid port: %d (OUT)\n", port);
		CPU_CRASH(cpu);
		break;
	}

	cpu->PC++;
	// CPU_CRASH(cpu); TODO: Needs implementation
	return 10;
}

// TODO: Check where the CPU would read from
uint8_t IN(cpu_t *cpu)
{
	cpu->PC++;
	uint8_t port = readMemoryValue(cpu->PC);

	printf("IN: Port: %02x at PC: %04x \n", port, cpu->PC);

	if (port != 3) {
		cpu->AF.highByte = cpu->io_port[port];
	} else {
		cpu->AF.highByte = getShiftRegister();
		printf("IN (Shift): %x\n", cpu->AF.highByte);
	}

	// if (port == 2) {
	// 	printf("IN 2: %x\n", cpu->io_port[2]);
	// 	exit(1);
	// }

	cpu->PC++;

	return 10;
}

// Exchange the Low- and High Byte of the memory address stored in SP with HL
uint8_t XTHL(cpu_t *cpu)
{
	uint8_t lowByte = readMemoryValue(cpu->SP);
	uint8_t highByte = readMemoryValue(cpu->SP + 1);

	writeByteToMemory(cpu->HL.lowByte, cpu->SP);
	writeByteToMemory(cpu->HL.highByte, cpu->SP + 1);

	cpu->HL.lowByte = lowByte;
	cpu->HL.highByte = highByte;

	cpu->PC++;
	return 18;
}

// Disable Interrupts TODO: Implementation
uint8_t DI(cpu_t *cpu)
{
	printf("Disabling Interrupts. PC: %04x\n", cpu->PC);
	cpu->interrupt_enabled = 0;
	cpu->PC++;
	return 4;
}

// Enable Interrupts TODO: Implementation
uint8_t EI(cpu_t *cpu)
{
	printf("Enabling Interrupts. PC: %04x\n", cpu->PC);
	cpu->interrupt_enabled = 1;
	cpu->PC++;
	return 4;
}

// Load HL into PC
uint8_t PCHL(cpu_t *cpu)
{
	cpu->PC = cpu->HL.reg;
	return 5;
}

// Load HL into SP
uint8_t SPHL(cpu_t *cpu)
{
	cpu->SP = cpu->HL.reg;
	cpu->PC++;
	return 5;
}

// Exchange HL with DE
uint8_t XCHG(cpu_t *cpu)
{
	uint16_t temp = cpu->HL.reg;
	cpu->HL.reg = cpu->DE.reg;
	cpu->DE.reg = temp;

	cpu->PC++;
	return 5;
}

// Fetch next instruction, execute it and return the cycles it took
uint8_t step(cpu_t *cpu)
{
	// jumptable to the different instructions
	static uint8_t (*jumptable[256])(cpu_t *cpu) = {
		NOP,  LXI,	STA,  INX,	INR,   DCR,	 MVI, RLC,
		NOP,  DAD,	LDA,  DCX,	INR,   DCR,	 MVI, RRC, // 0x00 - 0x0F
		NOP,  LXI,	STA,  INX,	INR,   DCR,	 MVI, RAL,
		NOP,  DAD,	LDA,  DCX,	INR,   DCR,	 MVI, RAR, // 0x10 - 0x1F
		NOP,  LXI,	SHLD, INX,	INR,   DCR,	 MVI, DAA,
		NOP,  DAD,	LHLD, DCX,	INR,   DCR,	 MVI, CMA, // 0x20 - 0x2F
		NOP,  LXI,	STA,  INX,	INR,   DCR,	 MVI, STC,
		NOP,  DAD,	LDA,  DCX,	INR,   DCR,	 MVI, CMC, // 0x30 - 0x3F
		MOV,  MOV,	MOV,  MOV,	MOV,   MOV,	 MOV, MOV,
		MOV,  MOV,	MOV,  MOV,	MOV,   MOV,	 MOV, MOV, // 0x40 - 0x4F
		MOV,  MOV,	MOV,  MOV,	MOV,   MOV,	 MOV, MOV,
		MOV,  MOV,	MOV,  MOV,	MOV,   MOV,	 MOV, MOV, // 0x50 - 0x5F
		MOV,  MOV,	MOV,  MOV,	MOV,   MOV,	 MOV, MOV,
		MOV,  MOV,	MOV,  MOV,	MOV,   MOV,	 MOV, MOV, // 0x60 - 0x6F
		MOV,  MOV,	MOV,  MOV,	MOV,   MOV,	 HLT, MOV,
		MOV,  MOV,	MOV,  MOV,	MOV,   MOV,	 MOV, MOV, // 0x70 - 0x7F
		ADD,  ADD,	ADD,  ADD,	ADD,   ADD,	 ADD, ADD,
		ADC,  ADC,	ADC,  ADC,	ADC,   ADC,	 ADC, ADC, // 0x80 - 0x8F
		SUB,  SUB,	SUB,  SUB,	SUB,   SUB,	 SUB, SUB,
		SBB,  SBB,	SBB,  SBB,	SBB,   SBB,	 SBB, SBB, // 0x90 - 0x9F
		ANA,  ANA,	ANA,  ANA,	ANA,   ANA,	 ANA, ANA,
		XRA,  XRA,	XRA,  XRA,	XRA,   XRA,	 XRA, XRA, // 0xA0 - 0xAF
		ORA,  ORA,	ORA,  ORA,	ORA,   ORA,	 ORA, ORA,
		CMP,  CMP,	CMP,  CMP,	CMP,   CMP,	 CMP, CMP, // 0xB0 - 0xBF
		RETC, POP,	JMPC, JMP,	CALLC, PUSH, ADI, RST,
		RETC, RET,	JMPC, JMP,	CALLC, CALL, ACI, RST, // 0xC0 - 0xCF
		RETC, POP,	JMPC, OUT,	CALLC, PUSH, SUI, RST,
		RETC, RET,	JMPC, IN,	CALLC, CALL, SBI, RST, // 0xD0 - 0xDF
		RETC, POP,	JMPC, XTHL, CALLC, PUSH, ANI, RST,
		RETC, PCHL, JMPC, XCHG, CALLC, CALL, XRI, RST, // 0xE0 - 0xEF
		RETC, POP,	JMPC, DI,	CALLC, PUSH, ORI, RST,
		RETC, SPHL, JMPC, EI,	CALLC, CALL, CPI, RST, // 0xF0 - 0xFF
	};

	// Fetching next opcode
	// If Interrupt occured and Interrupts are enabled, jump to the
	// subroutine
	if (cpu->interrupt_enabled && cpu->interrupt) {
		cpu->opcode = cpu->interrupt;
		printf("Interrupt Routine: %02x\n", cpu->interrupt);
		cpu->interrupt = 0;
	} else {
		cpu->opcode = readMemoryValue(cpu->PC);
	}

	printf("Executing: %02x PC: %04x\n", cpu->opcode, cpu->PC);
	// fprintf(stdout, "A: %02x\tF: %02x\n", cpu->AF.highByte, cpu->AF.lowByte);
	// fprintf(stdout, "B: %02x\tC: %02x\n", cpu->BC.highByte, cpu->BC.lowByte);
	// fprintf(stdout, "D: %02x\tE: %02x\n", cpu->DE.highByte, cpu->DE.lowByte);
	// fprintf(stdout, "H: %02x\tL: %02x\n", cpu->HL.highByte, cpu->HL.lowByte);
	// fprintf(stdout, "PC: %04x\tSP: %04x\n", cpu->PC, cpu->SP);
	// fprintf(stdout, "Coins: %x (Addr: %x)\n", readMemoryValue(0x20EB), 0x20EB);
	// Executing Instruction and returning cycles
	return (*jumptable[cpu->opcode])(cpu);
}

void setInterruptRoutine(cpu_t *cpu, uint8_t interrupt)
{
	if (cpu->interrupt_enabled) {
		cpu->interrupt = interrupt;
	}
}
