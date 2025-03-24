#include "render.h"

void ShaderCache::load() {
	video.load(shader_files::video);
}

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
	shaderCache.load();
}
void Render::initResources() {
	shaders.create(shaderCache);
	shaders.link(*this);
}
void Render::reshape(int width, int height) {
	camera.reshape(width, height);
}
void Render::reloadShaders() {
	//todo: finish this shit
}
void Render::draw() {
	shaders.video.enable();
	shaders.video.draw(videoMesh);
	shaders.video.disable();
}
void Render::destroy() {
	shaders.destroy();
}
