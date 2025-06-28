#include "render.h"
#include "resources.h"


void Render::createShaders() {
	videoShader.create(resources::videoShader);
	circleShader.create(resources::circleShader);
}
void Render::reloadShaders() {
	destroyShaders();
	createShaders();
}
void Render::destroyShaders() {
	videoShader.destroy();
	circleShader.destroy();
}
void Render::destroyFrames() {
	for (auto& frame : frames) {
		if (frame.textureId) {
			glDeleteTextures(1, &frame.textureId);
			frame.textureId = 0;
		}
	}
}
static void setViewport(const Viewport& vp) {
	glViewport(vp.left, vp.bottom, vp.width, vp.height);
}
void Render::draw() {

	setViewport(frames[0].vp);

	videoShader.enable();
	videoShader.draw(frames[0]);
	videoShader.disable();

	circleShader.enable();
	circleShader.draw(frames[0]);
	circleShader.disable();
}
