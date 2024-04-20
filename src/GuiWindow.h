#include "Base.h"
#include <SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
    #include <SDL_opengles2.h>
#else
    #include <SDL_opengl.h>
#endif



namespace ez {
    class GuiWindow {
    public:
        GuiWindow(const char* title, int w = 1280, int h = 720);
        ~GuiWindow();
        GuiWindow(const GuiWindow&) = delete;

        template <typename TFunc> 
        inline bool run(TFunc&& func) { 
            beginFrame();
            func();
            endFrame();
            return m_shouldExit;
        }

      private:
        void beginFrame();
        void endFrame();

        bool m_shouldExit = false;

        SDL_Window* m_window = nullptr;
        SDL_GLContext m_glContext{};
    };
} // namespace ez