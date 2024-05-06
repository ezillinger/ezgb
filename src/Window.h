#pragma once

#include "Base.h"
#include "ThirdParty_SDL.h" 

#include "Audio.h"
#include <deque>
#include <mutex>

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

    void push_audio(std::span<const audio::Sample> data);

  protected:

    void init_audio();
    static void audio_callback(void* userdata, uint8_t* stream, int len);
    void fill_audio_buffer(std::span<audio::Sample> dst);

    audio::Sample get_next_sample();

    void begin_frame();
    void end_frame();

    bool m_shouldExit = false;

    bool m_audioInitialized = false;

    std::mutex m_audioLock{};
    std::deque<audio::Sample> m_audioBuffer;

    SDL_AudioDeviceID m_audioDevice = 0;
    SDL_Window* m_window = nullptr;
    SDL_GLContext m_glContext{};
};
} // namespace ez