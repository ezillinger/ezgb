#include "Gui.h"
#include <filesystem>

namespace ez {

static constexpr ImVec4 rgbMult(ImVec4 v, float t) {
    return ImVec4{v.x * t, v.y * t, v.z * t, v.w};
}

static constexpr auto brown = ImVec4(59 / 255.0f, 47 / 255.0f, 9 / 255.0f, 1.0f);
// static constexpr auto ltBrown = rgbMult(brown, 1.2f);
static constexpr auto dkBrown = rgbMult(brown, 0.73f);
static constexpr auto dkdkBrown = rgbMult(brown, 0.7f);

static constexpr auto green = ImVec4(52 / 255.0f, 89 / 255.0f, 58 / 255.0f, 1.0f);
static constexpr auto ltGreen = rgbMult(green, 1.2f);
static constexpr auto dkGreen = rgbMult(green, 0.7f);

static constexpr auto blue = ImVec4(100 / 255.0f, 138 / 255.0f, 146 / 255.0f, 1.0f);
// static constexpr auto ltBlue = rgbMult(blue, 1.2f);
// static constexpr auto dkBlue = rgbMult(blue, 0.8f);

static constexpr auto white = ImVec4(226 / 255.0f, 228 / 255.0f, 232 / 255.0f, 1.0f);

Gui::Gui(AppState& state)
    : m_state(state) {
    log_info("Creating GUI");
    update_rom_list();
    configure_ImGui();
    log_info("Finished creating GUI");
}

Gui::~Gui() {}

void Gui::update_rom_list() {

#if EZ_WASM
// todo, figure out how to load roms on wasm
#else
    m_romsAvail.clear();
    const auto romDir = "./roms/";
    for (auto& file : fs::recursive_directory_iterator(romDir)) {
        if (file.is_regular_file() && file.path().extension() == ".gb") {
            log_info("Found rom: {}", file.path().string().c_str());
            m_romsAvail.push_back(file.path());
        }
    }
    std::sort(m_romsAvail.begin(), m_romsAvail.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.filename() < rhs.filename();
    });
#endif
}

void Gui::update_op_cache() {
    m_opCache.clear();
    // cart and wram
    for (const auto& range : {iRange{0, 0x8000}, iRange{0xC000, 0xE000}}) {
        auto isPrefixedOffset = -1; // when 0 the instr is prefixed
        for (int lastOpSize, addr = range.m_min; addr < range.m_max; addr += lastOpSize) {
            auto opByte = m_state.m_emu->read_addr(uint16_t(addr));
            if (opByte == +OpCode::PREFIX) {
                isPrefixedOffset = 1;
            }
            const auto info =
                isPrefixedOffset == 0 ? get_opcode_info_prefixed(opByte) : get_opcode_info(opByte);
            m_opCache.emplace_back(addr, info);
            lastOpSize = info.m_size;
            --isPrefixedOffset;
        }
    }
}

InputState Gui::handle_keyboard() {

    auto inputState = m_inputFromOnScreenControls;
    inputState.m_a |= ImGui::IsKeyDown(ImGuiKey_Z);
    inputState.m_b |= ImGui::IsKeyDown(ImGuiKey_X);

    inputState.m_start |= ImGui::IsKeyDown(ImGuiKey_A);
    inputState.m_select |= ImGui::IsKeyDown(ImGuiKey_S);

    inputState.m_left |= ImGui::IsKeyDown(ImGuiKey_LeftArrow);
    inputState.m_right |= ImGui::IsKeyDown(ImGuiKey_RightArrow);
    inputState.m_up |= ImGui::IsKeyDown(ImGuiKey_UpArrow);
    inputState.m_down |= ImGui::IsKeyDown(ImGuiKey_DownArrow);

    if (ImGui::IsKeyPressed(ImGuiKey_R)) {
        reset_emulator();
    }
    if (ImGui::IsKeyPressed(ImGuiKey_N)) {
        if (m_state.m_isPaused) {
            m_state.m_stepToNextInstr = true;
        }
    }
    if (ImGui::IsKeyPressed(ImGuiKey_M)) {
        if (m_state.m_isPaused) {
            m_state.m_stepOneCycle = true;
        }
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
        m_state.m_isPaused = !m_state.m_isPaused;
        update_op_cache();
    }

    m_inputFromOnScreenControls = {};

    return inputState;
}

