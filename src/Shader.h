#ifndef SHADER_H
#define SHADER_H

#include <string>
#include <glad/glad.h> // For GLuint and other OpenGL types

// Include actual GLM type definitions instead of forward declaring
#include <glm/vec3.hpp> // For glm::vec3
#include <glm/mat4x4.hpp> // For glm::mat4 (or can use glm/glm.hpp)
// Note: <glm/gtc/type_ptr.hpp> will be needed in Shader.cpp for value_ptr

class Shader {
public:
    GLuint ID; // Shader Program ID

    Shader();
    ~Shader();

    // Loads shaders from file, compiles and links them.
    // Returns true on success, false on failure.
    bool load(const char* vertexPath, const char* fragmentPath);

    // Activates the shader program
    void use() const;

    // Utility functions for setting uniforms (examples)
    void setBool(const std::string &name, bool value) const;
    void setInt(const std::string &name, int value) const;
    void setFloat(const std::string &name, float value) const;
    void setVec3(const std::string &name, const glm::vec3 &value) const;
    void setMat4(const std::string &name, const glm::mat4 &value) const;

private:
    // Utility function to check for shader compilation/linking errors.
    bool checkCompileErrors(GLuint shader, std::string type);
};

#endif // SHADER_H 