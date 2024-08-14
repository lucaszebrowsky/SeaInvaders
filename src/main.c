#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_scancode.h>
#include <stdint.h>
#include <stdio.h>
#include <SDL2/SDL_events.h>

#include "bus.h"
#include "cpu.h"
#include "renderer.h"

void handle_input(cpu_t *cpu)
{
	uint8_t p1_input = 0;
	// uint8_t p2_input = cpu->io_port[2];

	const uint8_t *keystate = SDL_GetKeyboardState(NULL);

	// Handle generic inputs
	p1_input |= (keystate[SDL_SCANCODE_C] << 0); // Credit -> Port 1, Bit 0
	p1_input |= (keystate[SDL_SCANCODE_1] << 2); // 1P Start -> Port 1, Bit 2
	p1_input |= (keystate[SDL_SCANCODE_2] << 1); // 2P Start -> Port 1, Bit 1
	// p1_input |= (1 << 3); // Bit 3 is always 1

	// Handle Player 1 inputs
	p1_input |= (keystate[SDL_SCANCODE_UP] << 4); // 1P Fire -> Port 1, Bit 4
	p1_input |= (keystate[SDL_SCANCODE_LEFT] << 5); // 1P Left -> Port 1, Bit 5
	p1_input |=
		(keystate[SDL_SCANCODE_RIGHT] << 6); // 1P Right -> Port 1, Bit 6

	// Handle Player 2 inputs
	// p2_input |= (keystate[SDL_SCANCODE_W] << 4); // 2P Fire -> Port 2, Bit 4
	// p2_input |= (keystate[SDL_SCANCODE_A] << 5); // 2P Left -> Port 2, Bit 5
	// p2_input |= (keystate[SDL_SCANCODE_D] << 6); // 2P Right -> Port 2, Bit 6

	// Write the calculated inputs to the CPU ports
	cpu->io_port[1] = p1_input;
	// cpu->io_port[2] = p2_input;
}

void emu_run(cpu_t *cpu, memory_t *memory)
{
	// CPU -> 2 000 000 HZ
	// Screen -> 60 HZ
	// -> 200000/60 = 33333 -> ~33K cycles per Frame
	uint32_t cycles = 0;
	const uint32_t maxcycles = 2000000 / 60; // ~33K Max Cycles per frame

	// maxcycles / 2 -> every half an interrupt occurs
	while (cycles <= maxcycles / 2) {
		cycles += executeNextInstruction(cpu);
	}

	handle_input(cpu);

	setInterruptRoutine(cpu, 0xCF);
	drawScreen(memory);

	while (cycles <= maxcycles) {
		cycles += executeNextInstruction(cpu);
	}
	handle_input(cpu);

	setInterruptRoutine(cpu, 0xD7);
	drawScreen(memory);
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("Usage: %s <path_to_rom>\n", argv[0]);
		return 1;
	}

	memory_t memory;
	cpu_t cpu;
	SDL_Event event;

	initSDL();
	initBus(&memory);
	loadROM(argv[1]);
	initCPU(&cpu);

	while (1) {
		if (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				break;
			}
		}

		emu_run(&cpu, &memory);
	}

	killSDL();

	return 0;
}
