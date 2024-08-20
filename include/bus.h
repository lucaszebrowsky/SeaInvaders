#pragma once
#include "cpu.h"
#include <stdint.h>

/*
 *   Memory Map:
 *
 *   0000-1FFF 8K ROM
 *   2000-23FF 1K RAM
 *   2400-3FFF 7K Video RAM
 *   4000-???? ?k RAM mirror
 *
 */
typedef struct memory {
	uint8_t rom[0x2000]; // 8KB of ROM Storage
	uint8_t ram[0x400]; // 1KB of RAM
	uint8_t vram[0x1C00]; // 7KB of VRAM
} memory_t;

// Clear the memory
void initBus(memory_t *memory);

// Load the provided ROM into memory
void loadROM(char *path);

// Write a byte to a given address in memory
void writeByteToMemory(uint8_t data, uint16_t address);

// Return a pointer to the given address
uint8_t *getAddressPointer(uint16_t address);

// Wrapper around getAddressPointer which only returns the value
uint8_t readMemoryValue(uint16_t address);
