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
    imageMesh = ImageMesh::createImageMesh(width, height);
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
void FrameRender::render(ShaderContext& shaders) const {
	gl::setViewport(vp);

	shaders.video.enable();
	shaders.video.render(*this);
	shaders.video.disable();

	shaders.lines.enable();
	shaders.lines.render(*this);
	shaders.lines.disable();

	shaders.point.enable();
	for (const auto& line : lines) {
		//shaders.point.render(cam, line.points);
	}
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
void FrameRender::setLineWidth(float width) {
	lineWidth = width;
}
void FrameRender::setLineColor(float r, float g, float b) {
	frontColor[0] = r;
	frontColor[1] = g;
	frontColor[2] = b;
	lineMesh.color = frontColor;
}

void FrameRender::mouseClick(int x, int y) {
	const auto point = toSceneSpace(x, y);
	const auto radius = 0.5f * lineWidth * cam.scale_inverse;
	const auto meshOffset = lineMesh.vertex.size();
	Line& line = lines.emplace_back(Line(radius, meshOffset));
	line.addPoint(point, lineMesh);
	line.updateMesh(lineMesh);
}
void FrameRender::mouseDrag(int x, int y) {
	if (lines.empty()) {
		return;
	}

	auto point = toSceneSpace(x, y);
	auto& line = lines.back();

	if (line.points.size() < 2) {
		line.addPoint(point, lineMesh);
	} else {
		//line.addPoint(point, lineMesh);
		line.moveLast(point);
	}
	line.updateMesh(lineMesh);
	
}
void FrameRender::mouseStop(int x, int y) {
	//todo: close line here?
}
void FrameRender::clearDrawn() {
	lines.clear();
	lineMesh.clear();
}

static glm::vec2 dirOrDefault(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& defVal) {
	constexpr float eps = 1.f;
	if (math::distanceL1(p1, p2) < eps) {
		return defVal;
	}
	return math::dir(p1, p2);
}
Segment::Segment(const glm::vec2& p1, const glm::vec2& p2) :
	p1(p1),
	p2(p2) {
	dir = dirOrDefault(p1, p2, { 0,1 });
	n1 = { -dir.y, dir.x };
	n2 = { -dir.y, dir.x };
	openLeft = true;
	openRight = true;
}
void Segment::moveP2(const glm::vec2& pos) {
	p2 = pos;
	dir = dirOrDefault(p1, p2, { 0,1 });
	n1 = { -dir.y, dir.x };
	n2 = { -dir.y, dir.x };
}



Line::Line(float radius, size_t meshOffset) :
	radius(radius),
	meshOffset(meshOffset) { }
void Line::addPoint(const glm::vec2& pos, LineMesh& mesh) {
	if (points.size() > 1) {
		Segment segment(points.back(), pos);
		points.emplace_back(pos);
		segments.emplace_back(segment);
		mesh.reserveQuad();	
	} 
	else if (points.empty()) {
		points.emplace_back(pos);
		segments.emplace_back(Segment(pos, pos));
		mesh.reserveQuad();
	}
	else {
		points.emplace_back(pos);
		segments.back().moveP2(pos);
	}
}
void Line::moveLast(const glm::vec2& pos) {
	points.back() = pos;
	segments.back().moveP2(pos);
}
static void updateVertex(LineVertex* data, const Segment& s, float r) {
	if (s.openLeft && s.openRight) {
		data[0] = LineVertex{ s.p1 - r * s.dir - r * s.n1, s.p1, s.p2, r };
		data[1] = LineVertex{ s.p1 - r * s.dir + r * s.n1, s.p1, s.p2, r };
		data[2] = LineVertex{ s.p2 + r * s.dir - r * s.n2, s.p1, s.p2, r };
		data[3] = LineVertex{ s.p2 + r * s.dir + r * s.n2, s.p1, s.p2, r };
	}
	else if (s.openLeft) {
		data[0] = LineVertex{ s.p1 - r * s.dir - r * s.n1, s.p1, s.p2, r };
		data[1] = LineVertex{ s.p1 - r * s.dir + r * s.n1, s.p1, s.p2, r };
		data[2] = LineVertex{ s.p2 - r * s.n2, s.p1, s.p2, r };
		data[3] = LineVertex{ s.p2 + r * s.n2, s.p1, s.p2, r };
	}
	else if (s.openRight) {
		data[0] = LineVertex{ s.p1 - r * s.n1, s.p1, s.p2, r };
		data[1] = LineVertex{ s.p1 + r * s.n1, s.p1, s.p2, r };
		data[2] = LineVertex{ s.p2 + r * s.dir - r * s.n2, s.p1, s.p2, r };
		data[3] = LineVertex{ s.p2 + r * s.dir + r * s.n2, s.p1, s.p2, r };
	}
	else {
		data[0] = LineVertex{ s.p1 - r * s.n1, s.p1, s.p2, r };
		data[1] = LineVertex{ s.p1 + r * s.n1, s.p1, s.p2, r };
		data[2] = LineVertex{ s.p2 - r * s.n2, s.p1, s.p2, r };
		data[3] = LineVertex{ s.p2 + r * s.n2, s.p1, s.p2, r };
	}
}
void Line::updateMesh(LineMesh& mesh) {
	auto sIndex = segments.size() - 1;
	auto data = &mesh.vertex[4 * sIndex]+ meshOffset;
	auto& segment = segments[sIndex];
	updateVertex(data, segment, radius);
}
