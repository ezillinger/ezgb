#include "EmuGui.h"

namespace ez {
void EmuGui::drawGui() { 
    drawToolbar();
    drawRegisters();
    drawSettings();
}

void EmuGui::drawToolbar() {
    if(ImGui::BeginMainMenuBar()){
        if(ImGui::BeginMenu("File")){
            if(ImGui::MenuItem("Exit", "x", nullptr)){
                m_shouldExit = true;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void EmuGui::drawRegisters() {
    if(ImGui::Begin("Registers")){
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
        ImGui::Text("Flags:\n Z {}\n N {}\n H {}\n C {}"_format(emu.getFlag(Flag::ZERO), emu.getFlag(Flag::NEGATIVE),
                                                emu.getFlag(Flag::HALF_CARRY), emu.getFlag(Flag::CARRY)).c_str());
    }
    ImGui::End();
}

void EmuGui::drawSettings() {
    auto& emu = *m_state.m_emu;
    if (ImGui::Begin("Settings")) {

        if(ImGui::Button("Reset")){
            m_state.m_emu = std::make_unique<Emulator>(*m_state.m_cart);
        }
        
        ImGui::Separator();

        ImGui::Checkbox("Log", &emu.m_settings.m_logEnable);
        ImGui::Checkbox("No Wait", &emu.m_settings.m_runAsFastAsPossible);
        ImGui::DragInt("PC Break Addr", &emu.m_settings.m_breakOnPC, 1.0f, -1, INT16_MAX, "%04x");
        ImGui::DragInt("OC Break", &emu.m_settings.m_breakOnOpCode, 1.0f, -1, INT16_MAX, "%04x");
        ImGui::DragInt("OC Break Prefixed", &emu.m_settings.m_breakOnOpCodePrefixed, 1.0f, -1, INT16_MAX, "%04x");
        

        
        }
    ImGui::End();
}
}