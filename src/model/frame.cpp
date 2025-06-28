#include <iostream>
#include "frame.h"

using std::cout;

namespace gl {
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
	static void updateTexture(GLuint textureId, int16_t width, int16_t height, const uint8_t* pixels) {
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
}

void FrameRender::create(int16_t width, int16_t height) {
    if (textureId) {
        glDeleteTextures(1, &textureId);
    }
    textureId = gl::createTexture(width, height);
    mesh = Mesh::createImageMesh(width, height);
    cam.init({ -width / 2, -height / 2 }, 1.f);
}
void FrameRender::update(int16_t width, int16_t height, const uint8_t* pixels) {
	gl::updateTexture(textureId, width, height, pixels);
}
void FrameRender::reshape(int left, int top, int width, int height, int screenHeight) {
	vp.left = left;
	vp.bottom = screenHeight - (top + height);
	vp.width = width;
	vp.height = height;
    cam.reshape(width, height);
}
void FrameRender::move(int dx, int dy) {
	cam.move(dx, -dy);
}
void FrameRender::zoom(float units) {
	cam.zoom(units * 0.1f);
}
glm::vec2 FrameRender::toOpenGLSpace(int x, int y) const {
    /*
       Converting
       from view space x = [0, frameWidth], y = [0, frameHeight]	- coordinates from top left corner of view
       to OpenGL space x = [-1, 1], y = [-1, 1]						- coordinates from bottom left corner of view
    */
    auto result = glm::vec2(
        ((2.f * x) / vp.width) - 1.f,
        1.f - ((2.f * y) / vp.height)
    );
    return result;
}
glm::vec2 FrameRender::toSceneSpace(int x, int y) const {
    auto point2D = toOpenGLSpace(x, y);	// x=[-1, 1], y=[-1, 1]
    auto point4D = cam.pv_inverse * glm::vec4(point2D.x, point2D.y, 0.5f, 1.f);
	return { point4D.x / point4D.w, point4D.y / point4D.w };
}

void FrameRender::addPoint(int x, int y, float r) {
	auto scene = toSceneSpace(x, y);
	circles.add(scene.x, scene.y, r);
	cout << "addPoint x = " << scene.x << " y=" << scene.y << std::endl;
}