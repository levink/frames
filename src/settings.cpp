#include "settings.h"
#include <fstream>

using namespace std;

namespace {
	struct BinaryWriter {
		ofstream& file;
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
	};

	struct BinaryReader {
		ifstream& file;
		char buf[1024] = {0};
		uint32_t getUInt32() {
			uint32_t result = 0;
			file.read((char*)&result, sizeof(result));
			return result;
		}
		uint64_t getUInt64() {
			uint64_t result = 0;
			file.read((char*)&result, sizeof(result));
			return result;
		}
		void getString(string& value) {
			uint64_t count = 0;
			file.read((char*)&count, sizeof(count));
			file.read(buf, count);
			buf[count] = 0;
			value.assign(buf);
		}
	};
}

void Settings::save(const char* path) {
	ofstream file(path, ios::out | ios::binary);
	if (!file.is_open()) {
		return;
	}

	auto writer = BinaryWriter(file);
	writer.putUInt32(formatVersion);
	writer.putString(folderWindow.folder);
	writer.putUInt64(folderWindow.files.size());
	for (const auto& item : folderWindow.files) {
		writer.putString(item);
	}
}

void Settings::load(const char* path) {
	ifstream file(path, ios::in | ios::binary);
	if (!file.is_open()) {
		return;
	}

	auto reader = BinaryReader(file);
	auto version = reader.getUInt32();
	if (version != formatVersion) {
		return;
	}

	reader.getString(folderWindow.folder);

	auto size = reader.getUInt64();
	folderWindow.files.resize(size);
	for (size_t i = 0; i < size; i++) {
		reader.getString(folderWindow.files[i]);
	}
}
