#include "render.h"
#include "resources.h"


void Render::createShaders() {
	shaders.video.create(resources::videoShader);
	shaders.lines.create(resources::linesShader);
	shaders.point.create(resources::pointShader);
}
void Render::reloadShaders() {
	destroyShaders();
	createShaders();
}
void Render::destroyShaders() {
	shaders.video.destroy();
	shaders.lines.destroy();
	shaders.point.destroy();
}
void Render::destroyFrames() {
	frames[0].destroyTexture();
	frames[1].destroyTexture();
}
void Render::createFrameBuffers() {
	frames[0].fb.create(1, 1);
	frames[1].fb.create(1, 1);
}
void Render::destroyFrameBuffers() {
	frames[0].fb.destroy();
	frames[1].fb.destroy();
}
void Render::render() {
	frames[0].render(shaders);
	frames[1].render(shaders);
}
