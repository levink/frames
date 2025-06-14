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
	shaders.video.draw(frame[0]);
	shaders.video.draw(frame[1]);
	shaders.video.disable();
}
void Render::destroy() {
	shaders.video.destroy();
}
void Render::select(const glm::vec2& cursor) {
	selected = -1;

	int index = 0;
	for (auto& item : frame) {
		bool hit = item.hit(cursor.x, cursor.y);
		if (hit) {
			selected = index;
			break;
		}
		index++;
	}

	if (selected != -1) {
		auto& f = frame[selected];
		auto point2D = f.toOpenGLSpace(cursor);
		auto point4D = glm::inverse(f.cam.ortho * f.mesh.modelMatrix) * glm::vec4(point2D.x, point2D.y, 0.5f, 1.f);
		auto point3D = glm::vec3(point4D / point4D.w);
		std::cout << point3D.x << " " << point3D.y << std::endl;
	}
}
void Render::move(const glm::vec2& delta) {
	if (selected > -1) {
		frame[selected].mesh.move(delta.x, -delta.y);
	}
}
void Render::zoom(float value) {
	if (selected > -1) {
		frame[selected].mesh.zoom(value * 0.1);
	}
}
void Render::createFrame(size_t frameIndex, GLuint textureId, int imageWidth, int imageHeight) {
	frame[frameIndex].textureId = textureId;
	frame[frameIndex].mesh.setSize(imageWidth, imageHeight);
	//todo: probably scale here
	//todo: implement destroyFrame()
}
