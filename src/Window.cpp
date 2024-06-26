#include "Window.h"

#include "ThirdParty_ImGui.h"
#include "libs/imgui/backends/imgui_impl_opengl3.h"
#include "libs/imgui/backends/imgui_impl_sdl2.h"

namespace ez {

Window::Window(const char* title, int width, int height) {
    log_info("Creating window");
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) != 0) {
        fail("Failed to init SDL: {}\n", SDL_GetError());
    }
    log_info("SDL_Init completed");

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,
                        SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags =
        (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    m_window = SDL_CreateWindow(
        title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, window_flags);
    if (m_window == nullptr) {
        fail("Error: SDL_CreateWindow(): {}", SDL_GetError());
    }

    m_glContext = SDL_GL_CreateContext(m_window);
    SDL_GL_MakeCurrent(m_window, m_glContext);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(m_window, m_glContext);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple
    // fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the
    // font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those
    // errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture
    // when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below
    // will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher
    // quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to
    // write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the
    // "fonts/" folder. See Makefile.emscripten for details.
    // io.Fonts->AddFontDefault();
    // io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);

    log_info("Loading Fonts");
#if EZ_WASM
    io.Fonts->AddFontDefault();
#else
    ImFont* defaultFont = nullptr;
    for (const auto& fontPath : std::filesystem::directory_iterator("./data/fonts")) {
        if (fontPath.path().extension() == ".ttf") {
            auto fontPtr = io.Fonts->AddFontFromFileTTF(fontPath.path().string().c_str(), 15.0f);
            if (fontPath.path().string().find("Cousine") != std::string::npos) {
                defaultFont = fontPtr;
            }
        }
        if (defaultFont) {
            io.FontDefault = defaultFont;
        }
        // io.Fonts->AddFontFromFileTTF("./data/fonts/ProggyClean.ttf", 15.0f);
        // io.Fonts->AddFontFromFileTTF("./data/fonts/Roboto-Medium.ttf", 15.0f);
    }
#endif
    log_info("Finished loading fonts");
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    // ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f,
    // nullptr, io.Fonts->GetGlyphRangesJapanese()); IM_ASSERT(font != nullptr);

#if !EZ_WASM
    // we're not allowed to initialize audio until a user input in the browser
    init_audio();
#endif
}

Window::~Window() {
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    if (m_audioInitialized) {
        SDL_CloseAudioDevice(m_audioDevice);
    }
    SDL_GL_DeleteContext(m_glContext);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void Window::audio_callback(void* userdata, uint8_t* stream, int len) {
    auto& window = *reinterpret_cast<Window*>(userdata);
    const size_t numSamples = len / int(audio::NUM_CHANNELS * sizeof(float));
    window.fill_audio_buffer({reinterpret_cast<audio::Sample*>(stream), numSamples});
}

void Window::fill_audio_buffer(std::span<audio::Sample> dst) {
    const auto lg = std::scoped_lock(m_audioLock);
    for (size_t i = 0; i < dst.size(); ++i) {
        dst[i] = get_next_sample();
    }
}

audio::Sample Window::get_next_sample() {
    if (m_audioBuffer.empty()) {
        return {0.0f, 0.0f};
    }
    const auto ret = m_audioBuffer.front();
    m_audioBuffer.pop_front();
    return ret;
}

void Window::push_audio(std::span<const audio::Sample> data) {
    if (!data.empty()) {
        // if we're more than a half second behind dump old audio
        const auto maxBufferSize = audio::SAMPLE_RATE / 2;
        const auto lg = std::scoped_lock(m_audioLock);
        if (m_audioBuffer.size() > maxBufferSize) {
            m_audioBuffer.clear();
        }
        m_audioBuffer.insert(m_audioBuffer.end(), data.begin(), data.end());
    }
}

void Window::init_audio() {

    ez_assert(!m_audioInitialized);
    log_info("Initializing Audio");

    // todo, fix audio callback
    SDL_AudioSpec specDesired{};
    specDesired.freq = audio::SAMPLE_RATE;
    specDesired.channels = 2;
    specDesired.format = audio::FORMAT;
    specDesired.samples = audio::BUFFER_SIZE;
    specDesired.callback = audio_callback;
    specDesired.userdata = this;

    SDL_AudioSpec specObtained{};
    const auto allowNoChanges = 0;
    const auto noCapture = 0;
    m_audioDevice =
        SDL_OpenAudioDevice(nullptr, noCapture, &specDesired, &specObtained, allowNoChanges);
    SDL_PauseAudioDevice(m_audioDevice, 0);

    m_audioInitialized = true;
    log_info("Finished Initializing Audio");
}

void Window::begin_frame() {
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui
    // wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main
    // application, or clear/overwrite your copy of the mouse data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main
    // application, or clear/overwrite your copy of the keyboard data. Generally you may always pass
    // all inputs to dear imgui, and hide them from your application based on those two flags.
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT)
            m_shouldExit = true;
        if (event.type == SDL_WINDOWEVENT && event.window.windowID == SDL_GetWindowID(m_window)) {
            if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                m_shouldExit = true;
            }
            if(m_audioInitialized){
                if(event.window.event == SDL_WINDOWEVENT_FOCUS_LOST){
                    SDL_PauseAudioDevice(m_audioDevice, 1);
                }
                if(event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED){
                    SDL_PauseAudioDevice(m_audioDevice, 0);
                }
            }
        }
    }

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

#if EZ_WASM
    // only initialize audio once we have user input - browser prevents audio from starting before
    // and we get out of sync
    if (!m_audioInitialized) {
        ImGui::SetTooltip("Click anywhere to enable audio");
        if (ImGui::GetIO().WantCaptureMouse && ImGui::IsMouseClicked(0)) {
            init_audio();
        }
    }
#endif
}

void Window::end_frame() {

    // Rendering
    ImGui::Render();
    auto& io = ImGui::GetIO();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    const auto clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    glClearColor(clear_color.x * clear_color.w,
                 clear_color.y * clear_color.w,
                 clear_color.z * clear_color.w,
                 clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(m_window);
}

} // namespace ez