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
	SDL_Rect GetTextureRect()  const {
		if(m_is_hidden)
			return GetBackTextureRect();
		return GetTextureCoords(static_cast<Uint32>(m_suit), m_number - 1);
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
	std::list<Card> cards;
	static std::pair<Uint32, Sint32> GetYIncrement() {
		return { 96 / 5, 0 };
	}
	void MoveNElementsToList(size_t to_skip, std::list<Card>& other, bool at_end = false) {
		auto begin = cards.begin();
		std::advance(begin, to_skip);
		other.splice(at_end ? other.end() : other.begin(), cards, begin, cards.end());
	}
public:
	Pile() = default;
	void AddCard(SUIT suit, Uint8 number) {
		cards.emplace_back(suit, number);
	}
	bool MergePile(Pile& other) {
		if(!empty() && cards.back().IsVisible())
			return false;
		other.MoveNElementsToList(0, cards, true);
		return true;
	}
	template<typename T>
	void DrawAt(Sint32 x, Sint32 start_y, Uint32 max_y, T&& draw_func) const {
		if(empty())
			return;
		auto [increment, cards_to_skip] = GetYIncrement();
		auto it = cards.begin();
		std::advance(it, cards_to_skip);
		for(; it != cards.end(); ++it) {
			it->DrawAt(x, start_y, draw_func);
			start_y += increment;
		}
	}
	void MakeLastCardVisible() {
		if(empty())
			return;
		cards.back().ToggleVisibility(true);
	}
	bool HitTestAndSplice(Uint32 mouse_x, Uint32 mouse_y, Pile& other) {
		auto [increment, cards_to_skip] = GetYIncrement();
		auto start_y = 20 + (increment * (cards.size() - cards_to_skip - 1));
		auto it = cards.rbegin();
		std::advance(it, cards_to_skip);
		for(int i = cards.size() - 1; i >= cards_to_skip; --i) {
			if(start_y <= mouse_y && start_y + Card::HEIGHT >= mouse_y) {
				if(!it->IsVisible())
					return false;
				MoveNElementsToList(i, other.cards);
				return true;
			}
			++it;
			start_y -= increment;
		}
		return false;
	}
	bool empty() const { return cards.empty(); }
	size_t size() const { return cards.size(); }
};

class GameBoard {
	static constexpr auto COLUMN_OFFSET = 100;
	static constexpr auto TOTAL_DECKS = 8;
	static auto make_random_array() {
		std::array<std::pair<SUIT, Uint8>, TOTAL_DECKS * 13> ret;
		for(int i = 0; i < TOTAL_DECKS; ++i) {
			for(int j = 0; j < 13; ++j)
				ret[i * 13 + j] = { static_cast<SUIT>(i / 2), j + 1 };
		}
		std::mt19937 rnd(std::random_device{}());
		std::shuffle(ret.begin(), ret.end(), rnd);
		return ret;
	}
	std::array<Pile, 10> piles;
	Pile* previous_pile{};
	Pile floating_pile;
	static std::pair<Uint32, Uint32> GetColumnBounds(size_t column) {
		return { 10 + (COLUMN_OFFSET * column), (10 + (COLUMN_OFFSET * column)) + Card::WIDTH };
	}
	bool TryPick(Uint32 mouse_x, Uint32 mouse_y) {
		for(size_t i = 0; i < piles.size(); ++i) {
			auto [left_bound, right_bound] = GetColumnBounds(i);
			if(mouse_x >= left_bound && mouse_x <= right_bound) {
				const auto ret = piles[i].HitTestAndSplice(mouse_x, mouse_y, floating_pile);
				if(ret)
					previous_pile = &piles[i];
				return ret;
			}
		}
		return false;
	}
	bool TryDrop(Uint32 mouse_x, Uint32 mouse_y) {
		for(size_t i = 0; i < piles.size(); ++i) {
			auto [left_bound, right_bound] = GetColumnBounds(i);
			if(mouse_x >= left_bound && mouse_x <= right_bound) {
				const auto ret = piles[i].MergePile(floating_pile);
				if(ret)
					std::exchange(previous_pile, nullptr)->MakeLastCardVisible();
				return ret;
			}
		}
		return false;
	}
	void RollBack() {
		std::exchange(previous_pile, nullptr)->MergePile(floating_pile);
	}
public:
	GameBoard() {
		auto arr = make_random_array();
		size_t pile_idx = 0;
		for(auto [suit, number] : arr) {
			piles[pile_idx].AddCard(suit, number);
			pile_idx = (pile_idx + 1) % piles.size();
		}
		for(auto& pile : piles) {
			pile.MakeLastCardVisible();
		}
	}
	template<typename T>
	void Draw(Uint32 mouse_x, Uint32 mouse_y, T&& draw_func) const {
		Uint32 x = 10;
		for(auto& pile : piles) {
			pile.DrawAt(x, 20, 50, draw_func);
			x += COLUMN_OFFSET;
		}
		const auto y_offset = floating_pile.size() > 1 ? 0 : Card::HEIGHT/2;
		floating_pile.DrawAt(mouse_x - Card::WIDTH/2, mouse_y - y_offset, 50, draw_func);
	}
	bool TryGrabFromPile(Uint32 mouse_x, Uint32 mouse_y) {
		return TryPick(mouse_x, mouse_y);
	}
	bool DropToPileOrRollBack(Uint32 mouse_x, Uint32 mouse_y) {
		if(floating_pile.empty())
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

extern "C" int main(int argc, char** argv) {
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
