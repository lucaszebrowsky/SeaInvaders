#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_scancode.h>
#include <stdint.h>
#include <stdio.h>
// #include <stdlib.h>
#include <SDL2/SDL_events.h>

#include "bus.h"
#include "cpu.h"
#include "renderer.h"

// const int keymap[] = {
// 	SDL_SCANCODE_A,	   SDL_SCANCODE_D, // Player 2 Left/Right
// 	SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, // Player 1 Left/Right
// 	SDL_SCANCODE_1,	   SDL_SCANCODE_2, // Select Player 1 or 2 Mode
// 	SDL_SCANCODE_UP, // Player 1 Fire
// 	SDL_SCANCODE_W, // Player 2 Fire,
// 	SDL_SCANCODE_C // Credit
// };

void handle_input(cpu_t *cpu)
{
	uint8_t p1_input = 0;
	// uint8_t p2_input = 0;

	const uint8_t *keystate = SDL_GetKeyboardState(NULL);

	p1_input |= keystate[SDL_SCANCODE_C];
	p1_input = p1_input | (keystate[SDL_SCANCODE_1] << 2);

	p1_input = p1_input | (keystate[SDL_SCANCODE_2] << 1);

	cpu->io_port[1] = p1_input;

	// for (uint8_t i = 0; i < 2; i++) {
	// }
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
		printf("Usdage: %s <path_to_rom>\n", argv[0]);
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