void Gui::draw() {

    if (!m_mobileLayoutDismissed) {
        const auto vpSize = ImGui::GetMainViewport()->Size;
        const auto vpAspect = vpSize.x / vpSize.y;
        static constexpr auto useMobileIfAspectLessThan = 0.6f;

        m_mobileLayout = vpAspect < useMobileIfAspectLessThan;
    }

    // reset mouse cursor
    ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

    draw_popups();

    if (m_mobileLayout) {
        draw_controls();
    } else {
        draw_toolbar();
        draw_registers();
        draw_settings();
        draw_console();
        draw_instructions();
        if (m_showPPU) {
            draw_ppu();
        }
    }
    draw_display();

    if (m_showDemoWindow) {
        ImGui::ShowDemoWindow();
    }
}

static void drawUnderline(ImColor col_) {
    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax();
    min.y = max.y;
    ImGui::GetWindowDrawList()->AddLine(min, max, col_, 1.0f);
}

static void urlText(const char* text, const char* url = nullptr) {
    ImGui::Text(text);
    if (ImGui::IsItemHovered()) {
        drawUnderline(ImGui::GetStyleColorVec4(ImGuiCol_Text));
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    } else {
    }
    if (ImGui::IsItemClicked(0)) {
        SDL_OpenURL(url ? url : text);
    }
}

