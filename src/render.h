#pragma once
#include "shader/shader.h"
#include "shader/shaderSource.h"

struct Render; //forward

struct ShaderCache {
	ShaderSource video;
};

struct Shaders {
	VideoShader video;
	void create(const ShaderCache& cache);
	void link(const Render& render);
	void destroy();
};

struct Render {
	Camera camera;
	Shaders shaders;
	ShaderCache shaderCache;
	VideoMesh videoMesh;

	void loadResources();
	void initResources();
	void reshape(int width, int height);
	void reloadShaders();
	void draw();
	void destroy();
};