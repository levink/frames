#include "render.h"
#include <iostream>

void Render::loadResources() {
	shaders.videoSource.load(shader_path::video);
}
void Render::initResources() {
	shaders.video.create(shaders.videoSource);
}
static void reloadShader(const char* path, ShaderSource& cache, Shader& shader) {
	cache.load(path);
	shader.destroy();
	shader.create(cache);
}
void Render::reloadShaders() {
	reloadShader(shader_path::video, shaders.videoSource, shaders.video);
}
void Render::reshape(int x, int y, int w, int h) {
	int width = 0.5f * w;
	frame[0].cam.reshape(x, y, width, h);
	frame[1].cam.reshape(x + width, y, w - width, h);
}
void Render::draw() {
	shaders.video.enable();
	shaders.video.draw(frame[0].cam, frame[0].mesh, frame[0].textureId);
	shaders.video.draw(frame[1].cam, frame[1].mesh, frame[1].textureId);
	shaders.video.disable();
}
void Render::destroy() {
	shaders.video.destroy();
}
void Render::select(const glm::vec2& cursor) {
	selected = -1;

	int index = 0;
	for (auto& item : frame) {
		bool hit = item.cam.vp.hit(cursor.x, cursor.y);
		if (hit) {
			selected = index;
			break;
		}
		index++;
	}

	/*std::cout << cursor.y << std::endl;*/
}
void Render::move(const glm::vec2& delta) {
	if (selected > -1) {
		frame[selected].mesh.move(delta.x, delta.y);
	}
}
void Render::zoom(float value) {
	if (selected > -1) {
		frame[selected].mesh.zoom(value * 0.1);
	}
}
