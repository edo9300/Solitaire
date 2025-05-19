/*
 * Copyright (c) 2022, Edoardo Lolletti <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: Zlib
 */
#include <algorithm> //std::find_if

#include "Pile.h"

void Pile::MoveNElementsToList(size_t to_skip, std::list<Card>& other, bool at_end) {
	auto begin = m_cards.begin();
	std::advance(begin, to_skip);
	other.splice(at_end ? other.end() : other.begin(), m_cards, begin, m_cards.end());
}

template<typename Iterator>
static bool DoCardsInIteratorRangeMakeAValidSequence(Iterator first, Iterator last) {
	while(first != last) {
		auto& below = *first++;
		if(first == last)
			return true;
		auto& above = *first;
		if(below.GetSuit() != above.GetSuit() || (below.GetNumber() + 1) != above.GetNumber())
			return false;
	}
	return true;
}

std::optional<Pile> Pile::CheckForCompletition() {
	if(size() < 13)
		return { std::nullopt };
	const auto end = m_cards.end();
	auto find_next = [&](auto it) {
		return std::find_if(it, end, [](const Card& c) {
			return c.GetNumber() == 13;
		});
	};
	auto it = find_next(m_cards.begin());
	while(it != m_cards.end()) {
		if(std::distance(it, end) < 13)
			return { std::nullopt };
		uint8_t target_number = 13;
		const SUIT target_suit = it->GetSuit();
		for(; it != end || target_number != 0; ++it, --target_number) {
			if(it->GetSuit() != target_suit)
				break;
			if(it->GetNumber() != target_number)
				break;
		}
		if(target_number == 0) {
			Pile temp_pile;
			MoveNElementsToList(static_cast<size_t>(std::distance(m_cards.begin(), it)) - 13, temp_pile.m_cards);
			return { std::move(temp_pile) };
		}
		it = find_next(it);
	}
	return { std::nullopt };
}

bool Pile::HitTestAndSplice([[maybe_unused]] int32_t mouse_x, int32_t mouse_y, Pile& other) {
	auto [increment, cards_to_skip] = GetYIncrement();
	auto start_y = 20 + static_cast<int32_t>(increment * (m_cards.size() - cards_to_skip - 1));
	auto it = m_cards.rbegin();
	for(int i = (static_cast<int>(m_cards.size()) - 1); i >= cards_to_skip; --i) {
		if(start_y <= mouse_y && start_y + Card::HEIGHT >= mouse_y) {
			if(!it->IsVisible())
				return false;
			if(!DoCardsInIteratorRangeMakeAValidSequence(m_cards.rbegin(), std::next(it)))
				return false;
			MoveNElementsToList(i, other.m_cards);
			return true;
		}
		++it;
		start_y -= increment;
	}
	return false;
}