#pragma once
#include "shader/shader.h"

struct Render; //forward

struct Shaders {
	ShaderSource videoSource;
	VideoShader video;
};

struct VideoFrame {
	Camera cam;
	Mesh mesh;
	GLuint textureId;
};

struct Render {
	Shaders shaders;
	VideoFrame frame[2];
	int selected = -1;

	void loadShaders();
	void reloadShaders();
	void reshape(int x, int y, int w, int h);
	void draw();
	void destroy();

	void select(const glm::vec2& cursor);
	void move(const glm::vec2& delta);
	void zoom(float value);
};