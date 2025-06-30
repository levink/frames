#pragma once
#include "model/frame.h"
#include "shader/shader.h"

struct Render {
	ShaderContext shaders;
	FrameRender frames[2];
	void createShaders();
	void reloadShaders();
	void destroyShaders();
	void destroyFrames();
	void draw();
};