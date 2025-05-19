/*
 * Copyright (c) 2022, Edoardo Lolletti <edoardo762@gmail.com>
 *
 * SPDX-License-Identifier: Zlib
 */
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <stdexcept> //std::runtime_error

#include "GameWindow.h"

GameWindow::GameWindow() {
	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
	if(!SDL_CreateWindowAndRenderer("Solitaire", 1000, 670, SDL_WINDOW_RESIZABLE, &m_window, &m_renderer)) {
		throw std::runtime_error("Failed to create window and renderer");
	}
	SDL_SetRenderLogicalPresentation(m_renderer, 1000, 670, SDL_LOGICAL_PRESENTATION_LETTERBOX);
	m_field_texture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 1000, 670);
	m_cards_texture = LoadSpriteTexture("./cards.png");
	if(!m_cards_texture) {
		SDL_DestroyRenderer(m_renderer);
		SDL_DestroyWindow(m_window);
		throw std::runtime_error(SDL_GetError());
	}
}
GameWindow::~GameWindow() {
	SDL_DestroyTexture(m_cards_texture);
	SDL_DestroyTexture(m_field_texture);
	SDL_DestroyRenderer(m_renderer);
	SDL_DestroyWindow(m_window);
}

SDL_Texture* GameWindow::LoadSpriteTexture(const char* path) const {
	auto* tmp = IMG_Load(path);
	if(!tmp) return nullptr;
	auto* sprite = SDL_CreateTextureFromSurface(m_renderer, tmp);
	SDL_DestroySurface(tmp);
	return sprite;
}

void GameWindow::DrawBoard() {
	if(m_redraw) {
		m_redraw = false;
		SDL_SetRenderTarget(m_renderer, m_field_texture);
		SDL_SetRenderDrawColor(m_renderer, 0, 255, 0, 255);
		SDL_RenderClear(m_renderer);
		m_board.Draw(m_mouse_x, m_mouse_y, [&](const auto& rect, const auto& dest_rect) {
			SDL_RenderTexture(m_renderer, m_cards_texture, &rect, &dest_rect);
		});
		SDL_SetRenderTarget(m_renderer, nullptr);
	}
	SDL_RenderTexture(m_renderer, m_field_texture, nullptr, nullptr);
	SDL_RenderPresent(m_renderer);
}

int GameWindow::onEvent(const SDL_Event& e) {
	bool redraw = false;
	SDL_Event transformed;
	switch(e.type) {
	case SDL_EVENT_QUIT:
		return SDL_APP_SUCCESS;
	case SDL_EVENT_WINDOW_RESIZED:
	case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
	case SDL_EVENT_WINDOW_SHOWN:
	case SDL_EVENT_WINDOW_EXPOSED:
		redraw = true;
		break;
	case SDL_EVENT_WINDOW_FOCUS_LOST:
		transformed = e;
		SDL_ConvertEventToRenderCoordinates(m_renderer, &transformed);
		redraw = m_board.DropToPileOrRollBack(m_mouse_x, m_mouse_y);
		break;
	case SDL_EVENT_MOUSE_MOTION:
		transformed = e;
		SDL_ConvertEventToRenderCoordinates(m_renderer, &transformed);
		m_mouse_x = transformed.motion.x;
		m_mouse_y = transformed.motion.y;
		redraw = true;
		break;
	case SDL_EVENT_MOUSE_BUTTON_DOWN:
		transformed = e;
		SDL_ConvertEventToRenderCoordinates(m_renderer, &transformed);
		m_mouse_x = transformed.button.x;
		m_mouse_y = transformed.button.y;
		redraw = transformed.button.button == 1 && m_board.TryGrabFromPile(m_mouse_x, m_mouse_y);
		break;
	case SDL_EVENT_MOUSE_BUTTON_UP:
		transformed = e;
		SDL_ConvertEventToRenderCoordinates(m_renderer, &transformed);
		m_mouse_x = transformed.button.x;
		m_mouse_y = transformed.button.y;
		redraw = transformed.button.button == 1 && m_board.DropToPileOrRollBack(m_mouse_x, m_mouse_y);
		break;
	}
	m_redraw = m_redraw || redraw;
	return SDL_APP_CONTINUE;
}
