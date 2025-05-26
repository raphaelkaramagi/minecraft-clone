// TextRenderer.h
#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include <glad/glad.h> // For GLuint, GLfloat
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Shader.h" // We'll reuse our Shader class
#include <map>

// FreeType Headers
#include <ft2build.h>
#include FT_FREETYPE_H

// Represents a single character loaded by FreeType
struct Character {
    GLuint     TextureID;    // ID handle of the glyph texture (now part of a texture atlas)
    glm::ivec2 Size;         // Size of glyph
    glm::ivec2 Bearing;      // Offset from baseline to left/top of glyph
    GLuint     Advance;      // Horizontal offset to advance to next glyph
    // UV coordinates in the texture atlas for this specific character
    float      U1, V1;       // Top-left UV
    float      U2, V2;       // Bottom-right UV
    // UV coordinates in the texture atlas are not stored per character anymore,
    // as we build quads directly when rendering, using atlas dimensions.
    // Or, if packing into an atlas and pre-calculating, we would store them.
    // For simplicity with FreeType, we often render glyphs to an atlas texture one by one.
};

class TextRenderer {
public:
    // Constructor now takes window dimensions, not font size directly here
    TextRenderer(GLuint windowWidth, GLuint windowHeight);
    ~TextRenderer();

    // Load a font using FreeType. fontPath is path to .ttf file.
    bool loadFont(const std::string& fontPath, GLuint fontSize);
    void renderText(std::string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color = glm::vec3(1.0f));
    void setWindowSize(GLuint windowWidth, GLuint windowHeight);

private:
    Shader* m_textShader;
    std::map<GLchar, Character> m_characters; // Stores pre-rendered glyphs (or their metrics)
    GLuint m_vao, m_vbo;
    GLuint m_windowWidth, m_windowHeight;
    FT_Library m_ft;      // FreeType library instance
    FT_Face    m_face;      // FreeType face instance
    bool m_fontLoaded;    // Flag to check if font has been successfully loaded
    // GLuint m_fontTextureAtlas; // This concept might change if we render glyphs on the fly
                                 // or if we build a single large atlas. Let's assume an atlas.
    GLuint m_textureAtlasID; // ID of the generated texture atlas
    glm::ivec2 m_atlasSize; // Dimensions of the texture atlas

    // initEmbeddedFont is no longer needed
};

#endif // TEXT_RENDERER_H 