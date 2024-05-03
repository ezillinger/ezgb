#include "Texture.h"

namespace ez {

static GLuint makeHandle() {
    GLuint handle = 0;
    glGenTextures(1, &handle);
    return handle;
}

Tex2D::Tex2D(const int2& dim, std::span<const rgba8> data) : m_dim(dim), m_handle(makeHandle()) {  
    glBindTexture(GL_TEXTURE_2D, m_handle);
    // todo, make these configurable
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // nullptr is ok, we're just setting size
    update(data);
}

Tex2D::~Tex2D() { glDeleteTextures(1, &m_handle); };

void ez::Tex2D::update(std::span<const rgba8> data) {

    // todo, use pixel buffer to upload
    ez_assert(data.empty() || int(data.size()) == m_dim.area());
    glBindTexture(GL_TEXTURE_2D, m_handle);
    glTexImage2D(GL_TEXTURE_2D, 0, m_format, m_dim.x, m_dim.y, 0, m_format, GL_UNSIGNED_BYTE, data.data());
}

} // namespace ez