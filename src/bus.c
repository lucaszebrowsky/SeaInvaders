#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bus.h"

static memory_t *_memory = NULL;

void initBus(memory_t *memory)
{
	memset(memory->rom, 0, sizeof(memory->rom));
	memset(memory->ram, 0, sizeof(memory->ram));
	memset(memory->vram, 0, sizeof(memory->vram));

	_memory = memory; // Saving a Reference
}

//  Load the file at the given path into memory
void loadROM(char *path)
{
	FILE *file = fopen(path, "rb");

	if (NULL == file) {
		fprintf(stderr, "Could not open ROM: %s", path);
		exit(EXIT_FAILURE);
	}

	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	fseek(file, 0, SEEK_SET);

	if (size > sizeof(_memory->rom)) {
		fprintf(stderr, "Your ROM is too big! Max Size is %zu bytes!\n",
				sizeof(_memory->rom));
		exit(EXIT_FAILURE);
	}

	size_t result = fread(_memory->rom, sizeof(_memory->rom), 1, file);

	if (1 != result) {
		fprintf(stderr, "Failed to load ROM into memory!");
		exit(EXIT_FAILURE);
	}

	printf("Rom loaded successfully!\n");

	fclose(file);
	file = NULL;
}

/*
 *  Wrapper function around getAddressPointer.
 *  Returns only the value stored at the specified memory address
 */
uint8_t readMemoryValue(uint16_t address)
{
	return *getAddressPointer(address);
}

//  Returns a pointer to specified address in memory
uint8_t *getAddressPointer(uint16_t address)
{
	if (address == 0x20F7) {
		printf("BUS read: 0x20F7, data: %x\n", _memory->ram[address - 0x2000]);
	}

	if (address <= 0x1FFF) {
		return &_memory->rom[address];
	} else if (address >= 0x2000 && address <= 0x23FF) {
		return &_memory->ram[address - 0x2000];
	} else if (address >= 0x2400 && address <= 0x3FFF) {
		return &_memory->vram[address - 0x2400];
	} else if (address >= 0x4000 &&
			   address <= 0x43FF) { // Mirror of RAM (Work RAM and VRAM?)
		return &_memory->ram[address - 0x4000];
	} else if (address >= 0x4400 && address <= 0x5FFF) { // Mirror of VRAM
		return &_memory->vram[address - 0x4400];
	} else {
		fprintf(stderr,
				"Error! Tried to read data from out-of-range address: %04x\n",
				address);

		exit(EXIT_FAILURE);
	}
}

//  Write a 8-Bit value to a specific address in memory
void writeByteToMemory(uint8_t data, uint16_t address)
{
	if (address == 0x20F7 && data == 0xff) {
		printf("BUS write: 0x20F7, data: %x\n", data);
		// exit(1);
	}
	if (address >= 0x2000 && address <= 0x23FF) {
		_memory->ram[address - 0x2000] = data;
	} else if (address >= 0x2400 && address <= 0x3FFF) {
		printf("VRAM (%x): %x\n", address, data);
		_memory->vram[address - 0x2400] = data;
	} else if (address >= 0x4000 && address <= 0x43FF) { // Mirror of RAM
		_memory->ram[address - 0x4000] = data;
	} else if (address >= 0x4400 && address <= 0x5FFF) { // Mirror of VRAM
		printf("VRAM (%x): %x\n", address, data);
		_memory->vram[address - 0x4400] = data;
	} else {
		fprintf(stderr,
				"Error! Tried to write data to out-of-range address: %04x "
				"(data: %02x)\n",
				address, data);
		exit(EXIT_FAILURE);
	}
}
