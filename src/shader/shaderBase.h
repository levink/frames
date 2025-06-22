#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "shaderSource.h"
#include "model/mesh.h"

struct GLSLType {
    GLint size;
    GLenum type;
};

struct Attribute {
    GLint id;
    GLint size;
    GLenum type;
    std::string name;
    Attribute();
    Attribute(GLSLType type, std::string name);
};

struct Uniform {
    GLint id;
    std::string name;
    Uniform();
    explicit Uniform(std::string name);
};

class Shader {

protected:
    constexpr static const GLSLType FLOAT = GLSLType{ 1, GL_FLOAT };
    constexpr static const GLSLType VEC_2 = GLSLType{ 2, GL_FLOAT };
    constexpr static const GLSLType VEC_3 = GLSLType{ 3, GL_FLOAT };

    static void setViewPort(const glm::ivec2& leftBottom, const glm::ivec2& size);
    static void attr(const Attribute& attribute, const void* data, GLsizei stride, GLuint offset);
    static void attr(const Attribute& attribute, GLsizei stride, GLuint offset);
    static void attr(const Attribute& attribute, const std::vector<float>& data);
    static void attr(const Attribute& attribute, const std::vector<glm::vec2>& data);
    static void attr(const Attribute& attribute, const std::vector<glm::vec3>& data);
    static void set1(const Uniform& uniform, float value);
    static void set3(const Uniform& uniform, const glm::vec3& value);
    static void set4(const Uniform& uniform, const glm::mat4& value);
    static void drawFaces(const std::vector<GLFace>& faces);

    GLuint programId;
    std::vector<Attribute> a;
    std::vector<Uniform> u;
public:

    Shader(uint8_t uniforms, uint8_t attributes);
    virtual ~Shader();

    void create(const ShaderSource& source);
    void destroy();
    virtual void enable() const;
    virtual void disable() const;
};


