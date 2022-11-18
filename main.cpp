#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_main.h>
#include <stdexcept>
#include <memory>
#include <list>
#include <array>
#include <cstdlib>
#include <algorithm>
#include <random>
#include <utility>
#include <optional>
// NOTE: Remember to compile with `-s USE_SDL=2` and `-s USE_SDL_IMAGE=2` if using emscripten.
// build with g++ --std=c++17 main.cpp -lSDL2 -lSDL2_image otherwise
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

enum class SUIT : Uint32 {
	CLUB,
	DIAMOND,
	HEART,
	SPADE,
};

class Card {
	SUIT m_suit;
	Uint8 m_number;
	bool m_is_hidden;
	static SDL_Rect GetTextureCoords(Uint8 suit, Uint8 idx) {
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
	Card(SUIT suit, Uint8 number) : m_suit(suit), m_number(number), m_is_hidden(true) {}
	SUIT GetSuit() const { return m_suit; }
	Uint8 GetNumber() const { return m_number; }
	SDL_Rect GetTextureRect()  const {
		if(m_is_hidden)
			return GetBackTextureRect();
		return GetTextureCoords(static_cast<Uint8>(m_suit), m_number - 1u);
	}
	void ToggleVisibility(bool is_visible) {
		m_is_hidden = !is_visible;
	}
	bool IsVisible() const {
		return !m_is_hidden;
	}
	template<typename T>
	void DrawAt(Sint32 x, Sint32 y, T&& draw_func) const {
		SDL_Rect dest_rect{ x, y, 71, 96 };
		draw_func(GetTextureRect(), dest_rect);
	}
};

class Pile {
	std::list<Card> m_cards;
	bool m_is_compacted{false};
	std::pair<Uint32, size_t> GetYIncrement() const {
		return { Card::HEIGHT / 5u, m_is_compacted ? static_cast<size_t>(m_cards.size() - 1) : 0 };
	}
	void MoveNElementsToList(size_t to_skip, std::list<Card>& other, bool at_end = false) {
		auto begin = m_cards.begin();
		std::advance(begin, to_skip);
		other.splice(at_end ? other.end() : other.begin(), m_cards, begin, m_cards.end());
	}
public:
	Pile() = default;
	void AddCard(SUIT suit, Uint8 number) {
		m_cards.emplace_back(suit, number);
	}
	bool MergePile(Pile& other) {
		if(!empty() && m_cards.back().IsVisible() && (m_cards.back().GetNumber() - 1) != other.m_cards.front().GetNumber())
			return false;
		other.MoveNElementsToList(0, m_cards, true);
		return true;
	}
	template<typename T>
	void DrawAt(Sint32 x, Sint32 start_y, [[maybe_unused]] Uint32 max_y, T&& draw_func) const {
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
	std::optional<Pile> CheckForCompletition() {
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
			Uint8 target_number = 13;
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
	template<typename Iterator>
	bool DoCardsInIteratorRangeMakeAValidSequence(Iterator first, Iterator last) {
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
	bool HitTestAndSplice([[maybe_unused]] Sint32 mouse_x, Sint32 mouse_y, Pile& other) {
		auto [increment, cards_to_skip] = GetYIncrement();
		auto start_y = 20 + static_cast<Sint32>(increment * (m_cards.size() - cards_to_skip - 1));
		auto it = m_cards.rbegin();
		for(size_t i = m_cards.size() - 1; i >= cards_to_skip; --i) {
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
	bool empty() const { return m_cards.empty(); }
	size_t size() const { return m_cards.size(); }
	void CompactPile() { m_is_compacted = true; }
};

class GameBoard {
	static constexpr auto COLUMN_OFFSET = 100;
	static constexpr auto TOTAL_DECKS = 8;
	static auto make_random_array() {
		std::array<std::pair<SUIT, Uint8>, TOTAL_DECKS * 13> ret;
		for(Uint8 i = 0; i < TOTAL_DECKS; ++i) {
			for(Uint8 j = 0; j < 13; ++j)
				ret[i * 13u + j] = { static_cast<SUIT>(i / 2), static_cast<Uint8>(j + 1) };
		}
		std::mt19937 rnd(std::random_device{}());
		std::shuffle(ret.begin(), ret.end(), rnd);
		return ret;
	}
	std::array<Pile, 10> m_piles;
	Pile* m_previous_pile{};
	Pile m_floating_pile;
	std::list<Pile> m_completed_piles;
	static std::pair<Sint32, Sint32> GetColumnBounds(size_t column) {
		return { 10 + static_cast<Sint32>(COLUMN_OFFSET * column), static_cast<Sint32>(10 + (COLUMN_OFFSET * column)) + Card::WIDTH };
	}
	bool TryPick(Sint32 mouse_x, Sint32 mouse_y) {
		for(size_t i = 0; i < m_piles.size(); ++i) {
			auto [left_bound, right_bound] = GetColumnBounds(i);
			if(mouse_x >= left_bound && mouse_x <= right_bound) {
				const auto ret = m_piles[i].HitTestAndSplice(mouse_x, mouse_y, m_floating_pile);
				if(ret)
					m_previous_pile = &m_piles[i];
				return ret;
			}
		}
		return false;
	}
	bool TryDrop(Sint32 mouse_x, [[maybe_unused]] Sint32 mouse_y) {
		for(size_t i = 0; i < m_piles.size(); ++i) {
			auto [left_bound, right_bound] = GetColumnBounds(i);
			if(mouse_x >= left_bound && mouse_x <= right_bound) {
				auto& target_pile = m_piles[i];
				const auto ret = target_pile.MergePile(m_floating_pile);
				if(ret) {
					std::exchange(m_previous_pile, nullptr)->MakeLastCardVisible();
					auto completed_pile = target_pile.CheckForCompletition();
					if(completed_pile.has_value()) {
						auto pile = std::move(*completed_pile);
						pile.CompactPile();
						m_completed_piles.push_back(std::move(pile));
					}
				}
				return ret;
			}
		}
		return false;
	}
	void RollBack() {
		std::exchange(m_previous_pile, nullptr)->MergePile(m_floating_pile);
	}
public:
	GameBoard() {
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
	template<typename T>
	void Draw(Sint32 mouse_x, Sint32 mouse_y, T&& draw_func) const {
		Sint32 x = 10;
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
	bool TryGrabFromPile(Sint32 mouse_x, Sint32 mouse_y) {
		return TryPick(mouse_x, mouse_y);
	}
	bool DropToPileOrRollBack(Sint32 mouse_x, Sint32 mouse_y) {
		if(m_floating_pile.empty())
			return false;
		if(!TryDrop(mouse_x, mouse_y))
			RollBack();
		return true;
	}
};

class GameWindow {
	Uint32 m_display_format{};
	SDL_Window* m_window{};
	SDL_Renderer* m_renderer{};
	SDL_Texture* m_cards_texture{};
	Sint32 m_mouse_x{};
	Sint32 m_mouse_y{};
	GameBoard m_board;
	bool EventLoop();
	void DrawBoard();
	SDL_Texture* LoadSpriteTexture(const char* path) const {
		auto* tmp = IMG_Load(path);
		if(!tmp) return nullptr;
		auto* sprite = SDL_CreateTextureFromSurface(m_renderer, tmp);
		SDL_FreeSurface(tmp);
		return sprite;
	}
public:
	GameWindow() {
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
	~GameWindow() {
		SDL_DestroyTexture(m_cards_texture);
		SDL_DestroyRenderer(m_renderer);
		SDL_DestroyWindow(m_window);
	}
	void run() {
#ifdef __EMSCRIPTEN__
		emscripten_set_main_loop_arg([](void* ptr){
			static_cast<GameWindow*>(ptr)->EventLoop();
		}, this, -1, 1);
#else
		while(EventLoop());
#endif
	}
};

void GameWindow::DrawBoard() {
	SDL_SetRenderDrawColor(m_renderer, 0, 255, 0, 255);
	SDL_RenderClear(m_renderer);
	m_board.Draw(m_mouse_x, m_mouse_y, [&](const SDL_Rect& rect, const SDL_Rect& dest_rect) {
		SDL_RenderCopy(m_renderer, m_cards_texture, &rect, &dest_rect);
	});
	SDL_RenderPresent(m_renderer);
}

bool GameWindow::EventLoop() {
	SDL_Event e;
	while(SDL_WaitEvent(&e)) {
		switch(e.type) {
		case SDL_QUIT: {
#ifdef __EMSCRIPTEN__
			emscripten_cancel_main_loop();
#endif
			return false;
		}
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

extern "C" int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
	SDL_Init(SDL_INIT_VIDEO);
	IMG_Init(IMG_INIT_PNG);
	std::unique_ptr<GameWindow> game{};
	try {
		game = std::make_unique<GameWindow>();
	} catch(const std::runtime_error& e) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Initialization error", e.what(), nullptr);
		SDL_Quit();
		return 1;
	}
	game->run();
	SDL_Quit();
	return 0;
}
