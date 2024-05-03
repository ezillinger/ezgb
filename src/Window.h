#pragma once

#include "Base.h"
#include <SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
    #include <SDL_opengles2.h>
#else
    #include <SDL_opengl.h>
#endif

namespace ez {
class Window {
  public:
    Window(const char* title, int w = 1280, int h = 720);
    ~Window();
    EZ_DECLARE_COPY_MOVE(Window, delete, delete);

    template <typename TFunc>
    inline bool run(TFunc&& func) {
        begin_frame();
        func();
        end_frame();
        return m_shouldExit;
    }

  protected:
    void begin_frame();
    void end_frame();

    bool m_shouldExit = false;

    SDL_Window* m_window = nullptr;
    SDL_GLContext m_glContext{};
};
} // namespace ez