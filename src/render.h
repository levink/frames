#pragma once
#include "model/frame.h"
#include "shader/shader.h"

struct Render {
	VideoShader videoShader;
	Frame frames[2];
	int selected = -1;

	void loadShaders();
	void reloadShaders();
	void destroyShaders();
	void destroyFrames();

	void draw();
	void move(int dx, int dy);
	void zoom(float value);
};