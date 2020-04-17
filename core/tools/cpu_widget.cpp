#include <stdafx.hpp>
#include "cpu_widget.hpp"
#include "debugger.hpp"
#include <memory/bus.h>
#include <cpu/cpu.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>

void CPUWidget::execute()
{
    auto& cpu = debugger->bus->cpu;

    ImGui::Begin(name.c_str());

    ImGui::Text("Actions");
    ImGui::Separator();

    if (ImGui::Button("Trace")) {
        cpu->should_log = !cpu->should_log;
    }
    ImGui::SameLine();

    ImGui::PushID(1);
    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0, 0.6f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0, 0.7f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0, 0.8f, 0.8f));

    if (ImGui::Button("Break")) {
        cpu->break_on_next_tick();
    }
    ImGui::PopStyleColor(3);
    ImGui::PopID();

    ImGui::SameLine();

    ImGui::PushID(2);
    ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(2.7f / 7.0f, 0.6f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(2.7f / 7.0f, 0.7f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(2.7f / 7.0f, 0.8f, 0.8f));

    if (ImGui::Button("Disasm")) {

    }

    ImGui::PopStyleColor(3);
    ImGui::PopID();

    ImGui::Text("Program Counter");
    ImGui::Separator();

    ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(10, 0));
    ImGui::TextColored(ImVec4(0.219f, 0.964f, 0.054f, 1.0f), "PC: ");
    ImGui::SameLine(ImGui::GetWindowWidth() * 0.17f);
    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.25f);
    ImGui::DragScalar("##pc", ImGuiDataType_U32, &cpu->pc, 1.0f, 0, 0, "%08x");

    ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(10, 0));
    ImGui::TextColored(ImVec4(0.968f, 0.941f, 0.149f, 1.0f), "HI: ");
    ImGui::SameLine(ImGui::GetWindowWidth() * 0.17f);
    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.25f);
    ImGui::DragScalar("##hi", ImGuiDataType_U32, &cpu->hi, 1.0f, 0, 0, "%08x");

    ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(10, 0));
    ImGui::TextColored(ImVec4(0.968f, 0.941f, 0.149f, 1.0f), "LO: ");
    ImGui::SameLine(ImGui::GetWindowWidth() * 0.17f);
    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.25f);
    ImGui::DragScalar("##lo", ImGuiDataType_U32, &cpu->lo, 1.0f, 0, 0, "%08x");
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Show All")) {

        for (int i = 0; i < 32; i += 2) {
            auto color1 = (ImVec4)ImColor::HSV(0, 0.8f, 0.8f);
            ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(10, 0));
            ImGui::TextColored(color1, "$%s: ", reg_table[i].c_str());
            ImGui::SameLine(ImGui::GetWindowWidth() * 0.17f);
            ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.25f);
            ImGui::DragScalar(("##" + reg_table[i]).c_str(), ImGuiDataType_U32, &cpu->registers[i], 1.0f, 0, 0, "%08x");

            ImGui::SameLine();
            auto color2 = (ImVec4)ImColor::HSV(1 / 7.0f, 0.8f, 0.8f);
            ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(30, 0));

            ImGui::TextColored(color2, "$%s: ", reg_table[i + 1].c_str());
            ImGui::SameLine(ImGui::GetWindowWidth() * 0.65f);
            ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.25f);
            ImGui::DragScalar(("##" + reg_table[i + 1]).c_str(), ImGuiDataType_U32, &cpu->registers[i + 1], 1.0f, 0, 0, "%08x");
        }
    }

    ImGui::End();
}
