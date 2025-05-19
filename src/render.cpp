#include "render.h"


void Shaders::create(const ShaderCache& cache) {
	video.create(cache.video);
}
void Shaders::link(const Render& render) {
	video.link(&render.camera);
}
void Shaders::destroy() {
	video.destroy();
}
void Render::loadResources() {
	shaderCache.video.load(shader_path::video);
}
void Render::initResources() {
	shaders.create(shaderCache);
	shaders.link(*this);
}
static void reloadShader(const char* path, ShaderSource& cache, Shader& shader) {
	cache.load(path);
	shader.destroy();
	shader.create(cache);
}
void Render::reloadShaders() {
	reloadShader(shader_path::video, shaderCache.video, shaders.video);
}
void Render::draw() {

	const auto& x = camera.viewOffset.x;
	const auto& y = camera.viewOffset.y;
	const auto& w = camera.viewSize.x;
	const auto& h = camera.viewSize.y;
	
	glViewport(x, y, w, h);
	shaders.video.enable();
	shaders.video.draw(videoMesh);
	shaders.video.disable();
}
void Render::destroy() {
	shaders.destroy();
}
