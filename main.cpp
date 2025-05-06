/*
 * Copyright (c) 2022, Edoardo Lolletti <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: Zlib
 */
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdexcept> //std::exception
#include "GameWindow.h"

SDL_AppResult SDL_AppInit(void **appstate, [[maybe_unused]] int argc, [[maybe_unused]] char *argv[]){
	SDL_Init(SDL_INIT_VIDEO);
	try {
		auto* game = new GameWindow();
		*appstate = game;
	} catch(const std::exception& e) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Initialization error", e.what(), nullptr);
		return SDL_APP_FAILURE;
	}
	return SDL_APP_CONTINUE;
}
SDL_AppResult SDL_AppIterate([[maybe_unused]] void *appstate){
	return SDL_APP_CONTINUE;
}
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event){
	return static_cast<SDL_AppResult>(static_cast<GameWindow*>(appstate)->onEvent(*event));
}
void SDL_AppQuit(void *appstate, SDL_AppResult result){
	delete static_cast<GameWindow*>(appstate);
}
