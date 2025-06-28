#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <fstream>
#include <iostream>
#include <string_view>
#include "shaderBase.h"
#include "model/mesh.h"

using std::string;

struct Source {
    string vertex;
    string fragment;
    Source() = default;
    Source(string&& vertex, string&& fragment);
    Source(Source&& right) noexcept;
    Source(const Source& right) = delete;
    Source& operator=(Source&& right) noexcept;
    Source& operator=(const Source& right) = delete;
};

static void warning(std::string_view error) {
    std::cout << "[Warning] " << error << std::endl;
}
static void warning(std::string_view error, std::string_view data) {
    std::cout << "[Warning] " << error << " " << data << std::endl;
}
static void swap(Source& left, Source& right) {
    std::swap(left.vertex, right.vertex);
    std::swap(left.fragment, right.fragment);
}
static string getShaderText(const char* fileName) {
    string result;
    std::ifstream input(fileName, std::ifstream::binary);
    if (!input) {
        warning("Could not open file", fileName);
        return result;
    }

    input.seekg(0, std::ios::end);
    size_t file_length = input.tellg();
    input.seekg(0, std::ios::beg);

    char* buf = new char[file_length + 1];
    std::streamsize readCount = 0;
    while (readCount < file_length) {
        input.read(buf + readCount, file_length - readCount);
        readCount += input.gcount();
    }

    buf[file_length] = 0;
    input.close();

    result.assign(buf);
    delete[] buf;

    return std::move(result);
}
static Source load(const char* path) {
    string text = getShaderText(path);

    static const auto vertexTag = "//#vertex";
    auto vertexPosition = text.find(vertexTag);
    if (vertexPosition == string::npos) {
        warning("Vertex tag not found in shader", path);
        return {};
    }

    static const auto fragmentTag = "//#fragment";
    auto fragmentPosition = text.find(fragmentTag);
    if (fragmentPosition == string::npos) {
        warning("Fragment tag not found in shader", path);
        return {};
    }

    auto uniforms = text.substr(0, vertexPosition);
    auto vertex = text.substr(0, fragmentPosition);
    auto fragment = uniforms + text.substr(fragmentPosition);
    return { std::move(vertex), std::move(fragment) };
}
static GLuint compile(GLenum shaderType, const char* shaderText) {
    if (shaderText == nullptr) {
        string error;
        switch (shaderType) {
        case GL_VERTEX_SHADER: error = "Vertex shader text is null"; break;
        case GL_FRAGMENT_SHADER: error = "Fragment shader text is null"; break;
        default: error = "Shader text is null"; break;
        }
        warning(error);
        return 0;
    }

    GLint compiled = 0;
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &shaderText, nullptr);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled) {
        return shader;
    }

    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    if (length) {
        std::vector<GLchar> description(length);
        glGetShaderInfoLog(shader, length, &length, description.data());

        std::string_view error;
        switch (shaderType) {
        case GL_VERTEX_SHADER: { error = "Could not compile vertex shader"; break; }
        case GL_FRAGMENT_SHADER: { error = "Could not compile fragment shader"; break; }
        default: { error = "Could not compile shader"; break; }
        }
        warning(error, description.data());
    }
    else {
        warning("Could not compile shader, but info log is empty");
    }

    glDeleteShader(shader);
    return 0;
}
static GLuint link(GLuint vertexShader, GLuint fragmentShader) {
    if (!vertexShader || !fragmentShader) {
        warning("Can't link shader because of empty parts");
        return 0;
    }

    GLint linked = 0;
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked) {
        glDetachShader(program, vertexShader);
        glDetachShader(program, fragmentShader);
        return program;
    }

    GLint length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    if (length) {
        std::vector<GLchar> info(length);
        glGetProgramInfoLog(program, length, &length, info.data());
        warning("Could not link program", info.data());
    }
    else {
        warning("Could not link shader, but info log is empty");
    }

    glDetachShader(program, vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteProgram(program);
    return 0;
}
static GLuint build(const char* path) {
    Source shaderSource = load(path);
    GLuint vertexShader = compile(GL_VERTEX_SHADER, shaderSource.vertex.c_str());
    GLuint fragmentShader = compile(GL_FRAGMENT_SHADER, shaderSource.fragment.c_str());
    GLuint programId = link(vertexShader, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return programId;
}

Source::Source(string&& vertex, string&& fragment) : 
    vertex(vertex), 
    fragment(fragment) { }
Source::Source(Source&& right) noexcept : Source() {
    swap(*this, right);
}
Source& Source::operator=(Source&& right) noexcept {
    if (this == &right) {
        return *this;
    }

    swap(*this, right);
    return *this;
}

Attribute::Attribute() : id(-1), size(-1), type(GL_NONE) {}
Attribute::Attribute(GLSLType type, string name) :
    id(-1),
    size(type.size),
    type(type.type),
    name(std::move(name)) {
}

Uniform::Uniform() : id(-1) {}
Uniform::Uniform(string name) :
    id(-1),
    name(std::move(name)) {
}

Shader::Shader(uint8_t uniforms, uint8_t attributes) :
    programId(0) {
    u.resize(uniforms);
    a.resize(attributes);
}
Shader::~Shader() {
    destroy();
}
void Shader::create(const char* path) {

    programId = build(path);
    if (programId == 0) {
        return;
    }

    glUseProgram(programId);
    for (auto& uniform : u) {
        uniform.id = glGetUniformLocation(programId, uniform.name.c_str());
    }
    for (auto& attribute : a) {
        attribute.id = glGetAttribLocation(programId, attribute.name.c_str());
    }
    glUseProgram(0);
}
void Shader::destroy() {
    if (programId > 0) {
        glDeleteProgram(programId);
        programId = 0;
    }
}
void Shader::enable() const {
    glUseProgram(programId);
    for (auto& attribute : a) {
        if (attribute.id != -1) {
            glEnableVertexAttribArray(attribute.id);
        }
    }
}
void Shader::disable() const {
    for (auto& attribute : a) {
        if (attribute.id != -1) {
            glDisableVertexAttribArray(attribute.id);
        }
    }
    glUseProgram(0);
}
void Shader::attr(const Attribute& attribute, const void* data, GLsizei stride, GLuint offset) {
    if (attribute.id != -1) {
        glVertexAttribPointer(
            attribute.id,
            attribute.size,
            attribute.type,
            GL_FALSE,
            stride,
            static_cast<const GLbyte*>(data) + offset
        );
    }
}
void Shader::attr(const Attribute& attribute, GLsizei stride, GLuint offset) {
    if (attribute.id != -1) {
        glVertexAttribPointer(
            attribute.id,
            attribute.size,
            attribute.type,
            GL_FALSE,
            stride,
            static_cast<const GLubyte*>(nullptr) + offset
        );
    }
}
void Shader::attr(const Attribute& attribute, const std::vector<float>& data) {
    attr(attribute, data.data(), 0, 0);
}
void Shader::attr(const Attribute& attribute, const std::vector<glm::vec2>& data) {
    attr(attribute, data.data(), 0, 0);
}
void Shader::attr(const Attribute& attribute, const std::vector<glm::vec3>& data) {
    attr(attribute, data.data(), 0, 0);
}
void Shader::set1(const Uniform& uniform, float value) {
    if (uniform.id != -1) {
        glUniform1f(uniform.id, value);
    }
}
void Shader::set3(const Uniform& uniform, const glm::vec3& value) {
    if (uniform.id != -1) {
        glUniform3fv(uniform.id, 1, glm::value_ptr(value));
    }
}
void Shader::set4(const Uniform& uniform, const glm::mat4& value) {
    if (uniform.id != -1) {
        glUniformMatrix4fv(uniform.id, 1, GL_FALSE, glm::value_ptr(value));
    }
}
void Shader::drawFaces(const std::vector<GLFace>& faces) {
    if (faces.empty()) {
        return;
    }
    glDrawElements(GL_TRIANGLES, static_cast<int>(faces.size() * 3u), GL_UNSIGNED_SHORT, faces.data());
}
