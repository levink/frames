#include <iostream>
#include "frame.h"
#include "shader/shader.h"
#include "util/math.h"

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
	static void setViewport(const Viewport& vp) {
		glViewport(vp.left, vp.bottom, vp.width, vp.height);
	}
}

void FrameRender::create(int16_t width, int16_t height) {
    if (textureId) {
        glDeleteTextures(1, &textureId);
    }
	textureReady = false;
	textureId = gl::createTexture(width, height);
    image = ImageMesh::createImageMesh(width, height);
    cam.init({ -width / 2, -height / 2 }, 1.f);
}
void FrameRender::update(int16_t width, int16_t height, const uint8_t* pixels) {
	gl::updateTexture(textureId, width, height, pixels);
	textureReady = true;
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
void FrameRender::draw(ShaderContext& shaders) const {
	gl::setViewport(vp);

	shaders.video.enable();
	shaders.video.draw(*this);
	shaders.video.disable();

	shaders.lines.enable();
	shaders.lines.draw(*this);
	shaders.lines.disable();

	shaders.point.enable();
	shaders.point.draw(cam, line.controlPoints);
	shaders.point.disable();
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
	//lineMesh.addPoint(scene.x, scene.y, r);
	cout << "addPoint x = " << scene.x << " y=" << scene.y << std::endl;
}
void FrameRender::addSegmentPoint(int mx, int my, float r) {
	auto point = toSceneSpace(mx, my);
	line.controlPoints.reserve(1000);
	if (line.controlPoints.size() > 1000) {
		return;
	}

	auto& points = line.controlPoints;
	if (points.empty()) {
		points.emplace_back(point);
		return;
	}

	float d2 = math::dist2(points.back(), point);
	if (d2 < 4 * r * r) {
		return;
	}

	points.emplace_back(point);
	if (points.size() == 2) {
		const auto& p0 = points[points.size() - 2];
		const auto& p1 = points[points.size() - 1];
		const auto dir = glm::normalize(p1 - p0);
		const auto n = r * glm::vec2(-dir.y, dir.x);

		auto& vertex = line.mesh.vertex;
		vertex.emplace_back(LineVertex{ p0 - n, p0, p1, r });
		vertex.emplace_back(LineVertex{ p0 + n, p0, p1, r });
		vertex.emplace_back(LineVertex{ p1 - n, p0, p1, r });
		vertex.emplace_back(LineVertex{ p1 + n, p0, p1, r });

		auto& face = line.mesh.face;
		const uint16_t i0 = vertex.size() - 4;
		const uint16_t i1 = i0 + 1;
		const uint16_t i2 = i0 + 2;
		const uint16_t i3 = i0 + 3;
		face.emplace_back(i0, i1, i2);
		face.emplace_back(i2, i1, i3);
		return;
	}

	const auto& p0 = points[points.size() - 3];
	const auto& p1 = points[points.size() - 2];
	const auto& p2 = points[points.size() - 1];
	const auto dir1 = glm::normalize(p2 - p0);
	const auto n1 = glm::vec2(-dir1.y, dir1.x);

	auto& vertex = line.mesh.vertex;
	auto& prev1 = vertex[vertex.size() - 2];
	auto& prev2 = vertex[vertex.size() - 1];
	prev1.position = prev1.end - prev1.radius * n1;
	prev2.position = prev2.end + prev2.radius * n1;

	const auto dir2 = glm::normalize(p2 - p1);
	const auto n2 = glm::vec2(-dir2.y, dir2.x);
	vertex.emplace_back(LineVertex{ p1 - n1 * r, p1, p2, r });
	vertex.emplace_back(LineVertex{ p1 + n1 * r, p1, p2, r });
	vertex.emplace_back(LineVertex{ p2 - n2 * r, p1, p2, r });
	vertex.emplace_back(LineVertex{ p2 + n2 * r, p1, p2, r });
	
	auto& face = line.mesh.face;
	const uint16_t i0 = vertex.size() - 4;
	const uint16_t i1 = i0 + 1;
	const uint16_t i2 = i0 + 2;
	const uint16_t i3 = i0 + 3;
	face.emplace_back(i0, i1, i2);
	face.emplace_back(i2, i1, i3);
}
