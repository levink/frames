#include "render.h"


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
void Render::draw() {
	const ViewPort& vp = camera.vp;
	glViewport(vp.x, vp.y, vp.width, vp.height);

	shaders.video.enable();
	shaders.video.draw(camera, mesh);
	shaders.video.disable();
}
void Render::destroy() {
	shaders.video.destroy();
}
