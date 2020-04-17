#include "stdafx.hpp"
#include "gpu_widget.hpp"
#include "debugger.hpp"
#include <memory/bus.h>
#include <imgui.h>

GPUWidget::GPUWidget(Debugger* _debugger) :
    Widget("GPU Status"), debugger(_debugger)
{
    auto& status = debugger->bus->gpu->status;

    /*rows =
    {
        { "GPUSTAT.0-3", "Texture page X Base", &status.page_base_x },
        { "GPUSTAT.4", "Texture page Y Base", "0" },
        { "GPUSTAT.5-6", "Semi Transparency", "0" },
        { "GPUSTAT.7-8", "Texture page colors", "0" },
        { "GPUSTAT.9", "Dither 24bit to 15bit", "0" },
        { "GPUSTAT.10", "Drawing to display area", "0" },
        { "GPUSTAT.11", "Set Mask-bit", "0" },
        { "GPUSTAT.12", "Draw Pixels", "0" },
        { "GPUSTAT.13", "Interlace Field ", "0" },
        { "GPUSTAT.14", "Reverseflag", "0" },
        { "GPUSTAT.15", "Texture Disable", "0" },
        { "GPUSTAT.16", "Horizontal Resolution 2", "0" },
        { "GPUSTAT.17-18", "Horizontal Resolution 1", "0" },
        { "GPUSTAT.19", "Vertical Resolution", "0" },
        { "GPUSTAT.20", "Video Mode", "0" },
        { "GPUSTAT.21", "Display Area Color Depth", "0" },
        { "GPUSTAT.22", "Vertical Interlace", "0" },
        { "GPUSTAT.23", "Display Enable", "0" },
        { "GPUSTAT.24", "Interrupt Request (IRQ1)", "0" },
        { "GPUSTAT.25", "DMA / Data Request", "0" },
        { "GPUSTAT.26", "Ready to receive Cmd Word", "0" },
        { "GPUSTAT.27", "Ready to send VRAM to CPU", "0" },
        { "GPUSTAT.28", "Ready to receive DMA Block", "0" },
        { "GPUSTAT.29-30", "DMA Direction", "0" },
        { "GPUSTAT.31", "Drawing even/odd lines in interlace mode", "0" },
    };*/
}

void GPUWidget::execute()
{
    /*ImGui::Begin(name.c_str());
    if (ImGui::BeginTabBar("GPU Registers")) {

        if (ImGui::BeginTabItem("GPUSTAT")) {
            ImGui::Columns(3, "columns1");
            ImGui::Separator();
            ImGui::Text("Bits"); ImGui::NextColumn();
            ImGui::Text("Description"); ImGui::NextColumn();
            ImGui::Text("Value"); ImGui::NextColumn();
            ImGui::Separator();

            for (auto& r : rows)
            {
                ImGui::Text(r.bits.c_str()); ImGui::NextColumn();
                ImGui::Text(r.description.c_str()); ImGui::NextColumn();
                ImGui::Text(r.value.c_str()); ImGui::NextColumn();

                if (r.bits != "GPUSTAT.31") ImGui::Separator();
            }
            ImGui::Columns();
            ImGui::Separator();
            ImGui::TreePop();
        }

        if (ImGui::BeginTabItem("GP0")) {
            ImGui::Columns(3, "columns2");
            ImGui::Separator();
            ImGui::Text("Bits"); ImGui::NextColumn();
            ImGui::Text("Description"); ImGui::NextColumn();
            ImGui::Text("Value"); ImGui::NextColumn();
            ImGui::Separator();

            for (auto& r : rows_gp0)
            {
                ImGui::Text(r.bits.c_str()); ImGui::NextColumn();
                ImGui::Text(r.description.c_str()); ImGui::NextColumn();
                ImGui::Text(r.value.c_str()); ImGui::NextColumn();

                if (r.bits != "GP0(E5h).11-21") ImGui::Separator();
            }
            ImGui::Columns();
            ImGui::Separator();
            ImGui::TreePop();
        }
    }

    ImGui::End();*/
}
