#include "render.h"
#include <iostream>

static void reloadShader(const char* path, ShaderSource& cache, Shader& shader) {
	cache.load(path);
	shader.destroy();
	shader.create(cache);
}
static GLuint createTexture(int16_t width, int16_t height) {
	GLuint videoTextureId = 0;
	glGenTextures(1, &videoTextureId);
	glBindTexture(GL_TEXTURE_2D, videoTextureId);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, // Target
		0,						// Mip-level
		GL_RGBA,			    // Texture format
		width,                  // Texture width
		height,		            // Texture height
		0,						// Border width
		GL_RGB,			        // Source format
		GL_UNSIGNED_BYTE,		// Source data type
		nullptr);               // Source data pointer
	glBindTexture(GL_TEXTURE_2D, 0);
	return videoTextureId;
}
static void updateTexture(GLuint textureId, int16_t width, int16_t height, uint8_t* pixels) {
	//todo: Probably better to use PBO for streaming data
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexSubImage2D(GL_TEXTURE_2D,	// Target
		0,							// Mip-level
		0,							// X-offset
		0,							// Y-offset
		width,						// Texture width
		height,						// Texture height
		GL_RGB,						// Source format
		GL_UNSIGNED_BYTE,			// Source data type
		pixels);					// Source data pointer
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Render::loadShaders() {
	shaders.videoSource.load(shader_path::video);
	shaders.videoShader.create(shaders.videoSource);
}
void Render::reloadShaders() {
	reloadShader(shader_path::video, shaders.videoSource, shaders.videoShader);
}
void Render::destroyShaders() {
	shaders.videoShader.destroy();
}
void Render::createFrame(size_t frameIndex, int16_t width, int16_t height) {
	auto& frame = frames[frameIndex];
	frame.textureId = createTexture(width, height);
	frame.mesh = Mesh::createImageMesh(width, height);
	frame.cam.init({ -width / 2, -height / 2 }, 1.f);
}
void Render::updateFrame(size_t frameIndex, int16_t width, int16_t height, uint8_t* pixels) {
	const auto& frame = frames[frameIndex];
	updateTexture(frame.textureId, width, height, pixels);
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
	shaders.videoShader.enable();
	shaders.videoShader.draw(frames[0]);
	//shaders.videoShader.draw(frames[1]);
	shaders.videoShader.disable();
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
