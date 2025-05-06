/*
 * Copyright (c) 2022, Edoardo Lolletti <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: Zlib
 */
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <stdexcept> //std::runtime_error

#include "GameWindow.h"

GameWindow::GameWindow() {
	m_window = SDL_SetVideoMode(1000, 670, 0, SDL_ANYFORMAT | SDL_DOUBLEBUF);
	if(m_window == nullptr) {
		throw std::runtime_error("Failed to create window and renderer");
	}
	SDL_WM_SetCaption("Solitaire", "");
	m_cards_texture = LoadSpriteTexture("./cards.png");
	if(!m_cards_texture) {
		throw std::runtime_error(SDL_GetError());
	}
}
GameWindow::~GameWindow() {
	SDL_FreeSurface(m_cards_texture);
}

SDL_Surface* GameWindow::LoadSpriteTexture(const char* path) const {
	auto* tmp = IMG_Load(path);
	auto* sprite = SDL_DisplayFormat(tmp);
	SDL_FreeSurface(tmp);
	return sprite;
}

void GameWindow::DrawBoard() {
	SDL_FillRect(m_window, NULL, SDL_MapRGB(m_window->format, 0, 255, 0));
	m_board.Draw(m_mouse_x, m_mouse_y, [&](SDL_Rect rect, SDL_Rect dest_rect) {
		SDL_BlitSurface(m_cards_texture, &rect, m_window, &dest_rect);
	});
	SDL_Flip(m_window);
}

void GameWindow::run() {
	while(EventLoop());
}

static constexpr auto& SDL_WaitFunction = SDL_WaitEvent;

bool GameWindow::EventLoop() {
	SDL_Event e;
	while(SDL_WaitFunction(&e)) {
		switch(e.type) {
		case SDL_QUIT:
			return false;
		case SDL_VIDEOEXPOSE:
		case SDL_VIDEORESIZE:
			DrawBoard();
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
