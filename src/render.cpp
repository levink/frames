#include "render.h"
#include <iostream>


void Render::loadShaders() {
	ShaderSource source;
	source.load(shader_path::video);
	videoShader.create(source);
}
void Render::reloadShaders() {
	ShaderSource source;
	source.load(shader_path::video);
	videoShader.destroy();
	videoShader.create(source);
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
