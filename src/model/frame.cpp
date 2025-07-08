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
}

void FrameBuffer::create(float w, float h) {
	width = w;
	height = h;
	
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glGenTextures(1, &tid);
	glBindTexture(GL_TEXTURE_2D, tid);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tid, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		cout << "Error: Framebuffer is not complete\n";
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void FrameBuffer::reshape(float w, float h) {
	width = w;
	height = h;
	glBindTexture(GL_TEXTURE_2D, tid);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tid, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}
void FrameBuffer::destroy() {
	glDeleteFramebuffers(1, &fbo);
	glDeleteRenderbuffers(1, &rbo);
	glDeleteTextures(1, &tid);
	fbo = 0;
	rbo = 0;
	tid = 0;
	width = 0;
	height = 0;
}

void FrameRender::createTexture(int16_t width, int16_t height) {
    if (imageMesh.textureId) {
        glDeleteTextures(1, &imageMesh.textureId);
    }
	imageMesh = ImageMesh::createImageMesh(width, height);
	imageMesh.textureId = gl::createTexture(width, height);
	imageMesh.textureReady = false;
    cam.init({ -width * 0.5f, -height * 0.5f }, 1.f);
}
void FrameRender::updateTexture(int16_t width, int16_t height, const uint8_t* pixels) {
	gl::updateTexture(imageMesh.textureId, width, height, pixels);
	imageMesh.textureReady = true;
}
void FrameRender::destroyTexture() {
	if (imageMesh.textureId) {
		glDeleteTextures(1, &imageMesh.textureId);
		imageMesh.textureId = 0;
		imageMesh.textureReady = false;
	}
}
void FrameRender::reshape(int width, int height) {
	cam.reshape(width, height);
	fb.reshape(width, height);
}
void FrameRender::moveCam(int dx, int dy) {
	cam.move(dx, -dy);
}
void FrameRender::zoomCam(float units) {
	cam.zoom(units * 0.1f);
	setCursor(cursor.screen.x, cursor.screen.y);
}
void FrameRender::render(ShaderContext& shaders) const {
	
	glBindFramebuffer(GL_FRAMEBUFFER, fb.fbo);
	glBindRenderbuffer(GL_RENDERBUFFER, fb.rbo);
	glBindTexture(GL_TEXTURE_2D, fb.tid);

	glViewport(0, 0, fb.width, fb.height);
	glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

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

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::vec2 FrameRender::toOpenGLSpace(int x, int y) const {
    /*
       Converting
       from view space x = [0, fb.Width], y = [0, fb.Height]	- coordinates from top left corner of view
       to OpenGL space x = [-1, 1], y = [-1, 1]					- coordinates from bottom left corner of view
    */
    auto result = glm::vec2(
        ((2.f * x) / fb.width) - 1.f,
        1.f - ((2.f * y) / fb.height)
    );
    return result;
}
glm::vec2 FrameRender::toSceneSpace(int x, int y) const {
    auto point2D = toOpenGLSpace(x, y);	// x=[-1, 1], y=[-1, 1]
    auto point4D = cam.pv_inverse * glm::vec4(point2D.x, point2D.y, 0.5f, 1.f);
	return { point4D.x / point4D.w, point4D.y / point4D.w };
}
float FrameRender::getLineRadius() const {
	return 0.5f * lineWidth * cam.scale_inverse;
}
glm::vec2 FrameRender::setCursor(int x, int y) {
	cursor.screen = { x, y };
	cursor.position = toSceneSpace(x, y);

	float radius = getLineRadius();
	cursor.mesh.createPoint(0, cursor.position, radius);
	cursor.mesh.color = frontColor;
	
	return cursor.position;
}
void FrameRender::setBrush(const float color[3], float width) {
	lineWidth = width;
	frontColor[0] = color[0];
	frontColor[1] = color[1];
	frontColor[2] = color[2];
	lineMesh.color = frontColor;

	float radius = getLineRadius();
	cursor.mesh.createPoint(0, cursor.position, radius);
	cursor.mesh.color = frontColor;
}
void FrameRender::showCursor(bool visible) {
	cursor.visible = visible;
}
void FrameRender::moveCursor(int x, int y) {
	setCursor(x, y);
}
void FrameRender::drawStart(int x, int y) {
	if (!draw) {
		draw = true;
	}
		
	const auto pos = setCursor(x, y);
	const auto radius = getLineRadius();
	const auto meshOffset = lineMesh.vertex.size();
	Line& line = lines.emplace_back(Line(radius, meshOffset));
	line.addPoint(pos, lineMesh);
	line.updateMesh(lineMesh);
}
void FrameRender::drawNext(int x, int y, int mode) {

	auto pos = setCursor(x, y);
	if (draw && !lines.empty()) {
		auto& line = lines.back();
		if (line.points.size() < 2) {
			line.addPoint(pos, lineMesh);
		} else {
			if (mode == 0) {
				line.addPoint(pos, lineMesh);
			} else {
				line.moveLast(pos);
			}
		}
		line.updateMesh(lineMesh);
	}
}
void FrameRender::drawStop() {
	if (draw) {
		draw = false;
	}
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
	//if (s.openLeft && s.openRight) {
		data[0] = LineVertex{ s.p1 - r * s.dir - r * s.n1, s.p1, s.p2, r };
		data[1] = LineVertex{ s.p1 - r * s.dir + r * s.n1, s.p1, s.p2, r };
		data[2] = LineVertex{ s.p2 + r * s.dir - r * s.n2, s.p1, s.p2, r };
		data[3] = LineVertex{ s.p2 + r * s.dir + r * s.n2, s.p1, s.p2, r };
	/*}
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
	}*/
}
void Line::updateMesh(LineMesh& mesh) {
	auto sIndex = segments.size() - 1;
	auto data = &mesh.vertex[4 * sIndex]+ meshOffset;
	auto& segment = segments[sIndex];
	updateVertex(data, segment, radius);
}
