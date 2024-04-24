#include "Gui.h"
#include <filesystem>
namespace ez {

Gui::Gui(AppState& state) : m_state(state) { updateRomList(); }

void Gui::updateRomList() {
    const auto romDir = "./roms/";
    for (auto& file : fs::directory_iterator(romDir)) {
        if (file.is_regular_file() && file.path().extension() == ".gb") {
            log_info("Found rom: {}", file.path().c_str());
            m_romsAvail.push_back(file.path());
        }
    }
    std::sort(m_romsAvail.begin(), m_romsAvail.end());
}

void Gui::updateOpCache() {
    m_opCache.clear();
    // cart and wram
    for (const auto& range : {iRange{0, 0x8000}, iRange{0xC000, 0xE000}}) {
        auto isPrefixedOffset = -1; // when 0 the instr is prefixed
        for (int lastOpSize, addr = range.m_min; addr < range.m_max; addr += lastOpSize) {
            auto opByte = m_state.m_emu->readAddr(addr);
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

void Gui::handleKeyboard() {
    if(ImGui::IsKeyPressed(ImGuiKey_N)){
        if(m_state.m_isPaused){
            m_state.m_singleStep = true;
        }
    }
    if(ImGui::IsKeyPressed(ImGuiKey_Space)){
        m_state.m_isPaused = !m_state.m_isPaused;
    }
}

void Gui::drawGui() {
    drawToolbar();
    drawRegisters();
    drawSettings();
    drawConsole();
    drawInstructions();
    drawDisplay();

    if (m_showDemoWindow) {
        ImGui::ShowDemoWindow();
    }
}

void Gui::drawToolbar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Reset")) {
                const auto settingsCopy = m_state.m_emu->m_settings;
                m_state.m_emu = std::make_unique<Emulator>(*m_state.m_cart, settingsCopy);
                clearCache();
            }
            if (ImGui::BeginMenu("Load ROM...")) {
                if (ImGui::MenuItem("Refresh")) {
                    updateRomList();
                }
                ImGui::Separator();
                for (auto& romPath : m_romsAvail) {
                    if (ImGui::MenuItem(romPath.c_str())) {
                        const auto settingsCopy = m_state.m_emu->m_settings;
                        m_state.m_cart = std::make_unique<Cart>(romPath);
                        m_state.m_emu = std::make_unique<Emulator>(*m_state.m_cart, settingsCopy);
                        clearCache();
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
            if (ImGui::Button("Play")) {
                m_state.m_isPaused = false;
            }
            if (ImGui::Button("Step")) {
                m_state.m_singleStep = true;
            }
        } else {
            if (ImGui::Button("Pause")) {
                m_state.m_isPaused = true;
            }
        }

        ImGui::EndMainMenuBar();
    }
}

void Gui::drawRegisters() {
    if (ImGui::Begin("Registers")) {
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
        auto brMapped = m_state.m_emu->m_io.isBootromMapped();
        ImGui::Checkbox("Bootrom Mapped", &brMapped);

        if (ImGui::CollapsingHeader("Cart")) {
            ImGui::LabelText("Size", "{}"_format(m_state.m_cart->m_sizeBytes).c_str());
            ImGui::LabelText("Type", "{}"_format(+m_state.m_cart->m_cartType).c_str());
        }
    }
    ImGui::End();
}

void Gui::clearCache() { m_opCache.clear(); }

void Gui::drawSettings() {
    auto& emu = *m_state.m_emu;
    if (ImGui::Begin("Settings")) {
        ImGui::Checkbox("Skip Bootrom", &emu.m_settings.m_skipBootROM);
        ImGui::Checkbox("Auto Un-Stop", &emu.m_settings.m_autoUnStop);
        ImGui::Checkbox("Log", &emu.m_settings.m_logEnable);
        ImGui::Checkbox("No Wait", &emu.m_settings.m_runAsFastAsPossible);
        ImGui::DragInt("PC Break Addr", &m_state.m_debugSettings.m_breakOnPC, 1.0f, -1, INT16_MAX,
                       "%04x");
        ImGui::DragInt("OC Break", &m_state.m_debugSettings.m_breakOnOpCode, 1.0f, -1, INT16_MAX,
                       "%04x");
        ImGui::DragInt("OC Break Prefixed", &m_state.m_debugSettings.m_breakOnOpCodePrefixed, 1.0f,
                       -1, INT16_MAX, "%04x");

        ImGui::Separator();
    }
    ImGui::End();
}

void Gui::drawConsole() {

    auto& emu = *m_state.m_emu;
    if (ImGui::Begin("Serial Console", nullptr)) {
        ImGui::TextWrapped(emu.m_io.getSerialOutput().c_str());
    }
    ImGui::End();
}

void Gui::drawInstructions() {

    auto& emu = *m_state.m_emu;
    if (ImGui::Begin("Instructions", nullptr)) {
        std::optional<int> scrollToLine;
        if (ImGui::Button("Refresh") || m_opCache.empty()) {
            updateOpCache();
        }
        ImGui::SameLine();
        ImGui::Checkbox("Follow PC", &m_followPC);
        ImGui::SameLine();
        if (ImGui::Button("Jump to PC") || m_followPC) {
            const auto lb =
                std::lower_bound(m_opCache.begin(), m_opCache.end(), m_state.m_emu->m_reg.pc,
                                 [](const OpLine& line, int addr) { return line.m_addr < addr; });
            scrollToLine = lb - m_opCache.begin();
        }
        const auto numCols = 4;
        if (ImGui::BeginTable("Memory View Table", numCols, ImGuiTableFlags_ScrollY)) {
            ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
            ImGui::TableSetupColumn("Addr", ImGuiTableColumnFlags_None);
            ImGui::TableSetupColumn("OpCode", ImGuiTableColumnFlags_None);
            ImGui::TableSetupColumn("Mnemonic", ImGuiTableColumnFlags_None);
            ImGui::TableSetupColumn("Data", ImGuiTableColumnFlags_None);
            ImGui::TableHeadersRow();
            ImGuiListClipper clipper;
            clipper.Begin(m_opCache.size());

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
                    ImGui::Text("{}{:02x}"_format(ocLine.m_info.m_prefixed ? "cb " : "", ocLine.m_info.m_addr).c_str());

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
                        const auto u16 = m_state.m_emu->readAddr16(ocLine.m_addr + 1);
                        operandText += "u16={:04x} "_format(u16);
                    } 
                    if (hasOperand("a16") || hasOperand("(a16)")) {
                        const auto u16 = m_state.m_emu->readAddr16(ocLine.m_addr + 1);
                        operandText += "a16={:04x} "_format(u16);
                    } 
                    if (hasOperand("a8") || hasOperand("(a8)")) {
                        const auto a8 = 0xFF00 + m_state.m_emu->readAddr(ocLine.m_addr + 1);
                        operandText += "a8={:04x} "_format(a8);
                    }
                    if (hasOperand("(a16)")) {
                        const auto u16 = m_state.m_emu->readAddr16(ocLine.m_addr + 1);
                        const auto a16deref = m_state.m_emu->readAddr(u16);
                        operandText += "(a16)={:02x} "_format(a16deref);
                    } 
                    if (hasOperand("(a8)")) {
                        const auto a8 = 0xFF00 + m_state.m_emu->readAddr(ocLine.m_addr + 1);
                        const auto a8deref = m_state.m_emu->readAddr(a8);
                        operandText += "(a8)={:02x} "_format(a8deref);
                    }
                    if (hasOperand("i8")) {
                        const auto i8 =
                            static_cast<int8_t>(m_state.m_emu->readAddr(ocLine.m_addr + 1));
                        operandText += "i8={:02x} "_format(i8);
                    } 
                    if (hasOperand("u8")) {
                        const auto u8 = m_state.m_emu->readAddr(ocLine.m_addr + 1);
                        operandText += "u8={:02x} "_format(u8);
                    }
                    ImGui::Text(operandText.c_str());
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

    if (ImGui::Begin("Display", nullptr)) {
        const auto& io = ImGui::GetIO();
        ImGui::Image(io.Fonts->TexID, ImGui::GetContentRegionAvail());
    }
    ImGui::End();
}

} // namespace ez