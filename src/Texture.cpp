#include "Texture.h"

namespace ez {

static GLuint makeHandle() {
    GLuint handle = 0;
    glGenTextures(1, &handle);
    return handle;
}

Tex2D::Tex2D(const int2& dim, std::span<const rgba8> data) : m_dim(dim), m_handle(makeHandle()) {
    log_info("Creating texture");
    glBindTexture(GL_TEXTURE_2D, m_handle);
    // todo, make these configurable
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif

    // nullptr is ok, we're just setting size
    ez_assert(data.empty() || int(data.size()) == m_dim.area());
    glTexImage2D(GL_TEXTURE_2D, 0, m_format, m_dim.x, m_dim.y, 0, m_format, GL_UNSIGNED_BYTE, data.data());
}

Tex2D::~Tex2D() { glDeleteTextures(1, &m_handle); };

void ez::Tex2D::update(std::span<const rgba8> data) {
    ez_assert(int(data.size()) == m_dim.area());
    glBindTexture(GL_TEXTURE_2D, m_handle);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_dim.x, m_dim.y, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
}

} // namespace ez