void Gui::draw_popups() {
    const auto modalDismissButton = []() {
        if (ImGui::Button("Close", ImVec2(ImGui::GetContentRegionAvail().x, 0)) ||
            ImGui::IsKeyPressed(ImGuiKey_Escape) || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
            ImGui::CloseCurrentPopup();
        }
    };

    if (ImGui::BeginPopupModal("About", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("ezgb - Made by Eric Zillinger in 2024");
        urlText("https://github.com/ezillinger/ezgb");
        ImGui::Spacing();
        ImGui::Text("Windsor Road - Made by Louie Zong in 2021");
        urlText("https://everydaylouie.itch.io/windsor-road");
        ImGui::Spacing();
        modalDismissButton();
        ImGui::EndPopup();
    }
    if (m_showAboutPopup) {
        ImGui::OpenPopup("About");
        m_showAboutPopup = false;
    }

    if (ImGui::BeginPopupModal("Controls##help", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("A - Z");
        ImGui::Text("B - X");
        ImGui::Text("Start - A");
        ImGui::Text("Select - S");
        ImGui::Text("D-Pad - Arrow Keys");
        ImGui::Spacing();
        ImGui::Text("Pause/Resume - Space");
        ImGui::Text("Restart ROM - R");
        ImGui::Text("Step One Cycle - M");
        ImGui::Text("Step One Instruction - N");
        ImGui::Spacing();
        modalDismissButton();
        ImGui::EndPopup();
    }
    if (m_showControlsPopup) {
        ImGui::OpenPopup("Controls##help");
        m_showControlsPopup = false;
    }

    if (m_mobileLayout) {
        auto showMobilePopup = !m_mobileLayoutDismissed;
        ImGui::SetNextWindowSize(ImVec2(ImGui::GetMainViewport()->Size.x, 0));
        if (ImGui::BeginPopupModal("Mobile Info", &showMobilePopup)) {
            ImGui::TextWrapped("It looks like you're using a Mobile Browser in portrait mode, but "
                               "there isn't a great way to detect this in SDL yet");
            ImGui::Spacing();
            ImGui::TextWrapped("The control pad is for demo purposes only. ImGui doesn't support "
                               "multi-touch so you can only press one button at a time.");

            const auto mobileButtonHeight = 50.0f;
            if (ImGui::Button("OK", ImVec2(ImGui::GetContentRegionAvail().x, mobileButtonHeight))) {
                m_mobileLayoutDismissed = true;
            }
            if (ImGui::Button("Switch to Desktop Mode", ImVec2(ImGui::GetContentRegionAvail().x, mobileButtonHeight))) {
                m_mobileLayout = false;
                m_mobileLayoutDismissed = true;
            }
            ImGui::EndPopup();
        }

        if(!m_mobileLayoutDismissed){
            ImGui::OpenPopup("Mobile Info");
        }
    }
}

void Gui::draw_controls() {
    put_next_window({0.0f, 2.5f}, {4.0f, 1.5f});
    if (ImGui::Begin("On-Screen Controls", nullptr, getWindowFlags())) {
        const auto numCols = 6;
        const auto numRows = 3;
        const auto winDim = ImGui::GetContentRegionAvail();

        const auto cellDim = ImVec2{winDim.x / numCols, winDim.y / numRows};
        const auto padding = ImVec2{0.05f * cellDim.x, 0.1f * cellDim.y};
        const auto buttonDim = ImVec2{cellDim.x - 2.0f * padding.x, cellDim.y};
        const auto smallButtonDim = ImVec2{buttonDim.x, (buttonDim.y - 2.0f * padding.y) / 2.0f};

        const auto putCursor = [&](float col, float row) {
            ImGui::SetCursorPos(ImVec2{cellDim.x * col + padding.x, cellDim.y * row});
        };

        putCursor(1, 0);
        ImGui::Button("UP", buttonDim);
        if (ImGui::IsItemActive()) {
            m_inputFromOnScreenControls.m_up = true;
        }

        putCursor(0, 1);
        ImGui::Button("LEFT", buttonDim);
        if (ImGui::IsItemActive()) {
            m_inputFromOnScreenControls.m_left = true;
        }

        putCursor(2, 1);
        ImGui::Button("RIGHT", buttonDim);
        if (ImGui::IsItemActive()) {
            m_inputFromOnScreenControls.m_right = true;
        }

        putCursor(1, 2);
        ImGui::Button("DOWN", buttonDim);
        if (ImGui::IsItemActive()) {
            m_inputFromOnScreenControls.m_down = true;
        }

        putCursor(4, 1);
        ImGui::Button("B", buttonDim);
        if (ImGui::IsItemActive()) {
            m_inputFromOnScreenControls.m_b = true;
        }       

        putCursor(5, 1);
        ImGui::Button("A", buttonDim);
        if (ImGui::IsItemActive()) {
            m_inputFromOnScreenControls.m_a = true;
        }

        putCursor(3, 2);
        ImGui::Button("Start", smallButtonDim);
        if (ImGui::IsItemActive()) {
            m_inputFromOnScreenControls.m_start = true;
        }

        putCursor(3, 2.5f);
        ImGui::Button("Select", smallButtonDim);
        if (ImGui::IsItemActive()) {
            m_inputFromOnScreenControls.m_select = true;
        }
    }
    ImGui::End();
}

void Gui::draw_toolbar() {

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Reset")) {
                reset_emulator();
            }
            if (ImGui::BeginMenu("Load ROM...")) {
                if (ImGui::MenuItem("Refresh")) {
                    update_rom_list();
                }
                ImGui::Separator();
                for (auto& romPath : m_romsAvail) {
                    if (ImGui::MenuItem(romPath.filename().string().c_str())) {
                        m_lastRom = romPath.filename().string();
                        m_state.m_cart = std::make_unique<Cart>(Cart::load_from_disk(romPath));
                        reset_emulator();
                    }
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Exit")) {
                m_shouldExit = true;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Options")) {
            ImGui::Checkbox("Show PPU Debug Panel", &m_showPPU);
            ImGui::Checkbox("Show ImGui Demo Window", &m_showDemoWindow);
            ImGui::Checkbox("Mobile Layout (Refresh Required to Undo)", &m_mobileLayout);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
                m_showAboutPopup = true;
            }
            if (ImGui::MenuItem("Controls##menu")) {
                m_showControlsPopup = true;
            }
            ImGui::EndMenu();
        }
        const auto buttonWidth = 100.0f;
        const auto buttonSize = ImVec2{buttonWidth, 0.0f};
        const int numButtons = 3;
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2.0f - 0.5f * buttonWidth * numButtons);
        ImGui::PushItemWidth(buttonWidth);
        if (ImGui::Button(m_state.m_isPaused ? "Resume" : "Break", buttonSize)) {
            m_state.m_isPaused = !m_state.m_isPaused;
            if (m_state.m_isPaused) {
                update_op_cache();
            }
        }
        ImGui::BeginDisabled(!m_state.m_isPaused);
        if (ImGui::Button("Step Instr", buttonSize)) {
            m_state.m_stepToNextInstr = true;
            update_op_cache();
        }
        if (ImGui::Button("Step Cycle", buttonSize)) {
            m_state.m_stepOneCycle = true;
            update_op_cache();
        }
        ImGui::EndDisabled();
        ImGui::PopItemWidth();

        static Stopwatch frameTimer{};
        const auto frameTime = frameTimer.elapsed();
        const auto timeText = "{}"_format(frameTime);
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::CalcTextSize(timeText.c_str()).x);
        ImGui::Text(timeText.c_str());
        frameTimer.reset();

        ImGui::EndMainMenuBar();
    }
}

