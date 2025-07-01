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
	shaders.video.draw(*this); //todo: render()?
	shaders.video.disable();

	shaders.lines.enable();
	shaders.lines.draw(*this); //todo: render()?
	shaders.lines.disable();

	//shaders.point.enable();
	//for (const auto& line : lines) {
	//	shaders.point.draw(cam, line.points); //todo: render()?
	//}
	//shaders.point.disable();
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

//void Line::addNextPoint(const glm::vec2& position) {
//	points.emplace_back(position);
//	const auto& p0 = points[points.size() - 3];
//	const auto& p1 = points[points.size() - 2];
//	const auto& p2 = points[points.size() - 1];
//
//	const auto dir1 = radius * glm::normalize(p2 - p0);
//	const auto n1 = glm::vec2(-dir1.y, dir1.x);
//	auto& vertex = mesh.vertex;
//	auto& prev1 = vertex[vertex.size() - 2];
//	auto& prev2 = vertex[vertex.size() - 1];
//	prev1.position = prev1.end - n1;
//	prev2.position = prev2.end + n1;
//
//	const auto dir2 = radius * glm::normalize(p2 - p1);
//	const auto n2 = glm::vec2(-dir2.y, dir2.x);
//	vertex.emplace_back(LineVertex{ p1 - n1,		p1, p2, radius });
//	vertex.emplace_back(LineVertex{ p1 + n1,		p1, p2, radius });
//	vertex.emplace_back(LineVertex{ p2 - n2 + dir2, p1, p2, radius });
//	vertex.emplace_back(LineVertex{ p2 + n2 + dir2, p1, p2, radius });
//
//	auto& face = mesh.face;
//	const uint16_t i0 = vertex.size() - 4;
//	const uint16_t i1 = i0 + 1;
//	const uint16_t i2 = i0 + 2;
//	const uint16_t i3 = i0 + 3;
//	face.emplace_back(i0, i1, i2);
//	face.emplace_back(i2, i1, i3);
//}

Line::Line(float width) : width(width), radius(0.5f * width) { }
void Line::moveLastPoint(const glm::vec2& position) {

	points.back() = position;
	const auto& p0 = points[points.size() - 2];
	const auto& p1 = points[points.size() - 1];

	auto index0 = mesh.vertex.size() - 4;
	auto& v0 = mesh.vertex[index0];
	auto& v1 = mesh.vertex[index0 + 1];
	auto& v2 = mesh.vertex[index0 + 2];
	auto& v3 = mesh.vertex[index0 + 3];
	if (math::distanceL1(p0, p1) < 2.f) {
		v2 = LineVertex{ p0 + (v1.segmentStart - v1.position), p0, p0, radius };
		v3 = LineVertex{ p0 + (v0.segmentStart - v0.position), p0, p0, radius };
	}
	else if (points.size() == 2) {
		auto dir = glm::normalize(p1 - p0);
		auto norm = glm::vec2(-dir.y, dir.x);
		v0 = LineVertex{ p0 + radius * (-dir - norm), p0, p1, radius };
		v1 = LineVertex{ p0 + radius * (-dir + norm), p0, p1, radius };
		v2 = LineVertex{ p1 + radius * (dir - norm), p0, p1, radius };
		v3 = LineVertex{ p1 + radius * (dir + norm), p0, p1, radius };
	}
	else {
		auto dir = glm::normalize(p1 - p0);
		auto norm = glm::vec2(-dir.y, dir.x);
		v0 = LineVertex{ p0 - radius * norm, p0, p1, radius }; //without dir offset
		v1 = LineVertex{ p0 + radius * norm, p0, p1, radius }; //without dir offset
		v2 = LineVertex{ p1 + radius * (dir - norm), p0, p1, radius };
		v3 = LineVertex{ p1 + radius * (dir + norm), p0, p1, radius };

		/*const auto& prevPoint = points[points.size() - 3];
		const auto prevDir = glm::normalize(p0 - prevPoint);
		const auto cos = glm::dot(dir, prevDir);
		auto& prev0 = mesh.vertex[index0 - 2];
		auto& prev1 = mesh.vertex[index0 - 1];
		if (cos > 0.9f) {
			prev0.position = p0 - radius * norm;
			prev1.position = p0 + radius * norm;
		} else {
			const auto prevNorm = glm::vec2(-prevDir.y, prevDir.x);
			prev0.position = prev0.end + radius * (prevDir - prevNorm);
			prev1.position = prev1.end + radius * (prevDir + prevNorm);
		}*/
	}
}
void Line::addNextPoint(const glm::vec2& position) {
	points.emplace_back(position);
	
	auto& vertex = mesh.vertex;
	vertex.emplace_back();
	vertex.emplace_back();
	vertex.emplace_back();
	vertex.emplace_back();

	auto& face = mesh.face;
	const uint16_t i0 = vertex.size() - 4;
	const uint16_t i1 = i0 + 1;
	const uint16_t i2 = i0 + 2;
	const uint16_t i3 = i0 + 3;
	face.emplace_back(i0, i1, i2);
	face.emplace_back(i2, i1, i3);
}
void Line::drawSmooth(glm::vec2 position) {
	size_t capacity = points.empty() ? 2 : 1000;
	points.reserve(capacity);

	if (points.empty()) {
		points.emplace_back(position);
		points.emplace_back(position);
		mesh.addQuad(position, radius);
		return;
	}
	
	const auto& p0 = points[points.size() - 2];
	const auto& p1 = points[points.size() - 1];
	if (math::distanceL1(p0, p1) < radius) {
		moveLastPoint(position);
		return;
	}

	//{
	//	//make line more smooth
	//	const auto& back = points.back();
	//	const auto dist = glm::length(position - back);
	//	const auto pointDir = glm::normalize(position - back);
	//	constexpr float K = 0.75;
	//	drawDir = K * drawDir + (1.f - K) * pointDir;
	//	position = back + dist * drawDir;
	//}
	addNextPoint(position);
	moveLastPoint(position);
	
}
void FrameRender::newLine(int x, int y, float width) {
	auto point = toSceneSpace(x, y);
	auto& line = lines.emplace_back(width);
	line.drawSmooth(point);
}
void FrameRender::addPoint(int x, int y) {
	if (lines.empty()) {
		return;
	}
	auto point = toSceneSpace(x, y);
	auto& line = lines.back();
	line.drawSmooth(point);
}
