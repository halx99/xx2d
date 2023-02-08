﻿#pragma once
#include "pch.h"
#include "logic_base.h"

struct Logic : xx::Engine {
	xx::BMFont fnt1;
	xx::Label lbInfo;
	std::string extraInfo;
	int fps{}, fpsCounter{};
	double nowSecs{}, delta{}, fpsTimePool{}, timePool{};

	xx::Shared<LogicBase> lg;

	template<typename LT>
	void DelaySwitchTo() {
		DelayExecute([this] {
			lg = xx::Make<LT>();
			lg->Init(this);
		});
	}

	void Init();
	int Update();
};
