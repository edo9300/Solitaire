/*
 * Copyright (c) 2022, Edoardo Lolletti <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: Zlib
 */
#ifndef GAME_WINDOW_H
#define GAME_WINDOW_H

#include <cstdint>

#include "Card.h"
#include "Pile.h"
#include "GameBoard.h"

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

class GameWindow {
	uint32_t m_display_format{};
	SDL_Window* m_window{};
	SDL_Renderer* m_renderer{};
	SDL_Texture* m_cards_texture{};
	int32_t m_mouse_x{};
	int32_t m_mouse_y{};
	GameBoard m_board;
	bool EventLoop();
	void DrawBoard();
	SDL_Texture* LoadSpriteTexture(const char* path) const;
public:
	GameWindow();
	~GameWindow();
	void run();
};

#endif //GAME_WINDOW_H