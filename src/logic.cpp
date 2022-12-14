﻿#include "pch.h"
#include "logic.h"

void Logic::Init() {
	rnd.SetSeed();

	size_t numSprites = 100'000;

	auto t1 = TextureCacheLoad("res/zazaka.pkm"sv);
	auto t2 = TextureCacheLoad("res/mouse.pkm"sv);

	ss.resize(numSprites);
	for (size_t i = 0; i < numSprites; i++) {
		auto& s = ss[i];
		s.SetTexture(/*rnd.Get()*/i % 2 == 0 ? t1 : t2);
		//s.SetTexture(t1);
		s.SetScale({ 1, 1 });
		auto c = rnd.Get(); auto cp = (uint8_t*)&c;
		s.SetColor({ cp[0], cp[1], cp[2], 255 });
		s.SetPositon({ float(rnd.Next(w) - w / 2), float(rnd.Next(h) - h / 2) });
	}

	{
		BMFont bmf;
		bmf.Load(this, "res/basechars.fnt"sv);
		title.SetText(bmf, 32, "asdfqwer");
		//title.SetScale({ 1, 1 });
		title.SetColor({ 255, 255, 255, 255 });
		title.SetPositon({ 0, float(h) / 2 - 32 / 2 - 8 });
	}
}

void Logic::Update(float delta) {
	//for (auto& s : ss) {
	//	s.SetPositon({ float(rnd.Next(w) - w / 2), float(rnd.Next(h) - h / 2) });
	//	s.Draw(this);
	//}
	title.Draw(this);
}
