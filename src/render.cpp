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
void Render::reshape(int width, int height) {
	camera.reshape(width, height);
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
	shaders.video.enable();
	shaders.video.draw(videoMesh);
	shaders.video.disable();
}
void Render::destroy() {
	shaders.destroy();
}
