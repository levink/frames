#include <iostream>
#include "frame.h"
#include "shader/shader.h"
#include "util/math.h"

using std::cout;
using std::endl;

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

	/*shaders.point.enable();
	for (const auto& line : lines) {
		shaders.point.draw(cam, line.points);
	}
	shaders.point.disable();*/
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


Line::Line(float width) : width(width), radius(0.5f * width) { }
void Line::addPoint(glm::vec2 point) {
	size_t capacity = points.empty() ? 4 : 1000;
	points.reserve(capacity);

	if (points.empty()) {
		points.emplace_back(point);
		mesh.addQuad(point, radius);
		return;
	}

	if (math::distance2(points.back(), point) < 0.5 * radius * radius) {
		return;
	}

	if (points.size() == 1) {
		points.emplace_back(point);
		const auto& p0 = points[0];
		const auto& p1 = points[1];

		drawDir = glm::normalize(p1 - p0);
		const auto dir = radius * drawDir;
		const auto n = glm::vec2(-dir.y, dir.x);
	
		mesh.vertex[0] = LineVertex{ p0 - dir - n, p0, p1, radius };
		mesh.vertex[1] = LineVertex{ p0 - dir + n, p0, p1, radius };
		mesh.vertex[2] = LineVertex{ p1 + dir - n, p0, p1, radius };
		mesh.vertex[3] = LineVertex{ p1 + dir + n, p0, p1, radius };

		return;
	}

	{
		//make smooth line
		const auto& back = points.back();
		const auto dist = glm::length(point - back);
		const auto pointDir = glm::normalize(point - back);
		auto cos = glm::dot(pointDir, drawDir);
		if (cos < 0.75f) {
			points.emplace_back(back);
			mesh.addQuad(back, radius);
			drawDir = pointDir;
		}
		else {
			constexpr float K = 0.5;
			drawDir = K * drawDir + (1.f - K) * pointDir;
			point = back + dist * drawDir;
		}
	}
	
	points.emplace_back(point);
	const auto& p0 = points[points.size() - 3];
	const auto& p1 = points[points.size() - 2];
	const auto& p2 = points[points.size() - 1];

	const auto dir1 = radius * glm::normalize(p2 - p0);
	const auto n1 = glm::vec2(-dir1.y, dir1.x);
	auto& vertex = mesh.vertex;
	auto& prev1 = vertex[vertex.size() - 2];
	auto& prev2 = vertex[vertex.size() - 1];
	prev1.position = prev1.end - n1;
	prev2.position = prev2.end + n1;

	const auto dir2 = radius * glm::normalize(p2 - p1);
	const auto n2 = glm::vec2(-dir2.y, dir2.x);
	vertex.emplace_back(LineVertex{ p1 - n1,		p1, p2, radius });
	vertex.emplace_back(LineVertex{ p1 + n1,		p1, p2, radius });
	vertex.emplace_back(LineVertex{ p2 - n2 + dir2, p1, p2, radius });
	vertex.emplace_back(LineVertex{ p2 + n2 + dir2, p1, p2, radius });

	auto& face = mesh.face;
	const uint16_t i0 = vertex.size() - 4;
	const uint16_t i1 = i0 + 1;
	const uint16_t i2 = i0 + 2;
	const uint16_t i3 = i0 + 3;
	face.emplace_back(i0, i1, i2);
	face.emplace_back(i2, i1, i3);
}
void FrameRender::newLine(int x, int y, float width) {
	auto point = toSceneSpace(x, y);
	auto& line = lines.emplace_back(width);
	line.addPoint(point);
}
void FrameRender::addPoint(int x, int y) {
	if (lines.empty()) {
		return;
	}
	auto point = toSceneSpace(x, y);
	auto& line = lines.back();
	line.addPoint(point);
}
