#include "Gui.h"
#include <filesystem>

namespace ez {

Gui::Gui(AppState& state) : m_state(state) {
    updateRomList();

    configureImGui();
}

Gui::~Gui() {}

void Gui::updateRomList() {
    m_romsAvail.clear();
    const auto romDir = "./roms/";
    for (auto& file : fs::recursive_directory_iterator(romDir)) {
        if (file.is_regular_file() && file.path().extension() == ".gb") {
            log_info("Found rom: {}", file.path().string().c_str());
            m_romsAvail.push_back(file.path());
        }
    }
    std::sort(m_romsAvail.begin(), m_romsAvail.end(),
              [](const auto& lhs, const auto& rhs) { return lhs.filename() < rhs.filename(); });
}

void Gui::updateOpCache() {
    m_opCache.clear();
    // cart and wram
    for (const auto& range : {iRange{0, 0x8000}, iRange{0xC000, 0xE000}}) {
        auto isPrefixedOffset = -1; // when 0 the instr is prefixed
        for (int lastOpSize, addr = range.m_min; addr < range.m_max; addr += lastOpSize) {
            auto opByte = m_state.m_emu->readAddr(uint16_t(addr));
            if (opByte == +OpCode::PREFIX) {
                isPrefixedOffset = 1;
            }
            const auto info = isPrefixedOffset == 0 ? getOpCodeInfoPrefixed(opByte)
                                                    : getOpCodeInfoUnprefixed(opByte);
            m_opCache.emplace_back(addr, info);
            lastOpSize = info.m_size;
            --isPrefixedOffset;
        }
    }
}

JoypadState Gui::handleKeyboard() {

    auto inputState = JoypadState{};
    inputState.m_a = ImGui::IsKeyDown(ImGuiKey_Z);
    inputState.m_b = ImGui::IsKeyDown(ImGuiKey_X);

    inputState.m_start = ImGui::IsKeyDown(ImGuiKey_A);
    inputState.m_select = ImGui::IsKeyDown(ImGuiKey_S);

    inputState.m_left = ImGui::IsKeyDown(ImGuiKey_LeftArrow);
    inputState.m_right = ImGui::IsKeyDown(ImGuiKey_RightArrow);
    inputState.m_up = ImGui::IsKeyDown(ImGuiKey_UpArrow);
    inputState.m_down = ImGui::IsKeyDown(ImGuiKey_DownArrow);

    if (ImGui::IsKeyPressed(ImGuiKey_R)) {
        resetEmulator();
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
        updateOpCache();
    }

    return inputState;
}

void Gui::drawGui() {
    drawToolbar();
    drawRegisters();
    drawSettings();
    drawConsole();
    drawInstructions();
    drawDisplay();
    drawPPU();

    if (m_showDemoWindow) {
        ImGui::ShowDemoWindow();
    }
}

void Gui::drawToolbar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Reset")) {
                resetEmulator();
            }
            if (ImGui::BeginMenu("Load ROM...")) {
                if (ImGui::MenuItem("Refresh")) {
                    updateRomList();
                }
                ImGui::Separator();
                for (auto& romPath : m_romsAvail) {
                    if (ImGui::MenuItem(romPath.filename().string().c_str())) {
                        m_lastRom = romPath.filename().string();
                        m_state.m_cart = std::make_unique<Cart>(Cart::loadFromDisk(romPath));
                        resetEmulator();
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
            ImGui::Checkbox("Show Demo Window", &m_showDemoWindow);
            ImGui::EndMenu();
        }
        if (m_state.m_isPaused) {
            ImGui::Dummy(ImVec2(ImGui::GetWindowWidth() * 0.3f, 0));
            if (ImGui::Button("Play")) {
                m_state.m_isPaused = false;
            }
            if (ImGui::Button("Next Instr")) {
                m_state.m_stepToNextInstr = true;
                updateOpCache();
            }
            if (ImGui::Button("Step")) {
                m_state.m_stepOneCycle = true;
                updateOpCache();
            }
        } else {
            ImGui::Dummy(ImVec2(ImGui::GetWindowWidth() * 0.35f, 0));
            if (ImGui::Button("Pause")) {
                m_state.m_isPaused = true;
                updateOpCache();
            }
        }

        ImGui::Text("{}"_format(m_lastRom).c_str());

        static Stopwatch frameTimer{};
        const auto frameTime = frameTimer.elapsed();
        const auto timeText = "{}"_format(frameTime);
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::CalcTextSize(timeText.c_str()).x);
        ImGui::Text(timeText.c_str());
        frameTimer.reset();

        ImGui::EndMainMenuBar();
    }
}

