/*
 * Copyright (c) 2022, Edoardo Lolletti <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: Zlib
 */
#ifndef GAME_BOARD_H
#define GAME_BOARD_H

#include <cstdint>
#include <utility>
#include <algorithm> //std::shuffle
#include <array>

#include "Card.h"
#include "Pile.h"

class GameBoard {
	static constexpr auto COLUMN_OFFSET = 100;
	static constexpr auto TOTAL_DECKS = 8;
	std::array<Pile, 10> m_piles;
	Pile* m_previous_pile{};
	Pile m_floating_pile;
	std::list<Pile> m_completed_piles;
	static std::pair<int32_t, int32_t> GetColumnBounds(size_t column) {
		return { 10 + static_cast<int32_t>(COLUMN_OFFSET * column), static_cast<int32_t>(10 + (COLUMN_OFFSET * column)) + Card::WIDTH };
	}
	bool TryPick(int32_t mouse_x, int32_t mouse_y);
	bool TryDrop(int32_t mouse_x, int32_t mouse_y);
	void RollBack() {
		std::exchange(m_previous_pile, nullptr)->MergePile(m_floating_pile);
	}
public:
	GameBoard();
	template<typename T>
	void Draw(int32_t mouse_x, int32_t mouse_y, T&& draw_func) const {
		int32_t x = 10;
		for(auto& pile : m_piles) {
			pile.DrawAt(x, 20, 50, draw_func);
			x += COLUMN_OFFSET;
		}
		const auto y_offset = m_floating_pile.size() > 1 ? 0 : Card::HEIGHT/2;
		m_floating_pile.DrawAt(mouse_x - Card::WIDTH/2, mouse_y - y_offset, 50, draw_func);
		{
			int startx = 50;
			for(const auto& pile : m_completed_piles) {
				pile.DrawAt(startx, 600, 50, draw_func);
				startx += Card::WIDTH / 3;
			}
		}
	}
	bool TryGrabFromPile(int32_t mouse_x, int32_t mouse_y) {
		return TryPick(mouse_x, mouse_y);
	}
	bool DropToPileOrRollBack(int32_t mouse_x, int32_t mouse_y) {
		if(m_floating_pile.empty())
			return false;
		if(!TryDrop(mouse_x, mouse_y))
			RollBack();
		return true;
	}
};

#endif //GAME_BOARD_H