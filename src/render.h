#pragma once
#include "model/frame.h"
#include "shader/shader.h"

struct Render {
	VideoShader videoShader;
	FrameRender frames[2];
	void loadShaders();
	void reloadShaders();
	void destroyShaders();
	void destroyFrames();
	void draw();
};