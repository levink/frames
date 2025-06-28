#include "render.h"
#include "resources.h"


void Render::loadShaders() {
	videoShader.create(resources::videoShader);
}
void Render::reloadShaders() {
	videoShader.destroy();
	videoShader.create(resources::videoShader);
}
void Render::destroyShaders() {
	videoShader.destroy();
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
	videoShader.enable();
	videoShader.draw(frames[0]);
	//videoShader.draw(frames[1]);
	videoShader.disable();
}
