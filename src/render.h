#pragma once
#include "model/frame.h"
#include "shader/shader.h"

struct Shaders {
	ShaderSource videoSource;
	VideoShader video;
};

struct Render {
	Shaders shaders;
	FrameView frame[2];
	int selected = -1;

	void loadShaders();
	void reloadShaders();
	void draw();
	void destroy();

	void select(const glm::vec2& cursor);
	void move(const glm::vec2& delta);
	void zoom(float value);

	void createFrame(size_t frameIndex, GLuint textureId, int imageWidth, int imageHeight);
	void destroyFrame() { /*TODO: not implemented*/ }
};