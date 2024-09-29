#include "flags.h"
#include "cpu.h"
#include <stdint.h>
#include <stdio.h>

void set_flag(cpu_t *cpu, enum FLAGS flag)
{
	cpu->AF.lowByte |= flag;
}

void clear_flag(cpu_t *cpu, enum FLAGS flag)
{
	cpu->AF.lowByte &= ~flag;
}

void handle_parity(cpu_t *cpu, uint8_t byte)
{
	uint8_t count = 0;

	for (int i = 0; i < 8; i++) {
		if (byte & (1 << i)) {
			count++;
		}
	}

	if (count % 2 == 0) {
		set_flag(cpu, PARITY);
	} else {
		clear_flag(cpu, PARITY);
	}
}

void handle_sign(cpu_t *cpu, uint8_t byte)
{
	if (byte & 0x80) {
		set_flag(cpu, SIGN);
	} else {
		clear_flag(cpu, SIGN);
	}
}

void handle_zero(cpu_t *cpu, uint8_t byte)
{
	if (byte == 0) {
		set_flag(cpu, ZERO);
	} else {
		clear_flag(cpu, ZERO);
	}
}

void handle_halfcarry8(cpu_t *cpu, uint8_t byte1, uint8_t byte2,
					   uint8_t isSubtraction)
{
	// uint8_t aux = ((cpu->AF.highByte & 0x0F) + (value & 0x0F)) > 0x0F;
	// uint8_t aux = ((cpu->AF.highByte & 0x0F) - ((value + carry) & 0x0F)) < 0x0F;

	if ((isSubtraction && (byte1 & 0x0F) - (byte2 & 0x0F) < 0x0F) ||

		(!isSubtraction && ((byte1 & 0x0F) + (byte2 & 0x0F)) > 0x0F)) {
		set_flag(cpu, AUXCARRY);
	} else {
		clear_flag(cpu, AUXCARRY);
	}
}

void handle_carry8(cpu_t *cpu, uint8_t byte1, uint8_t byte2,
				   uint8_t isSubtraction)
{
	if ((isSubtraction && byte1 < byte2) ||
		(!isSubtraction && byte1 + byte2 > 0xFF)) {
		set_flag(cpu, CARRY);
	} else {
		clear_flag(cpu, CARRY);
	}
}

void handle_carry16(cpu_t *cpu, uint16_t word1, uint16_t word2,
					uint8_t isSubtraction)
{
	if ((isSubtraction && word1 < word2) ||
		(!isSubtraction && word1 + word2 > 0xFFFF)) {
		set_flag(cpu, CARRY);
	} else {
		clear_flag(cpu, CARRY);
	}
}
