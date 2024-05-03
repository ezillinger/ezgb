#pragma once
#include "Base.h"
#include "ThirdParty_SDL.h"

namespace ez {
class Tex2D {
  public:
    Tex2D(const int2& dim, std::span<const rgba8> data = {});
    ~Tex2D();

    EZ_DECLARE_COPY_MOVE(Tex2D, delete, delete);

    void update(std::span<const rgba8> data);

    const int2& dim() const { return m_dim; }
    GLuint handle() const { return m_handle; }

      protected:
        int2 m_dim{};
        GLuint m_handle = 0;

        // todo, make this configurable
        static constexpr GLint m_format = GL_RGBA;
};
} // namespace ez