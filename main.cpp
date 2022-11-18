/*
 * Copyright (c) 2022, Edoardo Lolletti <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: Zlib
 */
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_main.h>
#include <stdexcept> //std::runtime_error
#include <optional>
#include "GameWindow.h"

extern "C" int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
	SDL_Init(SDL_INIT_VIDEO);
	IMG_Init(IMG_INIT_PNG);
	std::optional<GameWindow> game{ std::nullopt };
	try {
		game.emplace();
	} catch(const std::exception& e) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Initialization error", e.what(), nullptr);
		SDL_Quit();
		return 1;
	}
	game->run();
	SDL_Quit();
	return 0;
}
