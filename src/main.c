#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_timer.h>
#include <stdint.h>
#include <stdio.h>

#include "bus.h"
#include "cpu.h"
#include "renderer.h"
#include "input_handler.h"

void emulate_frame(cpu_t *cpu, memory_t *memory)
{
	// CPU -> 2 000 000 HZ
	// Screen -> 60 HZ
	// -> 200000/60 = 33333 -> ~33K cycles per Frame
	uint32_t cycles = 0;
	const uint32_t maxcycles = 2000000 / 60; // ~33K Max Cycles per frame
	uint64_t start = SDL_GetPerformanceCounter();

	// maxcycles / 2 -> every half an interrupt occurs
	while (cycles <= maxcycles / 2) {
		cycles += step(cpu);
	}

	setInterruptRoutine(cpu, 0xCF);
	drawScreen(memory);

	while (cycles <= maxcycles) {
		cycles += step(cpu);
	}

	setInterruptRoutine(cpu, 0xD7);
	drawScreen(memory);

	uint64_t end = SDL_GetPerformanceCounter();
	float elapsedMS =
		(end - start) / (float)SDL_GetPerformanceFrequency() * 1000.0f;
	SDL_Delay(floor(16.666f - elapsedMS));
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("Usage: %s <path_to_rom>\n", argv[0]);
		return 1;
	}

	memory_t memory;
	cpu_t cpu;
	uint8_t running = 1;

	initSDL();
	initBus(&memory);
	loadROM(argv[1]);
	initCPU(&cpu);

	while (running) {
		handle_events(&cpu, &running);
		emulate_frame(&cpu, &memory);
	}

	killSDL();

	return 0;
}
