#include "render.h"
#include <iostream>

static void reloadShader(const char* path, ShaderSource& cache, Shader& shader) {
	cache.load(path);
	shader.destroy();
	shader.create(cache);
}
void Render::loadShaders() {
	shaders.videoSource.load(shader_path::video);
	shaders.video.create(shaders.videoSource);
}
void Render::reloadShaders() {
	reloadShader(shader_path::video, shaders.videoSource, shaders.video);
}
void Render::draw() {
	shaders.video.enable();
	shaders.video.draw(frames[0]);
	shaders.video.draw(frames[1]);
	shaders.video.disable();
}
void Render::destroy() {
	shaders.video.destroy();
}
void Render::select(const glm::vec2& cursor) {
	selected = -1;

	int index = 0;
	for (auto& item : frames) {
		bool hit = item.hit(cursor.x, cursor.y);
		if (hit) {
			selected = index;
			break;
		}
		index++;
	}

	if (selected != -1) {
		auto& frame = frames[selected];
		auto point = frame.toSceneSpace(cursor);
		std::cout << point.x << " " << point.y << std::endl;
	}
}

void Render::move(int dx, int dy) {
	if (selected > -1) {
		auto& frame = frames[selected];
		frame.cam.move(dx, -dy);
	}
}
void Render::zoom(float value) {
	if (selected > -1) {
		auto& frame = frames[selected];
		frame.cam.zoom(value * 0.1f);
	}
}
void Render::createFrame(size_t frameIndex, GLuint textureId, int width, int height) {
	
	auto& frame = frames[frameIndex];
	frame.textureId = textureId;
	frame.mesh = Mesh::createImageMesh(width, height);
	frame.cam.init({ -width / 2, -height / 2 }, 1.f);

}
void Render::destroyFrame() { /* TODO: not implemented */

}
