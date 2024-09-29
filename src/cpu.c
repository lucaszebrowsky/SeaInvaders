#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"
#include "bus.h"
#include "shift_register.h"
#include "cpu_utils.h"
#include "flags.h"

#define CPU_CRASH(cpu) cpu_crash(cpu, __FILE__, __LINE__)

void initCPU(cpu_t *cpu)
{
	cpu->AF.reg = 0x2; // Bit 1 of Flag-Register is always set to 1
	cpu->BC.reg = 0;
	cpu->DE.reg = 0;
	cpu->HL.reg = 0;

	cpu->SP = 0;
	cpu->PC = 0;

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

// No Operation
uint8_t NOP(cpu_t *cpu)
{
	cpu->PC++;
	return 4;
}

// Increase Register or Memory by 1, flags affected
uint8_t INR(cpu_t *cpu)
{
	uint8_t *reg = get_reg8(cpu, (cpu->opcode >> 3) & 0x7);

	handle_halfcarry8(cpu, *reg, 1, 0);

	(*reg)++;

	handle_sign(cpu, *reg);
	handle_zero(cpu, *reg);
	handle_parity(cpu, *reg);

	cpu->PC++;
	return (cpu->opcode == 0x34 ? 10 : 5);
}

// Increase Register by 1, flags NOT affected
uint8_t INX(cpu_t *cpu)
{
	uint16_t *reg = get_reg16(cpu, cpu->opcode >> 4);

	(*reg)++;

	cpu->PC++;
	return 5;
}

// Decrease Register by 1
uint8_t DCR(cpu_t *cpu)
{
	uint8_t *reg = get_reg8(cpu, (cpu->opcode >> 3) & 0x7);

	// uint8_t aux = (((*result) & 0x0F) - 1) < 0x0F;
	// setFlag(cpu, aux, AUXCARRY);

	handle_halfcarry8(cpu, *reg, 1, 1);

	(*reg)--;

	handle_sign(cpu, *reg);
	handle_zero(cpu, *reg);
	handle_parity(cpu, *reg);

	cpu->PC++;
	return (cpu->opcode == 0x35 ? 10 : 5);
}

// Decrease Register by 1, flags NOT affected
uint8_t DCX(cpu_t *cpu)
{
	uint16_t *reg = get_reg16(cpu, cpu->opcode >> 4);

	(*reg)--;

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

// Store Accumulator at the given address
uint8_t STA(cpu_t *cpu)
{
	uint8_t lowByte = readMemoryValue(cpu->PC + 1);
	uint8_t highByte = readMemoryValue(cpu->PC + 2);
	uint16_t address = highByte << 8 | lowByte;

	writeByteToMemory(cpu->AF.highByte, address);

	cpu->PC += 3;
	return 13;
}

// Store Accumulator at the address stored in BC or DE
uint8_t STAX(cpu_t *cpu)
{
	uint16_t *reg = get_reg16(cpu, cpu->opcode >> 4);

	writeByteToMemory(cpu->AF.highByte, *reg);

	cpu->PC++;
	return 7;
}

// Load next 2 Bytes into a Register Pair
uint8_t LXI(cpu_t *cpu)
{
	uint8_t lowByte = readMemoryValue(cpu->PC + 1);
	uint8_t highByte = readMemoryValue(cpu->PC + 2);
	uint16_t *reg = get_reg16(cpu, cpu->opcode >> 4);

	*reg = highByte << 8 | lowByte;

	cpu->PC += 3;
	return 10;
}

// Move next byte into Register or memory location
uint8_t MVI(cpu_t *cpu)
{
	uint8_t byte = readMemoryValue(cpu->PC + 1);
	uint8_t *reg = get_reg8(cpu, (cpu->opcode >> 3) & 0x7);

	*reg = byte;

	cpu->PC += 2;
	return (cpu->opcode == 0x36 ? 10 : 7);
}

// Load byte at the given address into Accumulator
uint8_t LDA(cpu_t *cpu)
{
	uint8_t lowByte = readMemoryValue(cpu->PC + 1);
	uint8_t highByte = readMemoryValue(cpu->PC + 2);
	uint16_t address = highByte << 8 | lowByte;

	cpu->AF.highByte = readMemoryValue(address);

	cpu->PC += 3;
	return 13;
}

uint8_t LDAX(cpu_t *cpu)
{
	uint16_t *reg = get_reg16(cpu, cpu->opcode >> 4);

	cpu->AF.highByte = readMemoryValue(*reg);

	cpu->PC++;
	return 7;
}

// Load next 2 Bytes into HL
uint8_t LHLD(cpu_t *cpu)
{
	uint8_t lowByte = readMemoryValue(cpu->PC + 1);
	uint8_t highByte = readMemoryValue(cpu->PC + 2);
	uint16_t address = highByte << 8 | lowByte;

	cpu->HL.lowByte = readMemoryValue(address);
	cpu->HL.highByte = readMemoryValue(address + 1);

	cpu->PC += 3;
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
	uint8_t lowByte = readMemoryValue(cpu->PC + 1);
	uint8_t highByte = readMemoryValue(cpu->PC + 2);
	uint16_t address = highByte << 8 | lowByte;

	writeByteToMemory(cpu->HL.lowByte, address);
	writeByteToMemory(cpu->HL.highByte, address + 1);

	cpu->PC += 3;
	return 16;
}

// Move register or memory content into another register or memory address
uint8_t MOV(cpu_t *cpu)
{
	uint8_t *dst = get_reg8(cpu, (cpu->opcode >> 3) & 0x7);
	uint8_t *src = get_reg8(cpu, cpu->opcode & 0x7);

	*dst = *src;

	uint8_t cycles_lookup[64] = {
		5, 5, 5, 5, 5, 5, 7, 5, 5, 5, 5, 5, 5, 5, 7, 5, // 0x40 - 0x4F
		5, 5, 5, 5, 5, 5, 7, 5, 5, 5, 5, 5, 5, 5, 7, 5, // 0x50 - 0x5f
		5, 5, 5, 5, 5, 5, 7, 5, 5, 5, 5, 5, 5, 5, 7, 5, // 0x60 - 0x6f
		7, 7, 7, 7, 7, 7, 0, 7, 5, 5, 5, 5, 5, 5, 7, 5, // 0x70 - 0x7f
	};

	cpu->PC++;
	return cycles_lookup[cpu->opcode - 0x40];
}

// Add content of Register to Accumulator
uint8_t ADD(cpu_t *cpu)
{
	uint8_t *reg = get_reg8(cpu, cpu->opcode & 0x7);

	handle_carry8(cpu, cpu->AF.highByte, *reg, 0);
	handle_halfcarry8(cpu, cpu->AF.highByte, *reg, 0);

	cpu->AF.highByte += *reg;

	handle_zero(cpu, cpu->AF.highByte);
	handle_sign(cpu, cpu->AF.highByte);
	handle_parity(cpu, cpu->AF.highByte);

	cpu->PC++;
	return (cpu->opcode == 0x86) ? 7 : 4;
}

// Add next Byte to Accumulator
uint8_t ADI(cpu_t *cpu)
{
	uint8_t byte = readMemoryValue(cpu->PC + 1);

	handle_carry8(cpu, cpu->AF.highByte, byte, 0);
	handle_halfcarry8(cpu, cpu->AF.highByte, byte, 0);

	cpu->AF.highByte += byte;

	handle_zero(cpu, cpu->AF.highByte);
	handle_sign(cpu, cpu->AF.highByte);
	handle_parity(cpu, cpu->AF.highByte);

	cpu->PC += 2;
	return 7;
}

// Add next byte and carry to Accumulator
uint8_t ACI(cpu_t *cpu)
{
	uint8_t byte = readMemoryValue(cpu->PC + 1);
	uint8_t carry = cpu->AF.lowByte & 1;

	handle_carry8(cpu, cpu->AF.highByte, byte + carry, 0);
	handle_halfcarry8(cpu, cpu->AF.highByte, byte + carry, 0);

	cpu->AF.highByte = cpu->AF.highByte + byte + carry;

	handle_zero(cpu, cpu->AF.highByte);
	handle_sign(cpu, cpu->AF.highByte);
	handle_parity(cpu, cpu->AF.highByte);

	cpu->PC += 2;
	return 7;
}

// Add content of Register and carry-bit to Accumulator
uint8_t ADC(cpu_t *cpu)
{
	uint8_t carry = cpu->AF.lowByte & 1;
	uint8_t *reg = get_reg8(cpu, cpu->opcode & 0x7);

	handle_carry8(cpu, cpu->AF.highByte, carry + (*reg), 0);
	handle_halfcarry8(cpu, cpu->AF.highByte, carry + (*reg), 0);

	cpu->AF.highByte = cpu->AF.highByte + carry + (*reg);

	handle_zero(cpu, cpu->AF.highByte);
	handle_sign(cpu, cpu->AF.highByte);
	handle_parity(cpu, cpu->AF.highByte);

	cpu->PC++;
	return (cpu->opcode == 0x8E) ? 7 : 4;
}

// Bitwise AND Accumulator with content of register or memory
uint8_t ANA(cpu_t *cpu)
{
	uint8_t *reg = get_reg8(cpu, cpu->opcode & 0x7);

	handle_halfcarry8(cpu, cpu->AF.highByte, *reg, 0);

	cpu->AF.highByte &= *reg;

	clear_flag(cpu, CARRY);
	handle_zero(cpu, cpu->AF.highByte);
	handle_sign(cpu, cpu->AF.highByte);
	handle_parity(cpu, cpu->AF.highByte);

	cpu->PC++;
	return (cpu->opcode == 0xA6) ? 7 : 4;
}

// Bitwise And Accumulator with next Byte
uint8_t ANI(cpu_t *cpu)
{
	uint8_t byte = readMemoryValue(cpu->PC + 1);

	cpu->AF.highByte &= byte;

	clear_flag(cpu, CARRY);
	clear_flag(cpu, AUXCARRY);

	handle_zero(cpu, cpu->AF.highByte);
	handle_parity(cpu, cpu->AF.highByte);
	handle_sign(cpu, cpu->AF.highByte);

	cpu->PC += 2;
	return 7;
}

// Bitwise XOR Accumulator with content of register or memory
uint8_t XRA(cpu_t *cpu)
{
	uint8_t *reg = get_reg8(cpu, cpu->opcode & 0x7);

	cpu->AF.highByte ^= *reg;

	clear_flag(cpu, CARRY);
	clear_flag(cpu, AUXCARRY);

	handle_zero(cpu, cpu->AF.highByte);
	handle_parity(cpu, cpu->AF.highByte);
	handle_sign(cpu, cpu->AF.highByte);

	cpu->PC++;
	return (cpu->opcode == 0xAE) ? 7 : 4;
}

// Bitwise XOR Accumulator with next byte
uint8_t XRI(cpu_t *cpu)
{
	uint8_t byte = readMemoryValue(cpu->PC + 1);

	cpu->AF.highByte ^= byte;

	clear_flag(cpu, CARRY);
	clear_flag(cpu, AUXCARRY);

	handle_zero(cpu, cpu->AF.highByte);
	handle_parity(cpu, cpu->AF.highByte);
	handle_sign(cpu, cpu->AF.highByte);

	cpu->PC += 2;
	return 7;
}

// Bitwise OR Accumulator with content of register or memory
uint8_t ORA(cpu_t *cpu)
{
	uint8_t *reg = get_reg8(cpu, cpu->opcode & 0x7);

	cpu->AF.highByte |= *reg;

	clear_flag(cpu, CARRY);
	clear_flag(cpu, AUXCARRY);

	handle_zero(cpu, cpu->AF.highByte);
	handle_parity(cpu, cpu->AF.highByte);
	handle_sign(cpu, cpu->AF.highByte);

	cpu->PC++;
	return (cpu->opcode == 0xB6) ? 7 : 4;
}

// Bitwise OR Accumulator with next Byte
uint8_t ORI(cpu_t *cpu)
{
	uint8_t byte = readMemoryValue(cpu->PC + 1);

	cpu->AF.highByte |= byte;

	clear_flag(cpu, CARRY);
	clear_flag(cpu, AUXCARRY);

	handle_zero(cpu, cpu->AF.highByte);
	handle_parity(cpu, cpu->AF.highByte);
	handle_sign(cpu, cpu->AF.highByte);

	cpu->PC += 2;
	return 7;
}

// Halt the CPU TODO: IMPLEMENTION
uint8_t HLT(cpu_t *cpu)
{
	fprintf(stderr, "HLT instruction not implemented!\n");
	CPU_CRASH(cpu);
	cpu->PC++;
	return 7;
}

// Add the content of BC,DE,HL or SP to HL
uint8_t DAD(cpu_t *cpu)
{
	uint16_t *reg = get_reg16(cpu, cpu->opcode >> 4);

	handle_carry16(cpu, cpu->HL.reg, *reg, 0);

	cpu->HL.reg += *reg;

	cpu->PC++;
	return 10;
}

// Left shift Accumulator, Bit 7 will be transfered to Bit0 and Carry
uint8_t RLC(cpu_t *cpu)
{
	uint8_t bit7 = cpu->AF.highByte >> 7;

	cpu->AF.highByte = (cpu->AF.highByte << 1) | bit7;

	bit7 == 1 ? set_flag(cpu, CARRY) : clear_flag(cpu, CARRY);

	cpu->PC++;
	return 4;
}

// Right shift Accumulator, Bit 0 will be transfered to Bit7 and Carry
uint8_t RRC(cpu_t *cpu)
{
	uint8_t bit0 = cpu->AF.highByte & 1;

	cpu->AF.highByte = (cpu->AF.highByte >> 1) | (bit0 << 7);

	bit0 == 1 ? set_flag(cpu, CARRY) : clear_flag(cpu, CARRY);

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

	bit7 == 1 ? set_flag(cpu, CARRY) : clear_flag(cpu, CARRY);

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

	bit0 == 1 ? set_flag(cpu, CARRY) : clear_flag(cpu, CARRY);

	cpu->PC++;
	return 4;
}

// Sub content of Register from Accumulator
uint8_t SUB(cpu_t *cpu)
{
	uint8_t *reg = get_reg8(cpu, cpu->opcode & 0x7);

	handle_carry8(cpu, cpu->AF.highByte, *reg, 1);
	handle_halfcarry8(cpu, cpu->AF.highByte, *reg, 1);

	cpu->AF.highByte -= *reg;

	handle_zero(cpu, cpu->AF.highByte);
	handle_sign(cpu, cpu->AF.highByte);
	handle_parity(cpu, cpu->AF.highByte);

	cpu->PC++;
	return (cpu->opcode == 0x96) ? 7 : 4;
}

// Sub content of Register and carry from Accumulator
uint8_t SBB(cpu_t *cpu)
{
	uint8_t carry = cpu->AF.lowByte & 1;
	uint8_t *reg = get_reg8(cpu, cpu->opcode & 0x7);

	handle_carry8(cpu, cpu->AF.highByte, carry + *reg, 1);
	handle_halfcarry8(cpu, cpu->AF.highByte, carry + *reg, 1);

	cpu->AF.highByte = cpu->AF.highByte - carry - *reg;

	handle_zero(cpu, cpu->AF.highByte);
	handle_sign(cpu, cpu->AF.highByte);
	handle_parity(cpu, cpu->AF.highByte);

	cpu->PC++;
	return (cpu->opcode == 0x9E) ? 7 : 4;
}

// Subtract the next byte from Accumulator
uint8_t SUI(cpu_t *cpu)
{
	uint8_t byte = readMemoryValue(cpu->PC + 1);

	handle_carry8(cpu, cpu->AF.highByte, byte, 1);
	handle_halfcarry8(cpu, cpu->AF.highByte, byte, 1);

	cpu->AF.highByte = cpu->AF.highByte - byte;

	handle_zero(cpu, cpu->AF.highByte);
	handle_sign(cpu, cpu->AF.highByte);
	handle_parity(cpu, cpu->AF.highByte);

	cpu->PC += 2;
	return 7;
}

// Subtract the next byte and carry from Accumulator
uint8_t SBI(cpu_t *cpu)
{
	uint8_t byte = readMemoryValue(cpu->PC + 1);
	uint8_t carry = cpu->AF.lowByte & 1;

	handle_carry8(cpu, cpu->AF.highByte, byte + carry, 1);
	handle_halfcarry8(cpu, cpu->AF.highByte, byte + carry, 1);

	cpu->AF.highByte = cpu->AF.highByte - byte - carry;

	handle_zero(cpu, cpu->AF.highByte);
	handle_sign(cpu, cpu->AF.highByte);
	handle_parity(cpu, cpu->AF.highByte);

	cpu->PC += 2;
	return 7;
}

// Compare Accumulator to content of register or memory address
uint8_t CMP(cpu_t *cpu)
{
	uint8_t *reg = get_reg8(cpu, cpu->opcode & 0x7);

	handle_carry8(cpu, cpu->AF.highByte, *reg, 1);
	handle_halfcarry8(cpu, cpu->AF.highByte, *reg, 1);

	uint8_t result = cpu->AF.highByte - *reg;

	handle_zero(cpu, result);
	handle_sign(cpu, result);
	handle_parity(cpu, result);

	cpu->PC++;
	return (cpu->opcode == 0xBE) ? 7 : 4;
}

uint8_t CPI(cpu_t *cpu)
{
	uint8_t byte = readMemoryValue(cpu->PC + 1);

	handle_carry8(cpu, cpu->AF.highByte, byte, 1);
	handle_halfcarry8(cpu, cpu->AF.highByte, byte, 1);

	uint8_t result = cpu->AF.highByte - byte;

	handle_zero(cpu, result);
	handle_sign(cpu, result);
	handle_parity(cpu, result);

	cpu->PC += 2;
	return 7;
}

// Call Subroutine, next 2 Bytes provide the address
uint8_t CALL(cpu_t *cpu)
{
	uint8_t lowByte = readMemoryValue(cpu->PC + 1);
	uint8_t highByte = readMemoryValue(cpu->PC + 2);

	cpu->PC += 3;

	writeByteToMemory(cpu->PC >> 8, cpu->SP - 1);
	writeByteToMemory(cpu->PC & 0xFF, cpu->SP - 2);

	cpu->SP -= 2;

	cpu->PC = highByte << 8 | lowByte;
	return 17;
}

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
	cpu->PC++;

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
	writeByteToMemory(cpu->PC >> 8, cpu->SP - 1);
	writeByteToMemory(cpu->PC & 0xFF, cpu->SP - 2);

	cpu->SP -= 2;

	cpu->PC = cpu->opcode - 0xC7;
	return 11;
}

// Return from Subroutine
uint8_t RET(cpu_t *cpu)
{
	uint8_t lowByte = readMemoryValue(cpu->SP);
	uint8_t highByte = readMemoryValue(cpu->SP + 1);

	cpu->SP += 2;

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
		handle_halfcarry8(cpu, cpu->AF.highByte, 6, 0);
		cpu->AF.highByte += 6;
	}

	if (cpu->AF.lowByte & CARRY || ((cpu->AF.highByte >> 4) > 9)) {
		handle_carry8(cpu, cpu->AF.highByte, 0x60, 0);

		cpu->AF.highByte += (6 << 4);

		handle_zero(cpu, cpu->AF.highByte);
		handle_parity(cpu, cpu->AF.highByte);
		handle_sign(cpu, cpu->AF.highByte);
	}

	cpu->PC++;
	return 4;
}

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
	uint8_t highByte = readMemoryValue(cpu->SP + 1);

	cpu->SP += 2;

	uint16_t *reg = get_reg16(cpu, (cpu->opcode >> 4) & 0x3);

	if (cpu->opcode == 0xF1) {
		reg = &cpu->AF.reg;
	}

	*reg = highByte << 8 | lowByte;

	cpu->PC++;
	return 10;
}

