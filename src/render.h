#pragma once
#include "shader/shader.h"
#include "ui/dragdrop.h"

struct Render; //forward

struct Shaders {
	ShaderSource videoSource;
	VideoShader video;
};

struct Render {
	Shaders shaders;
	DragDrop drag;
	Camera camera;
	Mesh mesh;

	void loadResources();
	void initResources();
	//void reshape(int width, int height);
	void reloadShaders();
	void draw();
	void destroy();
};