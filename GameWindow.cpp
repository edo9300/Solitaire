/*
 * Copyright (c) 2022, Edoardo Lolletti <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: Zlib
 */
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <stdexcept> //std::runtime_error

#include "GameWindow.h"

GameWindow::GameWindow() {
	if(SDL_CreateWindowAndRenderer(1000, 670, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE, &m_window, &m_renderer) != 0) {
		if(m_window)
			SDL_DestroyWindow(m_window);
		throw std::runtime_error("Failed to create window and renderer");
	}
	SDL_SetWindowTitle(m_window, "Solitaire");
	SDL_RenderSetLogicalSize(m_renderer, 1000, 670);
	SDL_DisplayMode mode;
	SDL_GetCurrentDisplayMode(-1, &mode);
	m_display_format = mode.format;
	m_cards_texture = LoadSpriteTexture("./cards.png");
	if(!m_cards_texture) {
		SDL_DestroyRenderer(m_renderer);
		SDL_DestroyWindow(m_window);
		throw std::runtime_error(SDL_GetError());
	}
}
GameWindow::~GameWindow() {
	SDL_DestroyTexture(m_cards_texture);
	SDL_DestroyRenderer(m_renderer);
	SDL_DestroyWindow(m_window);
}

SDL_Texture* GameWindow::LoadSpriteTexture(const char* path) const {
	auto* tmp = IMG_Load(path);
	if(!tmp) return nullptr;
	auto* sprite = SDL_CreateTextureFromSurface(m_renderer, tmp);
	SDL_FreeSurface(tmp);
	return sprite;
}

void GameWindow::DrawBoard() {
	SDL_SetRenderDrawColor(m_renderer, 0, 255, 0, 255);
	SDL_RenderClear(m_renderer);
	m_board.Draw(m_mouse_x, m_mouse_y, [&](const SDL_Rect& rect, const SDL_Rect& dest_rect) {
		SDL_RenderCopy(m_renderer, m_cards_texture, &rect, &dest_rect);
	});
	SDL_RenderPresent(m_renderer);
}


void GameWindow::run() {
#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop_arg([](void* ptr){
		if(!static_cast<GameWindow*>(ptr)->EventLoop())
			emscripten_cancel_main_loop();
	}, this, -1, 1);
#else
	while(EventLoop());
#endif
}

#ifndef __EMSCRIPTEN__
static constexpr auto& SDL_WaitFunction = SDL_WaitEvent;
#else
static const auto& SDL_WaitFunction = emscripten_has_asyncify() ? SDL_WaitEvent : SDL_PollEvent;
#endif

bool GameWindow::EventLoop() {
	SDL_Event e;
	while(SDL_WaitFunction(&e)) {
		switch(e.type) {
		case SDL_QUIT:
			return false;
		case SDL_WINDOWEVENT:
			switch(e.window.event) {
			case SDL_WINDOWEVENT_RESIZED:
			case SDL_WINDOWEVENT_SIZE_CHANGED:
			case SDL_WINDOWEVENT_SHOWN:
				DrawBoard();
				break;
			case SDL_WINDOWEVENT_FOCUS_LOST:
				if(m_board.DropToPileOrRollBack(m_mouse_x, m_mouse_y))
					DrawBoard();
				break;
			}
			break;
		case SDL_MOUSEMOTION:
			m_mouse_x = e.motion.x;
			m_mouse_y = e.motion.y;
			DrawBoard();
			break;
		case SDL_MOUSEBUTTONDOWN:
			m_mouse_x = e.button.x;
			m_mouse_y = e.button.y;
			if(e.button.button == 1 && m_board.TryGrabFromPile(e.button.x, e.button.y))
				DrawBoard();
			break;
		case SDL_MOUSEBUTTONUP:
			m_mouse_x = e.button.x;
			m_mouse_y = e.button.y;
			if(e.button.button == 1 && m_board.DropToPileOrRollBack(e.button.x, e.button.y))
				DrawBoard();
			break;
		}
	}
	return true;
}