// Push the contents of BC,DE,HL or AF to stack
uint8_t PUSH(cpu_t *cpu)
{
	uint16_t *reg = get_reg16(cpu, (cpu->opcode >> 4) & 0x3);

	if (cpu->opcode == 0xF5) {
		reg = &cpu->AF.reg;
	}

	writeByteToMemory((*reg) >> 8, cpu->SP - 1);
	writeByteToMemory((*reg) & 0xFF, cpu->SP - 2);

	cpu->SP -= 2;

	cpu->PC++;
	return 11;
}

// Jump on Condition
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
	uint8_t lowByte = readMemoryValue(cpu->PC + 1);
	uint8_t highByte = readMemoryValue(cpu->PC + 2);

	cpu->PC = highByte << 8 | lowByte;
	return 10;
}

uint8_t OUT(cpu_t *cpu)
{
	cpu->PC++;
	uint8_t port = readMemoryValue(cpu->PC);

	switch (port) {
	case 2:
		setShiftOffset(cpu->AF.highByte);
		break;
	case 3:
		// printf("OUT (File: %s, Line: %d): Sound not implemented yet\n",
		// 	   __FILE__, __LINE__);
		// cpu->io_port[3] = cpu->AF.highByte;
		break;
	case 4:
		setShiftRegister(cpu->AF.highByte);
		// printf("Shift Register!\n");
		//exit(1);
		break;
	case 5:
		// printf("OUT (File: %s, Line: %d): Sound not implemented yet\n",
		// 	   __FILE__, __LINE__);
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
	return 10;
}

uint8_t IN(cpu_t *cpu)
{
	cpu->PC++;
	uint8_t port = readMemoryValue(cpu->PC);

	if (port != 3) {
		cpu->AF.highByte = cpu->io_port[port];
	} else {
		cpu->AF.highByte = getShiftRegister();
	}

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

// Disable Interrupts
uint8_t DI(cpu_t *cpu)
{
	cpu->interrupt_enabled = 0;
	cpu->PC++;
	return 4;
}

// Enable Interrupts
uint8_t EI(cpu_t *cpu)
{
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
		NOP,  LXI,	STAX, INX,	INR,   DCR,	 MVI, RLC, // 0x00 - 0x07
		NOP,  DAD,	LDAX, DCX,	INR,   DCR,	 MVI, RRC, // 0x08 - 0x0F
		NOP,  LXI,	STAX, INX,	INR,   DCR,	 MVI, RAL, // 0x10 - 0x17
		NOP,  DAD,	LDAX, DCX,	INR,   DCR,	 MVI, RAR, // 0x18 - 0x1F
		NOP,  LXI,	SHLD, INX,	INR,   DCR,	 MVI, DAA, // 0x20 - 0x27
		NOP,  DAD,	LHLD, DCX,	INR,   DCR,	 MVI, CMA, // 0x28 - 0x2F
		NOP,  LXI,	STA,  INX,	INR,   DCR,	 MVI, STC, // 0x30 - 0x37
		NOP,  DAD,	LDA,  DCX,	INR,   DCR,	 MVI, CMC, // 0x38 - 0x3F
		MOV,  MOV,	MOV,  MOV,	MOV,   MOV,	 MOV, MOV, // 0x40 - 0x47
		MOV,  MOV,	MOV,  MOV,	MOV,   MOV,	 MOV, MOV, // 0x48 - 0x4F
		MOV,  MOV,	MOV,  MOV,	MOV,   MOV,	 MOV, MOV, // 0x50 - 0x57
		MOV,  MOV,	MOV,  MOV,	MOV,   MOV,	 MOV, MOV, // 0x58 - 0x5F
		MOV,  MOV,	MOV,  MOV,	MOV,   MOV,	 MOV, MOV, // 0x60 - 0x67
		MOV,  MOV,	MOV,  MOV,	MOV,   MOV,	 MOV, MOV, // 0x68 - 0x6F
		MOV,  MOV,	MOV,  MOV,	MOV,   MOV,	 HLT, MOV, // 0x70 - 0x77
		MOV,  MOV,	MOV,  MOV,	MOV,   MOV,	 MOV, MOV, // 0x78 - 0x7F
		ADD,  ADD,	ADD,  ADD,	ADD,   ADD,	 ADD, ADD, // 0x80 - 0x87
		ADC,  ADC,	ADC,  ADC,	ADC,   ADC,	 ADC, ADC, // 0x88 - 0x8F
		SUB,  SUB,	SUB,  SUB,	SUB,   SUB,	 SUB, SUB, // 0x90 - 0x97
		SBB,  SBB,	SBB,  SBB,	SBB,   SBB,	 SBB, SBB, // 0x98 - 0x9F
		ANA,  ANA,	ANA,  ANA,	ANA,   ANA,	 ANA, ANA, // 0xA0 - 0xA7
		XRA,  XRA,	XRA,  XRA,	XRA,   XRA,	 XRA, XRA, // 0xA8 - 0xAF
		ORA,  ORA,	ORA,  ORA,	ORA,   ORA,	 ORA, ORA, // 0xB0 - 0xB7
		CMP,  CMP,	CMP,  CMP,	CMP,   CMP,	 CMP, CMP, // 0xB8 - 0xBF
		RETC, POP,	JMPC, JMP,	CALLC, PUSH, ADI, RST, // 0xC0 - 0xC7
		RETC, RET,	JMPC, JMP,	CALLC, CALL, ACI, RST, // 0xC7 - 0xCF
		RETC, POP,	JMPC, OUT,	CALLC, PUSH, SUI, RST, // 0xD0 - 0xD7
		RETC, RET,	JMPC, IN,	CALLC, CALL, SBI, RST, // 0xD8 - 0xDF
		RETC, POP,	JMPC, XTHL, CALLC, PUSH, ANI, RST, // 0xE0 - 0xE7
		RETC, PCHL, JMPC, XCHG, CALLC, CALL, XRI, RST, // 0xE8 - 0xEF
		RETC, POP,	JMPC, DI,	CALLC, PUSH, ORI, RST, // 0xF0 - 0xF7
		RETC, SPHL, JMPC, EI,	CALLC, CALL, CPI, RST, // 0xF8 - 0xFF
	};

	// Fetching next opcode
	// If Interrupt occured and Interrupts are enabled, jump to the
	// subroutine
	if (cpu->interrupt_enabled && cpu->interrupt) {
		cpu->opcode = cpu->interrupt;
		cpu->interrupt = 0;
	} else {
		cpu->opcode = readMemoryValue(cpu->PC);
	}

	// printf("Executing: %02x PC: %04x\n", cpu->opcode, cpu->PC);
	return (*jumptable[cpu->opcode])(cpu);
}

void setInterruptRoutine(cpu_t *cpu, uint8_t interrupt)
{
	if (cpu->interrupt_enabled) {
		cpu->interrupt = interrupt;
	}
}
