// TextRenderer.cpp
#include "TextRenderer.h"
#include <iostream> 
// FreeType is already included via TextRenderer.h -> ft2build.h and freetype.h

TextRenderer::TextRenderer(GLuint windowWidth, GLuint windowHeight) 
    : m_textShader(nullptr), m_vao(0), m_vbo(0), 
      m_windowWidth(windowWidth), m_windowHeight(windowHeight),
      m_ft(nullptr), m_face(nullptr), m_fontLoaded(false), m_textureAtlasID(0) {

    m_textShader = new Shader();
    if (!m_textShader->load("shaders/text.vert", "shaders/text.frag")) {
        std::cerr << "TextRenderer Error: Failed to load text shaders!" << std::endl;
        delete m_textShader;
        m_textShader = nullptr;
        return; // Critical error, cannot render text
    }

    // Initialize FreeType
    if (FT_Init_FreeType(&m_ft)) {
        std::cerr << "TextRenderer Error: Could not init FreeType Library" << std::endl;
        return; // Critical error
    }

    // Load font - Placeholder path, replace with your font
    // Common system font paths:
    // Windows: "C:/Windows/Fonts/arial.ttf"
    // Linux: "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf" (or similar)
    // MacOS: "/Library/Fonts/Arial.ttf"
    // Or provide a path relative to your executable e.g. "assets/fonts/yourfont.ttf"
    if (!loadFont("assets/fonts/minecraft_font.ttf", 24)) { 
         std::cerr << "TextRenderer Warning: Failed to load default font. Text rendering will not work." << std::endl;
         // Font loading failure is not immediately critical for program run, but text won't show.
    }

    // Configure VAO/VBO for texture quads
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0); 

    setWindowSize(m_windowWidth, m_windowHeight);      
}

TextRenderer::~TextRenderer() {
    delete m_textShader;
    if (m_fontLoaded) {
        FT_Done_Face(m_face);
    }
    if (m_ft) {
        FT_Done_FreeType(m_ft);
    }
    if (m_textureAtlasID != 0) {
        glDeleteTextures(1, &m_textureAtlasID);
    }
    if (m_vao != 0) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);
}

bool TextRenderer::loadFont(const std::string& fontPath, GLuint fontSize) {
    if (m_fontLoaded) { // If a font is already loaded, clean it up first
        FT_Done_Face(m_face);
        glDeleteTextures(1, &m_textureAtlasID);
        m_characters.clear();
        m_fontLoaded = false;
        m_textureAtlasID = 0;
    }

    if (FT_New_Face(m_ft, fontPath.c_str(), 0, &m_face)) {
        std::cerr << "TextRenderer Error: Failed to load font: " << fontPath << std::endl;
        return false;
    }

    FT_Set_Pixel_Sizes(m_face, 0, fontSize); // Set font size

    // --- Texture Atlas Generation ---
    // Determine atlas size: For simplicity, try a fixed size or estimate based on character count and size.
    // A more robust solution would use a rectangle packing algorithm.
    // For now, let's aim for a 512x512 atlas and pack ASCII 0-127.
    m_atlasSize = glm::ivec2(512, 512);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Important for single-byte alignment for glyphs

    glGenTextures(1, &m_textureAtlasID);
    glBindTexture(GL_TEXTURE_2D, m_textureAtlasID);
    // Create an empty texture for the atlas
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_atlasSize.x, m_atlasSize.y, 0, GL_RED, GL_UNSIGNED_BYTE, 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int xOffset = 0;
    int yOffset = 0;
    int maxRowHeight = 0;

    for (unsigned char c = 0; c < 128; ++c) { // Load ASCII characters 0-127
        if (FT_Load_Char(m_face, c, FT_LOAD_RENDER)) {
            std::cerr << "TextRenderer Warning: Failed to load Glyph: " << c << std::endl;
            continue;
        }

        // Check if glyph fits in current row
        if (static_cast<unsigned int>(xOffset) + m_face->glyph->bitmap.width > static_cast<unsigned int>(m_atlasSize.x)) {
            xOffset = 0;
            yOffset += maxRowHeight;
            maxRowHeight = 0;
        }
        // Check if glyph fits in atlas height
        if (static_cast<unsigned int>(yOffset) + m_face->glyph->bitmap.rows > static_cast<unsigned int>(m_atlasSize.y)) {
            std::cerr << "TextRenderer Error: Texture atlas too small for all glyphs!" << std::endl;
            break; 
        }

        // Upload glyph bitmap to texture atlas
        glTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            xOffset,
            yOffset,
            m_face->glyph->bitmap.width,
            m_face->glyph->bitmap.rows,
            GL_RED,
            GL_UNSIGNED_BYTE,
            m_face->glyph->bitmap.buffer
        );

        // Store character metrics
        Character character = {
            m_textureAtlasID, // All characters use the same atlas texture ID
            glm::ivec2(m_face->glyph->bitmap.width, m_face->glyph->bitmap.rows),
            glm::ivec2(m_face->glyph->bitmap_left, m_face->glyph->bitmap_top),
            static_cast<GLuint>(m_face->glyph->advance.x >> 6) // Advance is 1/64 pixels
        };
        // Store UV coordinates within the Character struct
        // This requires knowing where it was placed in the atlas
        // This part was missing in the original .h, adding it back for this approach.
        // If Character struct doesn't have U1,V1,U2,V2, you'd calculate in renderText.
        // Let's assume Character struct will be updated to hold UVs again.
        character.U1 = static_cast<float>(xOffset) / m_atlasSize.x;
        character.V1 = static_cast<float>(yOffset) / m_atlasSize.y;
        character.U2 = static_cast<float>(xOffset + m_face->glyph->bitmap.width) / m_atlasSize.x;
        character.V2 = static_cast<float>(yOffset + m_face->glyph->bitmap.rows) / m_atlasSize.y;

        m_characters.insert(std::pair<char, Character>(c, character));

        xOffset += m_face->glyph->bitmap.width + 1; // Add 1 pixel padding
        if (m_face->glyph->bitmap.rows > static_cast<unsigned int>(maxRowHeight)) {
            maxRowHeight = m_face->glyph->bitmap.rows;
        }
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // Reset to default
    m_fontLoaded = true;
    return true;
}

