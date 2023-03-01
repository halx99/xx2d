﻿#include "xx2d_pch.h"
#include "game_looper.h"
#include "s0_main_menu.h"
#include "s1_tiledmap.h"
#include "s2_dragable.h"
#include "s3_boxbutton.h"
#include "s4_physics_circles.h"
#include "s5_spacegridab.h"
#include "s6_box_circle_cd.h"
#include "s7_more_box_circle_cd.h"
#include "s8_node.h"
#include "s9_sprites.h"
#include "s10_quads.h"
#include "s11_shootgame.h"
#include "s12_shootgame_idx.h"
#include "s13_movepath.h"
#include "s14_scissor.h"
#include "s15_rendertexture.h"

namespace MainMenu {

	void Scene::Init(GameLooper* looper) {
		this->looper = looper;
		std::cout << "MainMenu::Scene::Init" << std::endl;

		meListener.Init(xx::Mbtns::Left);

		float x = 300;
		menus.emplace_back().Init(looper, { -500, x }, "1: tiledmap", 48);
		menus.emplace_back().Init(looper, { 0, x }, "2: dragable", 48);
		menus.emplace_back().Init(looper, { 500, x }, "3: box button", 48);

		x -= 150;
		menus.emplace_back().Init(looper, { -500, x }, "4: physics circles (space grid) ", 48);
		menus.emplace_back().Init(looper, { 0, x }, "5: boxs (space grid ab)", 48);
		menus.emplace_back().Init(looper, { 500, x }, "6: box & circle physics", 48);

		x -= 150;
		menus.emplace_back().Init(looper, { -500, x }, "7: more circle + box physics", 48);
		menus.emplace_back().Init(looper, { 0, x }, "8: node", 48);
		menus.emplace_back().Init(looper, { 500, x }, "9: sprites", 48);

		x -= 150;
		menus.emplace_back().Init(looper, { -500, x }, "10: quads", 48);
		menus.emplace_back().Init(looper, { 0, x }, "11: shooter game (slowly)", 48);
		menus.emplace_back().Init(looper, { 500, x }, "12: shooter game with index", 48);

		x -= 150;
		menus.emplace_back().Init(looper, { -500, x }, "13: move path", 48);
		menus.emplace_back().Init(looper, { 0, x }, "14: scissor", 48);
		menus.emplace_back().Init(looper, { 500, x }, "15: render texture", 48);

		looper->extraInfo.clear();
	}

	int Scene::Update() {

		// handle mouse event
		meListener.Update();
		auto&& iter = menus.begin();
		while (meListener.eventId && iter != menus.end()) {
			meListener.Dispatch(&*iter++);
		}

		for (auto&& m : menus) {
			m.content.Draw();
		}

		return 0;
	}

	bool Menu::HandleMouseDown(MenuMouseEventListener& L) {
		return Inside(L.downPos);
	}

	int Menu::HandleMouseMove(MenuMouseEventListener& L) {
		return 0;
	}

	void Menu::HandleMouseUp(MenuMouseEventListener& L) {
		if (Inside(xx::engine.mousePosition)) {
			if (txt.starts_with("1:"sv)) {
				looper->DelaySwitchTo<TiledMap::Scene>();
			} else if (txt.starts_with("2:"sv)) {
				looper->DelaySwitchTo<Dragable::Scene>();
			} else if (txt.starts_with("3:"sv)) {
				looper->DelaySwitchTo<BoxButton::Scene>();
			} else if (txt.starts_with("4:"sv)) {
				looper->DelaySwitchTo<PhysicsCircles::Scene>();
			} else if (txt.starts_with("5:"sv)) {
				looper->DelaySwitchTo<SpaceGridAB::Scene>();
			} else if (txt.starts_with("6:"sv)) {
				looper->DelaySwitchTo<BoxCircleCD::Scene>();
			} else if (txt.starts_with("7:"sv)) {
				looper->DelaySwitchTo<MoreBoxCircleCD::Scene>();
			} else if (txt.starts_with("8:"sv)) {
				looper->DelaySwitchTo<Node::Scene>();
			} else if (txt.starts_with("9:"sv)) {
				looper->DelaySwitchTo<Sprites::Scene>();
			} else if (txt.starts_with("10:"sv)) {
				looper->DelaySwitchTo<Quads::Scene>();
			} else if (txt.starts_with("11:"sv)) {
				looper->DelaySwitchTo<ShootGame::Looper>();
			} else if (txt.starts_with("12:"sv)) {
				looper->DelaySwitchTo<ShootGameWithIndex::Looper>();
			} else if (txt.starts_with("13:"sv)) {
				looper->DelaySwitchTo<MovePath::Looper>();
			} else if (txt.starts_with("14:"sv)) {
				looper->DelaySwitchTo<Scissor::Scene>();
			} else if (txt.starts_with("15:"sv)) {
				looper->DelaySwitchTo<RenderTextureTest::Scene>();
			} else {
				throw std::logic_error("unhandled menu");
			}
		}
	}

	void Menu::Init(GameLooper* looper, xx::XY const& pos, std::string_view const& txt_, float const& fontSize) {
		this->looper = looper;
		txt = txt_;
		content.SetText(looper->fnt1, txt, fontSize);
		content.SetPosition(pos);
		auto hw = content.size.x / 2;
		auto hh = content.size.y / 2;
		leftBottom = { pos.x - hw, pos.y - hh };
		rightTop = { pos.x + hw, pos.y + hh };
	}

	bool Menu::Inside(xx::XY const& p) {
		return p.x >= leftBottom.x && p.x <= rightTop.x
			&& p.y >= leftBottom.y && p.y <= rightTop.y;
	}

}