void Gui::drawRegisters() {
    putNextWindow({0, 0}, {1, 4});
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
        ImGui::Text("Flags:\n Z {}\n N {}\n H {}\n C {}"_format(
                        emu.getFlag(Flag::ZERO), emu.getFlag(Flag::NEGATIVE),
                        emu.getFlag(Flag::HALF_CARRY), emu.getFlag(Flag::CARRY))
                        .c_str());

        ImGui::Checkbox("Stop Mode", &m_state.m_emu->m_stopMode);
        ImGui::Checkbox("Halt Mode", &m_state.m_emu->m_haltMode);
        auto brMapped = !m_state.m_emu->m_ioReg.m_bootromDisabled;
        ImGui::Checkbox("Bootrom Mapped", &brMapped);
        ImGui::Checkbox("IME", &m_state.m_emu->m_interruptMasterEnable);

        if (ImGui::CollapsingHeader("Timers", ImGuiTreeNodeFlags_DefaultOpen)) {
            auto& ioReg = m_state.m_emu->m_ioReg;
            ImGui::LabelText("T-CYCLES", "{}"_format(m_state.m_emu->getCycleCounter()).c_str());
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
                    bool(ioReg.m_ie.vblank), bool(ioReg.m_if.vblank), bool(ioReg.m_ie.lcd),
                    bool(ioReg.m_if.lcd), bool(ioReg.m_ie.timer), bool(ioReg.m_if.timer),
                    bool(ioReg.m_ie.serial), bool(ioReg.m_if.serial), bool(ioReg.m_ie.joypad),
                    bool(ioReg.m_if.joypad))
                    .c_str());
        }

        if (ImGui::CollapsingHeader("Cart", ImGuiTreeNodeFlags_DefaultOpen)) {
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

void Gui::clearCache() { m_opCache.clear(); }

void Gui::resetEmulator() {
    const auto settingsCopy = m_state.m_emu->m_settings;
    m_state.m_emu = std::make_unique<Emulator>(*m_state.m_cart, settingsCopy);
    clearCache();
}

void Gui::configureImGui() {
    auto& style = ImGui::GetStyle();
    style.ScrollbarSize *= 1.25f;
}

void Gui::putNextWindow(const float2& pos, const float2& dims) {
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
    return ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_HorizontalScrollbar;
}

void Gui::drawSettings() {
    auto& emu = *m_state.m_emu;
    putNextWindow({3, 3}, {1, 1});
    if (ImGui::Begin("Settings", nullptr, getWindowFlags())) {
        ImGui::Checkbox("Skip Bootrom", &emu.m_settings.m_skipBootROM);
        ImGui::Checkbox("Log", &emu.m_settings.m_logEnable);
        ImGui::DragInt("PC Break Addr", &m_state.m_debugSettings.m_breakOnPC, 1.0f, -1, INT16_MAX,
                       "%04x");
        ImGui::DragInt("OC Break", &m_state.m_debugSettings.m_breakOnOpCode, 1.0f, -1, INT16_MAX,
                       "%04x");
        ImGui::DragInt("OC Break Prefixed", &m_state.m_debugSettings.m_breakOnOpCodePrefixed, 1.0f,
                       -1, INT16_MAX, "%04x");
        ImGui::DragInt("Break On Write Addr", &m_state.m_debugSettings.m_breakOnWriteAddr, 1.0f, -1,
                       INT16_MAX, "%04x");

        ImGui::Separator();
    }
    ImGui::End();
}

void Gui::drawConsole() {

    auto& emu = *m_state.m_emu;
    putNextWindow({1, 3.5f}, {2, 0.5f});
    if (ImGui::Begin("Serial Console", nullptr, getWindowFlags())) {
        ImGui::TextWrapped(emu.m_serialOutput.c_str());
    }
    ImGui::End();
}

void Gui::drawInstructions() {
    const auto justPaused = update(m_prevWasPaused, m_state.m_isPaused) && m_state.m_isPaused;
    auto& emu = *m_state.m_emu;
    putNextWindow({3, 0}, {1, 3});
    if (ImGui::Begin("Instructions", nullptr, getWindowFlags())) {
        std::optional<int> scrollToLine;
        if (ImGui::Button("Refresh") || m_opCache.empty() || justPaused) {
            updateOpCache();
        }
        ImGui::SameLine();
        ImGui::Checkbox("Follow PC", &m_followPC);
        ImGui::SameLine();
        if (ImGui::Button("Jump to PC") || m_followPC) {
            const auto lb =
                std::lower_bound(m_opCache.begin(), m_opCache.end(), m_state.m_emu->m_reg.pc,
                                 [](const OpLine& line, int addr) { return line.m_addr < addr; });
            scrollToLine = int(lb - m_opCache.begin());
        }
        const auto numCols = 5;
        if (ImGui::BeginTable("Memory View Table", numCols,
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
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, 0xFF0000FF);
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
                        const uint16_t a8 = 0xFF00 + m_state.m_emu->readAddr(uint16_t(ocLine.m_addr + 1));
                        operandText += "a8={:04x} "_format(a8);
                    }
                    if (hasOperand("(a16)")) {
                        const auto u16 = m_state.m_emu->readAddr16(uint16_t(ocLine.m_addr + 1));
                        const auto a16deref = m_state.m_emu->readAddr(u16);
                        operandText += "(a16)={:02x} "_format(a16deref);
                    }
                    if (hasOperand("(a8)")) {
                        const uint16_t a8 = 0xFF00 + m_state.m_emu->readAddr(uint16_t(ocLine.m_addr + 1));
                        const auto a8deref = m_state.m_emu->readAddr(a8);
                        operandText += "(a8)={:02x} "_format(a8deref);
                    }
                    if (hasOperand("i8")) {
                        const auto i8 =
                            static_cast<int8_t>(m_state.m_emu->readAddr(uint16_t(ocLine.m_addr + 1)));
                        operandText += "i8={:02x} "_format(i8);
                    }
                    if (hasOperand("u8")) {
                        const auto u8 = m_state.m_emu->readAddr(uint16_t(ocLine.m_addr + 1));
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

void Gui::drawDisplay() {
    putNextWindow({1, 0}, {2, 2});
    if (ImGui::Begin("Display", nullptr, getWindowFlags())) {
        // todo, use pixel buffer to upload
        const auto dims = m_displayTex.dim();
        m_displayTex.update(m_state.m_emu->getDisplayFramebuffer());

        const auto vpDim = ImGui::GetContentRegionAvail();
        const auto imageAr = float(dims.x) / dims.y;
        const auto vpAr = vpDim.x / vpDim.y;
        const auto imageDims = vpAr > imageAr ? ImVec2{vpDim.y * imageAr, vpDim.y}
                                              : ImVec2{vpDim.x, vpDim.x / imageAr};
        const auto paddingSize = ImVec2{std::max(0.0f, vpDim.x - imageDims.x) / 2.0f,
                                        std::max(0.0f, vpDim.y - imageDims.y) / 2.0f};
        ImGui::Dummy(paddingSize);
        if(paddingSize.x > 0){
            ImGui::SameLine();
        }
        imguiImage(m_displayTex, imageDims);
    }
    ImGui::End();
}

void Gui::drawPPU() {
    putNextWindow({1, 2}, {2, 1.5});
    if (ImGui::Begin("PPU", nullptr, getWindowFlags())) {
        const auto dimAvail = ImGui::GetContentRegionAvail();
        const auto imgDim = ImVec2{dimAvail.x / 2, dimAvail.y};
        {
            const auto data = m_ppuDisplayWindow ? m_state.m_emu->getWindowDebugFramebuffer()
                                                 : m_state.m_emu->getBgDebugFramebuffer();
            m_bgWindowTex.update(data);

            imguiImage(m_bgWindowTex, imgDim);
            ImGui::SetItemTooltip("Click to switch between Window and BG");
            if (ImGui::IsItemClicked()) {
                m_ppuDisplayWindow = !m_ppuDisplayWindow;
            }
        }
        ImGui::SameLine();
        // todo, use pixel buffer to upload
        {
            m_vramTex.update(m_state.m_emu->getVramDebugFramebuffer());
            imguiImage(m_vramTex, imgDim);
        }
    }
    ImGui::End();
}

void imguiImage(const Tex2D& tex, const ImVec2& imageSize) {
    ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<intptr_t>(tex.handle())), imageSize);
}

} // namespace ez