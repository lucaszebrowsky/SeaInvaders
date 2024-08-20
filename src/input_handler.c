
#include "cpu.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <stdint.h>
#include "input_handler.h"

void handle_input(cpu_t *cpu)
{
	uint8_t p1_input = 0;
	uint8_t p2_input = 0;

	SDL_PumpEvents();
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
	p2_input |= (keystate[SDL_SCANCODE_W] << 4); // 2P Fire -> Port 2, Bit 4
	p2_input |= (keystate[SDL_SCANCODE_A] << 5); // 2P Left -> Port 2, Bit 5
	p2_input |= (keystate[SDL_SCANCODE_D] << 6); // 2P Right -> Port 2, Bit 6

	// Write the calculated inputs to the CPU ports
	cpu->io_port[1] = p1_input;
	cpu->io_port[2] = p2_input;
}

void handle_events(cpu_t *cpu, uint8_t *running)
{
	SDL_Event event;

	if (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) {
			*running = 0;
		}
	}

	handle_input(cpu);
}
