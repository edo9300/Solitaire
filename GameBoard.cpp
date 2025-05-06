/*
 * Copyright (c) 2022, Edoardo Lolletti <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: Zlib
 */
#include <utility>
#include <algorithm> //std::find_if, std::shuffle
#include <random> //std::mt19937

#include "GameBoard.h"

GameBoard::GameBoard() {
	auto make_random_array = []{
		std::array<std::pair<SUIT, uint8_t>, TOTAL_DECKS * 13> ret;
		for(uint8_t i = 0; i < TOTAL_DECKS; ++i) {
			for(uint8_t j = 0; j < 13; ++j)
				ret[i * 13u + j] = { static_cast<SUIT>(i / 2), static_cast<uint8_t>(j + 1) };
		}
		std::mt19937 rnd(std::random_device{}());
		std::shuffle(ret.begin(), ret.end(), rnd);
		return ret;
	};
	auto arr = make_random_array();
	size_t pile_idx = 0;
	for(auto [suit, number] : arr) {
		m_piles[pile_idx].AddCard(suit, number);
		pile_idx = (pile_idx + 1) % m_piles.size();
	}
	for(auto& pile : m_piles) {
		pile.MakeLastCardVisible();
	}
}

bool GameBoard::TryPick(int32_t mouse_x, int32_t mouse_y) {
	for(size_t i = 0; i < m_piles.size(); ++i) {
		auto [left_bound, right_bound] = GetColumnBounds(i);
		if(mouse_x < left_bound || mouse_x > right_bound)
			continue;
		const auto ret = m_piles[i].HitTestAndSplice(mouse_x, mouse_y, m_floating_pile);
		if(ret)
			m_previous_pile = &m_piles[i];
		return ret;
	}
	return false;
}
bool GameBoard::TryDrop(int32_t mouse_x, [[maybe_unused]] int32_t mouse_y) {
	for(size_t i = 0; i < m_piles.size(); ++i) {
		auto [left_bound, right_bound] = GetColumnBounds(i);
		if(mouse_x < left_bound || mouse_x > right_bound)
			continue;
		auto& target_pile = m_piles[i];
		if(!target_pile.MergePile(m_floating_pile))
			return false;
		std::exchange(m_previous_pile, nullptr)->MakeLastCardVisible();
		auto completed_pile = target_pile.CheckForCompletition();
		if(completed_pile.has_value()) {
			auto pile = std::move(*completed_pile);
			pile.CompactPile();
			m_completed_piles.push_back(std::move(pile));
			target_pile.MakeLastCardVisible();
		}
		return true;
	}
	return false;
}
