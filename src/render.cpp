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
	for (auto& frame : frames) {
		if (frame.textureId) {
			glDeleteTextures(1, &frame.textureId);
			frame.textureId = 0;
		}
	}
}
void Render::draw() {
	frames[0].draw(shaders);
}
