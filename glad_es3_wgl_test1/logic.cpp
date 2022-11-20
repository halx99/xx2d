﻿#include "pch.h"
#include "logic.h"

Logic::Logic() {
	v = LoadVertexShader({ R"()" });
	f = LoadFragmentShader({ R"()" });
	p = LinkProgram(v, f);
}

void Logic::Update() {
	glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}