void TextRenderer::setWindowSize(GLuint windowWidth, GLuint windowHeight) {
    m_windowWidth = windowWidth;
    m_windowHeight = windowHeight;
    if (m_textShader) {
        m_textShader->use();
        glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(m_windowWidth), 0.0f, static_cast<GLfloat>(m_windowHeight));
        m_textShader->setMat4("projection", projection);
    }
}

void TextRenderer::renderText(std::string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color) {
    if (!m_fontLoaded || !m_textShader || m_textureAtlasID == 0) return;

    m_textShader->use();
    m_textShader->setVec3("textColor", color);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureAtlasID); // Bind the single texture atlas
    glBindVertexArray(m_vao);

    GLfloat originalX = x; // Store original x for multiline (not implemented here)

    for (std::string::const_iterator it = text.begin(); it != text.end(); ++it) {
        char c = *it;
        // Handle newline (simple version)
        if (c == '\n') {
            // Character ch_A = m_characters['A']; // Get metrics from a sample char like 'A' or space
            // y -= (ch_A.Size.y + some_line_spacing) * scale; // Approximate line height
            // For now, get line height from FT_Face (m_face->size->metrics.height >> 6)
            y -= (m_face->size->metrics.height >> 6) * scale; 
            x = originalX;
            continue;
        }

        std::map<GLchar, Character>::iterator char_it = m_characters.find(c);
        if (char_it == m_characters.end()) {
            std::cerr << "TextRenderer: Character '" << c << "' not found in font map." << std::endl;
            x += 10 * scale; // Advance for unknown char
            continue;
        }
        Character ch = char_it->second;

        GLfloat xpos = x + ch.Bearing.x * scale;
        GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale; // Y is from baseline up

        GLfloat w = ch.Size.x * scale;
        GLfloat h = ch.Size.y * scale;
        
        // UVs are now part of the Character struct
        GLfloat vertices[6][4] = {
            { xpos,     ypos + h,   ch.U1, ch.V1 }, // Top-left for char quad      
            { xpos,     ypos,       ch.U1, ch.V2 }, // Bottom-left for char quad
            { xpos + w, ypos,       ch.U2, ch.V2 }, // Bottom-right for char quad

            { xpos,     ypos + h,   ch.U1, ch.V1 },
            { xpos + w, ypos,       ch.U2, ch.V2 },
            { xpos + w, ypos + h,   ch.U2, ch.V1 }  // Top-right for char quad
        };
        
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); 
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        x += ch.Advance * scale;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0); // Unbind atlas
} 