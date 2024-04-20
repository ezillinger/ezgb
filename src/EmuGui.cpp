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

        drawReg16("PC", m_emu.m_reg.pc);
        drawReg16("SP", m_emu.m_reg.sp);

        drawReg8("A", m_emu.m_reg.a);
        drawReg8("F", m_emu.m_reg.f);
        drawReg16("AF", m_emu.m_reg.af);

        drawReg8("B", m_emu.m_reg.b);
        drawReg8("C", m_emu.m_reg.c);
        drawReg16("BC", m_emu.m_reg.bc);

        drawReg8("D", m_emu.m_reg.d);
        drawReg8("E", m_emu.m_reg.e);
        drawReg16("DE", m_emu.m_reg.de);

        drawReg8("H", m_emu.m_reg.h);
        drawReg8("L", m_emu.m_reg.l);
        drawReg16("HL", m_emu.m_reg.hl);
        ImGui::Text("Flags:\n Z {}\n N {}\n H {}\n C {}"_format(m_emu.getFlag(Flag::ZERO), m_emu.getFlag(Flag::NEGATIVE),
                                                m_emu.getFlag(Flag::HALF_CARRY), m_emu.getFlag(Flag::CARRY)).c_str());
    }
    ImGui::End();
}

void EmuGui::drawSettings() {
    if(ImGui::Begin("Settings")){
        ImGui::Checkbox("Log", &m_emu.m_settings.m_logEnable);
        ImGui::Checkbox("No Wait", &m_emu.m_settings.m_runAsFastAsPossible);
        ImGui::DragInt("PC Break Addr", &m_emu.m_settings.m_breakOnPC, 1.0f, -1, INT16_MAX, "%04x");
        ImGui::DragInt("OC Break", &m_emu.m_settings.m_breakOnOpCode, 1.0f, -1, INT16_MAX, "%04x");
        ImGui::DragInt("OC Break Prefixed", &m_emu.m_settings.m_breakOnOpCodePrefixed, 1.0f, -1, INT16_MAX, "%04x");
    }
    ImGui::End();
}
}