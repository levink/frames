#include <iostream>
#include <GLFW/glfw3.h>
#include "ffmpeg.h"

int main() {

	/*if (!glfwInit()) {
		std::cout << "glfwInit - error. Exit." << std::endl;
		return -1;
	} else {
		std::cout << "glfwInit - ok" << std::endl;
	}*/

	const char* file = "C:/Users/Konst/Desktop/k/IMG_3504.MOV";
	AVFormatContext* input_ctx = nullptr;
	if (avformat_open_input(&input_ctx, file, nullptr, nullptr) != 0) {
		std::cout << "Cannot open input file" << std::endl;
		return -1;
	}

	/*AVPacket* packet = av_packet_alloc();
	if (!packet) {
		fprintf(stderr, "Failed to allocate AVPacket\n");
		return -1;
	}*/

	return 0;
}