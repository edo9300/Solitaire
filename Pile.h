/*
 * Copyright (c) 2022, Edoardo Lolletti <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: Zlib
 */
#ifndef PILE_H
#define PILE_H

#include <list> //std::list
#include <utility> //std::exchange, std::pair
#include <optional>
#include <cstdint>

#include "Card.h"

class Pile {
	std::list<Card> m_cards;
	bool m_is_compacted{false};
	std::pair<uint32_t, int> GetYIncrement() const {
		return { Card::HEIGHT / 5u, m_is_compacted ? static_cast<int>(m_cards.size() - 1) : 0 };
	}
	void MoveNElementsToList(size_t to_skip, std::list<Card>& other, bool at_end = false);
public:
	Pile() = default;
	void AddCard(SUIT suit, uint8_t number) {
		m_cards.emplace_back(suit, number);
	}

	bool MergePile(Pile& other) {
		if(!empty() && m_cards.back().IsVisible() && (m_cards.back().GetNumber() - 1) != other.m_cards.front().GetNumber())
			return false;
		other.MoveNElementsToList(0, m_cards, true);
		return true;
	}
	
	template<typename T>
	void DrawAt(int32_t x, int32_t start_y, [[maybe_unused]] uint32_t max_y, T&& draw_func) const {
		if(empty())
			return;
		auto [increment, cards_to_skip] = GetYIncrement();
		auto it = m_cards.begin();
		std::advance(it, cards_to_skip);
		for(; it != m_cards.end(); ++it) {
			it->DrawAt(x, start_y, draw_func);
			start_y += increment;
		}
	}
	
	void MakeLastCardVisible() {
		if(empty())
			return;
		m_cards.back().ToggleVisibility(true);
	}
	
	std::optional<Pile> CheckForCompletition();
	
	bool HitTestAndSplice([[maybe_unused]] int32_t mouse_x, int32_t mouse_y, Pile& other);
	
	bool empty() const { return m_cards.empty(); }
	
	size_t size() const { return m_cards.size(); }
	
	void CompactPile() { m_is_compacted = true; }
};

#endif //PILE_H