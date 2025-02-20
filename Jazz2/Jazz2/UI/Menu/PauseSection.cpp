﻿#include "PauseSection.h"
#include "OptionsSection.h"
#include "InGameMenu.h"

namespace Jazz2::UI::Menu
{
	PauseSection::PauseSection()
		:
		_selectedIndex(0),
		_animation(0.0f)
	{
		_items[(int)Item::Resume].Name = "Resume"_s;
		_items[(int)Item::Options].Name = "Options"_s;
		_items[(int)Item::Exit].Name = "Save & Exit"_s;
	}

	void PauseSection::OnShow(IMenuContainer* root)
	{
		MenuSection::OnShow(root);

		_animation = 0.0f;
	}

	void PauseSection::OnUpdate(float timeMult)
	{
		if (_animation < 1.0f) {
			_animation = std::min(_animation + timeMult * 0.02f, 1.0f);
		}

		if (_root->ActionHit(PlayerActions::Fire)) {
			ExecuteSelected();
		} else if (_root->ActionHit(PlayerActions::Menu)) {
			if (auto ingameMenu = dynamic_cast<InGameMenu*>(_root)) {
				ingameMenu->ResumeGame();
			}
		} else if (_root->ActionHit(PlayerActions::Up)) {
			_root->PlaySfx("MenuSelect"_s, 0.5f);
			_animation = 0.0f;
			if (_selectedIndex > 0) {
				_selectedIndex--;
			} else {
				_selectedIndex = (int)Item::Count - 1;
			}
		} else if (_root->ActionHit(PlayerActions::Down)) {
			_root->PlaySfx("MenuSelect"_s, 0.5f);
			_animation = 0.0f;
			if (_selectedIndex < (int)Item::Count - 1) {
				_selectedIndex++;
			} else {
				_selectedIndex = 0;
			}
		}
	}

	void PauseSection::OnDraw(Canvas* canvas)
	{
		Vector2i viewSize = canvas->ViewSize;
		Vector2f center = Vector2f(viewSize.X * 0.5f, viewSize.Y * 0.5f * (1.0f - 0.048f * (int)Item::Count));
		int charOffset = 0;

		for (int i = 0; i < (int)Item::Count; i++) {
			_items[i].TouchY = center.Y;

			if (_selectedIndex == i) {
				float size = 0.5f + IMenuContainer::EaseOutElastic(_animation) * 0.6f;

				_root->DrawElement("MenuGlow"_s, 0, center.X, center.Y, IMenuContainer::MainLayer, Alignment::Center, Colorf(1.0f, 1.0f, 1.0f, 0.4f * size), (_items[i].Name.size() + 3) * 0.5f * size, 4.0f * size, true);

				_root->DrawStringShadow(_items[i].Name, charOffset, center.X, center.Y, IMenuContainer::FontLayer + 10,
					Alignment::Center, Font::RandomColor, size, 0.7f, 1.1f, 1.1f, 0.4f, 0.9f);
			} else {
				_root->DrawStringShadow(_items[i].Name, charOffset, center.X, center.Y, IMenuContainer::FontLayer,
					Alignment::Center, Font::DefaultColor, 0.9f);
			}

			center.Y += 34.0f + 32.0f * (1.0f - 0.15f * (int)Item::Count);
		}
	}

	void PauseSection::OnTouchEvent(const nCine::TouchEvent& event, const Vector2i& viewSize)
	{
		if (event.type == TouchEventType::Down) {
			int pointerIndex = event.findPointerIndex(event.actionIndex);
			if (pointerIndex != -1) {
				float x = event.pointers[pointerIndex].x;
				float y = event.pointers[pointerIndex].y * (float)viewSize.Y;

				for (int i = 0; i < (int)Item::Count; i++) {
					if (std::abs(x - 0.5f) < 0.22f && std::abs(y - _items[i].TouchY) < 22.0f) {
						if (_selectedIndex == i) {
							ExecuteSelected();
						} else {
							_root->PlaySfx("MenuSelect"_s, 0.5f);
							_animation = 0.0f;
							_selectedIndex = i;
						}
						break;
					}
				}
			}
		}
	}

	void PauseSection::ExecuteSelected()
	{
		_root->PlaySfx("MenuSelect"_s, 0.6f);

		switch (_selectedIndex) {
			case (int)Item::Resume:
				if (auto ingameMenu = dynamic_cast<InGameMenu*>(_root)) {
					ingameMenu->ResumeGame();
				}
				break;
			case (int)Item::Options: _root->SwitchToSection<OptionsSection>(); break;
			case (int)Item::Exit:
				if (auto ingameMenu = dynamic_cast<InGameMenu*>(_root)) {
					ingameMenu->GoToMainMenu();
				}
				break;
		}
	}
}