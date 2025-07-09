#include "workstate.h"
#include <fstream>

using namespace std;

namespace {
	struct BinaryWriter {
		ofstream& file;
		void putVersion(uint32_t version) {
			putUInt32(version);
		}
		void putBool(bool value) {
			putUInt8(value ? 1 : 0);
		}
		void putUInt8(uint8_t value) {
			file.write((const char*)&value, sizeof(value));
		}
		void putUInt32(uint32_t value) {
			file.write((const char*)&value, sizeof(value));
		}
		void putUInt64(uint64_t value) {
			file.write((const char*)&value, sizeof(value));
		}
		void putString(const string& value) {
			uint64_t count = value.length();
			const char* data = value.c_str();
			file.write((const char*)&count, sizeof(count));
			file.write(data, count);
		}
		void putFloat(float value) {
			file.write((const char*)&value, sizeof(value));
		}
		void putRGB(float color[3]) {
			putFloat(color[0]);
			putFloat(color[1]);
			putFloat(color[2]);
		}
	};

	struct BinaryReader {
		ifstream& file;
		char buf[1024] = {0};
		uint32_t getVersion() {
			uint32_t result = 0;
			getUInt32(result);
			return result;
		}
		void getBool(bool& value) {
			uint8_t res = 0; 
			getUInt8(res);
			value = (res == 1);
		}
		void getUInt8(uint8_t& result) {
			file.read((char*)&result, sizeof(result));
		}
		void getUInt32(uint32_t& result) {
			file.read((char*)&result, sizeof(result));
		}
		void getUInt64(uint64_t& result) {
			file.read((char*)&result, sizeof(result));
		}
		void getString(string& value) {
			uint64_t count = 0;
			file.read((char*)&count, sizeof(count));
			file.read(buf, count);
			buf[count] = 0;
			value.assign(buf);
		}
		void getFloat(float& result) {
			file.read((char*)&result, sizeof(result));
		}
		void getRGB(float color[3]) {
			getFloat(color[0]);
			getFloat(color[1]);
			getFloat(color[2]);
		}
	};
}

void WorkState::save(const char* path) {
	ofstream file(path, ios::out | ios::binary);
	if (!file.is_open()) {
		return;
	}

	auto writer = BinaryWriter(file);
	writer.putVersion(formatVersion);

	// Main
	{
		writer.putUInt32(main.width);
		writer.putUInt32(main.height);
	}

	// File Tree
	{
		writer.putString(fileTree.folder);
		writer.putUInt64(fileTree.files.size());
		for (const auto& item : fileTree.files) {
			writer.putString(item);
		}
	}

	// Global
	{
		writer.putBool(openedColor);
		writer.putBool(openedKeys);
		writer.putBool(openedWorkspace);

		writer.putUInt8(splitMode);
		writer.putUInt32(drawLineWidth);
		writer.putRGB(drawLineColor);
	}
}

bool WorkState::load(const char* path) {
	ifstream file(path, ios::in | ios::binary);
	if (!file.is_open()) {
		return false;
	}

	auto reader = BinaryReader(file);
	if (reader.getVersion() != formatVersion) {
		return false;
	}

	// Main
	{
		reader.getUInt32(main.width);
		reader.getUInt32(main.height);
	}
	
	// File Tree
	{
		reader.getString(fileTree.folder);
		size_t size = 0; 
		reader.getUInt64(size);
		fileTree.files.resize(size);
		for (size_t i = 0; i < size; i++) {
			reader.getString(fileTree.files[i]);
		}
	}

	// Global
	{
		reader.getBool(openedColor);
		reader.getBool(openedKeys);
		reader.getBool(openedWorkspace);

		reader.getUInt8(splitMode);
		reader.getUInt32(drawLineWidth);
		reader.getRGB(drawLineColor);
	}
	
	return true;
}
