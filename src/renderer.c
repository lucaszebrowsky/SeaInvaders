#include <SDL2/SDL_video.h>
#include <stdint.h>
#include <stdio.h>
#include <SDL2/SDL.h>

#include "bus.h"

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

#define SCALE 3

// 256x224 Pixels
#define WIDTH 256
#define HEIGHT 224

void initSDL(void)
{
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		fprintf(stderr, "Failed to initialize SDL!\n");
		exit(EXIT_FAILURE);
	}

	// Note: The screen in the cabinet is rotated by 90 degrees counter clockwise
	// so we swap the HEIGHT and the WIDTH for the SDL Window accordingly
	window = SDL_CreateWindow("SeaInvaders", SDL_WINDOWPOS_UNDEFINED,
							  SDL_WINDOWPOS_UNDEFINED, HEIGHT * SCALE,
							  WIDTH * SCALE, SDL_WINDOW_RESIZABLE);

	if (NULL == window) {
		fprintf(stderr, "Could not create SDL Window!\n");
		SDL_Quit();
		exit(EXIT_FAILURE);
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	if (NULL == renderer) {
		fprintf(stderr, "Could not create SDL Renderer!\n");
		SDL_DestroyWindow(window);
		SDL_Quit();
		exit(EXIT_FAILURE);
	}
}

void killSDL(void)
{
	if (renderer) {
		SDL_DestroyRenderer(renderer);
	}

	if (window) {
		SDL_DestroyWindow(window);
	}

	SDL_Quit();
}

void drawScreen(memory_t *memory)
{
	int32_t width = 0;
	int32_t height = 0;
	SDL_GetWindowSize(window, &width, &height);

	float scale_x = (float)width / HEIGHT;
	float scale_y = (float)height / WIDTH;

	// First clear the whole screen
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
	SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);

	for (int y = 0; y < HEIGHT; y++) {
		for (int x = 0; x < WIDTH; x++) {
			// Example:
			//	2D Array 4x3 (imagine this is the screen)
			// [ ][ ][ ][ ]
			// [ ][ ][x][ ] -> (2,1)
			// [ ][ ][ ][ ]
			// Here each row has 4 elements representing 1 pixel
			// offset = coord_y * elements_per_row + coord_x / number_of_pixels
			// (how many pixels per element, 1 pixel in this case)
			// offset = 1 * 4 + 2 * 1
			// = 6
			// 	1D Array
			// [][][][][][][x][][][][][] -> arr[6]

			// Each byte represents 8 pixels, there are 256 pixels
			// (horizontal resolution) per row:
			// 256 / 8 = 32 bytes
			// we also have
			// Now since our array is in bytes (meaning 1 element represents 8 pixels),
			// we need to map each byte to 8 pixels
			// offset = y * 32 + x / 8 (if we would multiply instead,
			// it would mean we map each pixel to 8 bytes!)

			uint16_t address = y * 32 + x / 8;
			uint8_t bit = x % 8; // the current pixel we are looking for
			uint8_t data = memory->vram[address];

			if ((data & (1 << bit))) {
				// The screen in the Space Invaders cabinet is rotated 90 degree counter clockwise
				// this means we also flip the coordinate system by 90 degree
				// this causes the normal x,y coordinate system to become x',y' where x' = y and y' = x
				// but since the top left is 0,0, and the bottom left is max_x we need to substract y' from max_x

				// Using rectangles as pixels instead of just small dots looks
				// cleaner but the dot way looks more retro :D

				// SDL_RenderDrawPoint(renderer, y * SCALE,
				// 					(WIDTH - 1 - x) * SCALE);

				SDL_Rect rect;

				rect.x =
					(int)(y *
						  scale_x); // Scale the y-coordinate to fit the new width
				rect.y =
					(int)((WIDTH - 1 - x) *
						  scale_y); // Scale the x-coordinate (inverted) to fit the new height
				rect.w = (int)(scale_x); // Scale the rectangle width
				rect.h = (int)(scale_y);

				SDL_RenderFillRect(renderer, &rect);
			}
		}
	}

	SDL_RenderPresent(renderer);
}
