/*
 * Copyright (c) 2022, Edoardo Lolletti <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: Zlib
 */
#ifndef CARD_H
#define CARD_H

#include <cstdint>
#include <SDL2/SDL.h>

enum class SUIT : uint32_t {
	CLUB,
	DIAMOND,
	HEART,
	SPADE,
};

class Card {
	SUIT m_suit;
	uint8_t m_number;
	bool m_is_hidden;
	static SDL_Rect GetTextureCoords(uint8_t suit, uint8_t idx) {
		SDL_Rect rect{};
		rect.w = WIDTH;
		rect.h = HEIGHT;
		rect.x = WIDTH * idx;
		rect.y = HEIGHT * suit;
		return rect;
	}
public:
	static constexpr auto WIDTH = 71;
	static constexpr auto HEIGHT = 96;
	
	static SDL_Rect GetBackTextureRect() {
		return GetTextureCoords(4, 0);
	}
	
	Card(SUIT suit, uint8_t number) : m_suit(suit), m_number(number), m_is_hidden(true) {}
	
	SDL_Rect GetTextureRect()  const {
		if(m_is_hidden)
			return GetBackTextureRect();
		return GetTextureCoords(static_cast<uint8_t>(m_suit), m_number - 1u);
	}
	
	void ToggleVisibility(bool is_visible) {
		m_is_hidden = !is_visible;
	}
	
	bool IsVisible() const { return !m_is_hidden; }
	SUIT GetSuit() const { return m_suit; }
	uint8_t GetNumber() const { return m_number; }
	
	template<typename T>
	void DrawAt(int32_t x, int32_t y, T&& draw_func) const {
		SDL_Rect dest_rect{ x, y, 71, 96 };
		draw_func(GetTextureRect(), dest_rect);
	}
};

#endif //CARD_H