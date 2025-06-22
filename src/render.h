#pragma once
#include "model/frame.h"
#include "shader/shader.h"

struct Shaders {
	//todo: really need split on source + shader?
	ShaderSource videoSource;
	VideoShader videoShader;
};

struct Render {
	Shaders shaders;
	Frame frames[2];
	int selected = -1;

	void loadShaders();
	void reloadShaders();
	void destroyShaders();

	void createFrame(size_t frameIndex, int16_t width, int16_t height);
	void updateFrame(size_t frameIndex, int16_t width, int16_t height, uint8_t* pixels);
	void reshapeFrame(size_t frameIndex, int left, int top, int width, int height, int screenHeight);
	void destroyFrames();

	void draw();
	void move(int dx, int dy);
	void zoom(float value);
};