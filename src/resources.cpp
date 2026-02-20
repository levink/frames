#include "resources.h"

#ifdef _DEBUG
#define _FRAMES_DATA_PATH(resource) "../../" resource
#else
#define _FRAMES_DATA_PATH(resource) resource
#endif

namespace resources {
	const char* programName = "Frames Player by Levin K. (v1.0.2)";
	const char* linesShader = _FRAMES_DATA_PATH("./data/shaders/lines.glsl");
	const char* pointShader = _FRAMES_DATA_PATH("./data/shaders/point.glsl");
	const char* videoShader = _FRAMES_DATA_PATH("./data/shaders/video.glsl");
	const char* font		= _FRAMES_DATA_PATH("./data/fonts/calibri.ttf");
	const char* workspace = "./workspace.ini";
}

#undef _FRAMES_DATA_PATH