void Gui::draw_registers() {
    put_next_window({0, 0}, {1, 4});
    if (ImGui::Begin("Registers", nullptr, getWindowFlags())) {
        const auto drawReg8 = [](const char* label, uint8_t data) {
            auto copy = int(data);
            ImGui::DragInt(label, &copy, 1.0f, 0, 0, "%02x");
        };
        const auto drawReg16 = [](const char* label, uint16_t data) {
            auto copy = int(data);
            ImGui::DragInt(label, &copy, 1.0f, 0, 0, "%04x");
        };
        auto& emu = *m_state.m_emu;

        drawReg16("PC", emu.m_reg.pc);
        drawReg16("SP", emu.m_reg.sp);

        drawReg8("A", emu.m_reg.a);
        drawReg8("F", emu.m_reg.f);
        drawReg16("AF", emu.m_reg.af);

        drawReg8("B", emu.m_reg.b);
        drawReg8("C", emu.m_reg.c);
        drawReg16("BC", emu.m_reg.bc);

        drawReg8("D", emu.m_reg.d);
        drawReg8("E", emu.m_reg.e);
        drawReg16("DE", emu.m_reg.de);

        drawReg8("H", emu.m_reg.h);
        drawReg8("L", emu.m_reg.l);
        drawReg16("HL", emu.m_reg.hl);
        ImGui::Text("Flags:\n Z {}\n N {}\n H {}\n C {}"_format(emu.get_flag(Flag::ZERO),
                                                                emu.get_flag(Flag::NEGATIVE),
                                                                emu.get_flag(Flag::HALF_CARRY),
                                                                emu.get_flag(Flag::CARRY))
                        .c_str());

        ImGui::Checkbox("Stop Mode", &m_state.m_emu->m_stopMode);
        ImGui::Checkbox("Halt Mode", &m_state.m_emu->m_haltMode);
        auto brMapped = !m_state.m_emu->m_ioReg.m_bootromDisabled;
        ImGui::Checkbox("Bootrom Mapped", &brMapped);
        ImGui::Checkbox("IME", &m_state.m_emu->m_interruptMasterEnable);

        if (ImGui::CollapsingHeader("Timers", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& ioReg = m_state.m_emu->m_ioReg;
            ImGui::LabelText("T-CYCLES", "{}"_format(m_state.m_emu->get_cycle_counter()).c_str());
            ImGui::LabelText("SYSCLK", "{}"_format(m_state.m_emu->m_sysclk).c_str());
            ImGui::LabelText("DIV", "{}"_format(ioReg.m_timerDivider).c_str());
            ImGui::LabelText("TIMA Enabled", "{}"_format(bool(ioReg.m_tac & 0b100)).c_str());
            ImGui::LabelText("TIMA", "{}"_format(ioReg.m_tima).c_str());
            ImGui::LabelText("MODULO", "{}"_format(ioReg.m_tma).c_str());
        }
        if (ImGui::CollapsingHeader("Interrupts", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& ioReg = m_state.m_emu->m_ioReg;
            ImGui::Text(
                "IE/IF:\n VB      {} {}\n LCD     {} {}\n TIMER   {} {}\n SERIAL  {} {}\n JOY     {} {}"_format(
                    bool(ioReg.m_ie.vblank),
                    bool(ioReg.m_if.vblank),
                    bool(ioReg.m_ie.lcd),
                    bool(ioReg.m_if.lcd),
                    bool(ioReg.m_ie.timer),
                    bool(ioReg.m_if.timer),
                    bool(ioReg.m_ie.serial),
                    bool(ioReg.m_if.serial),
                    bool(ioReg.m_ie.joypad),
                    bool(ioReg.m_if.joypad))
                    .c_str());
        }

        if (ImGui::CollapsingHeader("Cart", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("{}"_format(m_lastRom).c_str());
            ImGui::LabelText("Size", "{}"_format(m_state.m_cart->m_sizeBytes).c_str());
            ImGui::LabelText("Type", "{}"_format(+m_state.m_cart->m_cartType).c_str());
            if (m_state.m_cart->m_cartType == CartType::MBC1) {
                ImGui::LabelText("ROM Bank",
                                 "{}"_format(m_state.m_cart->m_mbc1State.m_romBankSelect).c_str());
            }
        }
    }
    ImGui::End();
}

void Gui::clear_cache() { m_opCache.clear(); }

void Gui::reset_emulator() {
    const auto settingsCopy = m_state.m_emu->m_settings;
    m_state.m_emu = std::make_unique<Emulator>(*m_state.m_cart, settingsCopy);
    clear_cache();
}

void Gui::configure_ImGui() {

    auto& style = ImGui::GetStyle();
    style.ScrollbarSize *= 1.25f;
    style.FrameRounding = 12.0f;
    style.WindowBorderSize = 0.0f;
    style.WindowTitleAlign.x = 0.5f;

    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = white;
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
    colors[ImGuiCol_Border] = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_TitleBg] = dkBrown; // ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = dkBrown;
    //    = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = dkBrown; // ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
    colors[ImGuiCol_Button] = dkGreen;       // ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    colors[ImGuiCol_ButtonHovered] = green;  //    = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
    colors[ImGuiCol_ButtonActive] = ltGreen; // = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_Header] = dkdkBrown;
    colors[ImGuiCol_HeaderHovered] = dkBrown;
    colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
    colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = dkBrown;
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
    colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.35f);
}

void Gui::put_next_window(const float2& pos, const float2& dims) {
    auto gridSize = float2{4, 4};

    const auto vpPos = ImGui::GetMainViewport()->WorkPos;
    const auto vpSize = ImGui::GetMainViewport()->WorkSize;

    auto cellDims = float2{vpSize.x / gridSize.x, vpSize.y / gridSize.y};

    ez_assert(pos.x >= 0 && pos.x < gridSize.x);
    ez_assert(pos.y >= 0 && pos.y < gridSize.y);
    ez_assert(pos.x + dims.x <= gridSize.x);
    ez_assert(pos.y + dims.y <= gridSize.y);

    ImGui::SetNextWindowPos(ImVec2{vpPos.x + cellDims.x * pos.x, vpPos.y + cellDims.y * pos.y});
    ImGui::SetNextWindowSize(ImVec2{cellDims.x * dims.x, cellDims.y * dims.y});
}

ImGuiWindowFlags Gui::getWindowFlags() {
    return ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_HorizontalScrollbar |
           ImGuiWindowFlags_NoResize;
}

void Gui::draw_settings() {
    auto& emu = *m_state.m_emu;
    put_next_window({3, 3}, {1, 1});
    if (ImGui::Begin("Settings", nullptr, getWindowFlags())) {
        ImGui::Checkbox("Skip Bootrom", &emu.m_settings.m_skipBootROM);
        ImGui::Checkbox("Log", &emu.m_settings.m_logEnable);
        ImGui::DragInt(
            "PC Break Addr", &m_state.m_debugSettings.m_breakOnPC, 1.0f, -1, INT16_MAX, "%04x");
        ImGui::DragInt(
            "OC Break", &m_state.m_debugSettings.m_breakOnOpCode, 1.0f, -1, INT16_MAX, "%04x");
        ImGui::DragInt("OC Break Prefixed",
                       &m_state.m_debugSettings.m_breakOnOpCodePrefixed,
                       1.0f,
                       -1,
                       INT16_MAX,
                       "%04x");
        ImGui::DragInt("Break On Write Addr",
                       &m_state.m_debugSettings.m_breakOnWriteAddr,
                       1.0f,
                       -1,
                       INT16_MAX,
                       "%04x");

        ImGui::Separator();
    }
    ImGui::End();
}

void Gui::draw_console() {

    auto& emu = *m_state.m_emu;
    put_next_window({1, 3.5f}, {2, 0.5f});
    if (ImGui::Begin("Serial Console", nullptr, getWindowFlags())) {
        ImGui::TextWrapped(emu.m_serialOutput.c_str());
    }
    ImGui::End();
}

void Gui::draw_instructions() {
    const auto justPaused = update(m_prevWasPaused, m_state.m_isPaused) && m_state.m_isPaused;
    auto& emu = *m_state.m_emu;
    put_next_window({3, 0}, {1, 3});
    if (ImGui::Begin("Instructions", nullptr, getWindowFlags())) {
        std::optional<int> scrollToLine;
        if (ImGui::Button("Refresh") || m_opCache.empty() || justPaused) {
            update_op_cache();
        }
        ImGui::SameLine();
        ImGui::Checkbox("Follow PC", &m_followPC);
        ImGui::SameLine();
        if (ImGui::Button("Jump to PC") || m_followPC) {
            const auto lb =
                std::lower_bound(m_opCache.begin(),
                                 m_opCache.end(),
                                 m_state.m_emu->m_reg.pc,
                                 [](const OpLine& line, int addr) { return line.m_addr < addr; });
            scrollToLine = int(lb - m_opCache.begin());
        }
        const auto numCols = 5;
        if (ImGui::BeginTable("Memory View Table",
                              numCols,
                              ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
            ImGui::TableSetupColumn("Addr", ImGuiTableColumnFlags_None);
            ImGui::TableSetupColumn("OpCode", ImGuiTableColumnFlags_None);
            ImGui::TableSetupColumn("Mnemonic", ImGuiTableColumnFlags_None);
            ImGui::TableSetupColumn("Data", ImGuiTableColumnFlags_None);
            ImGui::TableSetupColumn("Cycles", ImGuiTableColumnFlags_None);
            ImGui::TableHeadersRow();
            ImGuiListClipper clipper;
            clipper.Begin(int(m_opCache.size()));

            while (clipper.Step()) {
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
                    ImGui::TableNextRow();
                    const auto& ocLine = m_opCache[row];
                    if (emu.m_reg.pc == ocLine.m_addr) {
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImColor(blue));
                    }
                    ImGui::TableNextColumn();
                    ImGui::Text("{:04x}"_format(ocLine.m_addr).c_str());

                    ImGui::TableNextColumn();
                    ImGui::Text("{}{:02x}"_format(ocLine.m_info.m_prefixed ? "cb " : "",
                                                  ocLine.m_info.m_addr)
                                    .c_str());

                    ImGui::TableNextColumn();
                    ImGui::Text("{} {} {}"_format(ocLine.m_info.m_mnemonic,
                                                  ocLine.m_info.m_operandName1,
                                                  ocLine.m_info.m_operandName2)
                                    .c_str());

                    ImGui::TableNextColumn();
                    const auto hasOperand = [&](const char* txt) {
                        return strcmp(ocLine.m_info.m_operandName1, txt) == 0 ||
                               strcmp(ocLine.m_info.m_operandName2, txt) == 0;
                    };
                    auto operandText = std::string();
                    if (hasOperand("u16")) {
                        const auto u16 = m_state.m_emu->readAddr16(uint16_t(ocLine.m_addr + 1));
                        operandText += "u16={:04x} "_format(u16);
                    }
                    if (hasOperand("a16") || hasOperand("(a16)")) {
                        const auto u16 = m_state.m_emu->readAddr16(uint16_t(ocLine.m_addr + 1));
                        operandText += "a16={:04x} "_format(u16);
                    }
                    if (hasOperand("a8") || hasOperand("(a8)")) {
                        const uint16_t a8 =
                            0xFF00 + m_state.m_emu->read_addr(uint16_t(ocLine.m_addr + 1));
                        operandText += "a8={:04x} "_format(a8);
                    }
                    if (hasOperand("(a16)")) {
                        const auto u16 = m_state.m_emu->readAddr16(uint16_t(ocLine.m_addr + 1));
                        const auto a16deref = m_state.m_emu->read_addr(u16);
                        operandText += "(a16)={:02x} "_format(a16deref);
                    }
                    if (hasOperand("(a8)")) {
                        const uint16_t a8 =
                            0xFF00 + m_state.m_emu->read_addr(uint16_t(ocLine.m_addr + 1));
                        const auto a8deref = m_state.m_emu->read_addr(a8);
                        operandText += "(a8)={:02x} "_format(a8deref);
                    }
                    if (hasOperand("i8")) {
                        const auto i8 = static_cast<int8_t>(
                            m_state.m_emu->read_addr(uint16_t(ocLine.m_addr + 1)));
                        operandText += "i8={:02x} "_format(i8);
                    }
                    if (hasOperand("u8")) {
                        const auto u8 = m_state.m_emu->read_addr(uint16_t(ocLine.m_addr + 1));
                        operandText += "u8={:02x} "_format(u8);
                    }
                    ImGui::Text(operandText.c_str());
                    ImGui::TableNextColumn();
                    auto cycleText = ocLine.m_info.m_cyclesIfBranch
                                         ? "{}/{}"_format(ocLine.m_info.m_cycles,
                                                          *ocLine.m_info.m_cyclesIfBranch)
                                         : "{}"_format(ocLine.m_info.m_cycles);
                    ImGui::Text(cycleText.c_str());
                }
            }
            if (scrollToLine) {
                ImGui::SetScrollY(std::max(*scrollToLine - 2, 0) * clipper.ItemsHeight);
            }
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

void Gui::draw_display() {
    const auto left = m_mobileLayout ? 0.0f : 1.0f;
    const auto width = m_mobileLayout ? 4.0f : 2.0f;
    if (m_showPPU) {
        put_next_window({left, 0}, {width, 2});
    } else {
        const auto height = m_mobileLayout ? 2.5f : 3.5f;
        put_next_window({left, 0}, {width, height});
    }
    if (ImGui::Begin("Display",
                     nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
                         ImGuiWindowFlags_NoResize)) {
        // todo, use pixel buffer to upload
        const auto dims = m_displayTex.dim();
        m_displayTex.update(m_state.m_emu->get_display_framebuffer());

        const auto vpDim = ImGui::GetContentRegionAvail();
        const auto imageAr = float(dims.x) / dims.y;
        const auto vpAr = vpDim.x / vpDim.y;
        const auto imageDims = vpAr > imageAr ? ImVec2{vpDim.y * imageAr, vpDim.y}
                                              : ImVec2{vpDim.x, vpDim.x / imageAr};
        const auto paddingSize = ImVec2{std::max(0.0f, vpDim.x - imageDims.x) / 2.0f,
                                        std::max(0.0f, vpDim.y - imageDims.y) / 2.0f};
        ImGui::Dummy(paddingSize);
        if (paddingSize.x > 0) {
            ImGui::SameLine();
        }
        imguiImage(m_displayTex, imageDims);
    }
    ImGui::End();
}

void Gui::draw_ppu() {
    put_next_window({1, 2}, {2, 1.5});
    if (ImGui::Begin("PPU", nullptr, getWindowFlags())) {
        const auto dimAvail = ImGui::GetContentRegionAvail();
        const auto imgDim = ImVec2{dimAvail.x / 2, dimAvail.y};
        {
            const auto data = m_ppuDisplayWindow ? m_state.m_emu->get_window_dbg_framebuffer()
                                                 : m_state.m_emu->get_bg_dbg_framebuffer();
            m_bgWindowTex.update(data);

            imguiImage(m_bgWindowTex, imgDim);
            ImGui::SetItemTooltip("Click to switch between display of Window and BG");
            if (ImGui::IsItemClicked()) {
                m_ppuDisplayWindow = !m_ppuDisplayWindow;
            }
        }
        ImGui::SameLine();
        {
            m_vramTex.update(m_state.m_emu->get_vram_dbg_framebuffer());
            imguiImage(m_vramTex, imgDim);
            ImGui::SetItemTooltip("Tiles currently in VRAM");
        }
    }
    ImGui::End();
}

void imguiImage(const Tex2D& tex, const ImVec2& imageSize) {
    ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<intptr_t>(tex.handle())), imageSize);
}

} // namespace ez