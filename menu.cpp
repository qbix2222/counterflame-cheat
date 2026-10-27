#include "cheat.h"
#include <imgui.h>
#include <imgui_impl_metal.h>
#include <Metal/Metal.h>
#include <QuartzCore/QuartzCore.h>

static bool g_menu_initialized = false;
static id<MTLDevice> g_device = nil;
static id<MTLCommandQueue> g_command_queue = nil;

extern "C" void MenuInit() {
    if (g_menu_initialized) return;
    g_device = MTLCreateSystemDefaultDevice();
    if (!g_device) return;
    g_command_queue = [g_device newCommandQueue];

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = NULL;
    io.LogFilename = NULL;
    ImGui::StyleColorsDark();

    ImGui_ImplMetal_Init(g_device);
    g_menu_initialized = true;
}

void MenuRender() {
    if (!g_menu_open) return;
    ImGui::Begin("CounterFlame Internal", &g_menu_open, ImGuiWindowFlags_AlwaysAutoResize);

    if (ImGui::CollapsingHeader("Aimbot")) {
        ImGui::Checkbox("Silent Aim", &g_silent_aim);
        ImGui::SliderFloat("FOV", &g_fov, 1.0f, 90.0f, "%.1f");
        ImGui::Combo("Bone", &g_target_bone, "Head\0Body\0Neck\0");
        ImGui::SliderFloat("Max Distance", &g_max_distance, 10.0f, 500.0f, "%.0f m");
        ImGui::Checkbox("Predictive Aim", &g_predictive_aim);
        ImGui::SliderInt("Prediction (ms)", &g_prediction_ms, 0, 500, "%d ms");
    }
    if (ImGui::CollapsingHeader("Anti-Aim")) {
        ImGui::Checkbox("Spinbot", &g_spin_enabled);
        ImGui::Combo("Mode", &g_spin_mode, "Off\0Spin\0Jitter\0Static\0");
        ImGui::SliderFloat("Speed", &g_spin_speed, 10.0f, 720.0f, "%.0f deg/s");
        ImGui::Checkbox("Invert X", &g_spin_invert_x);
    }
    if (ImGui::CollapsingHeader("Visuals")) {
        ImGui::Checkbox("ESP Boxes", &g_esp_boxes);
        ImGui::Checkbox("ESP Names", &g_esp_names);
        ImGui::Checkbox("ESP Distance", &g_esp_distance);
        ImGui::Checkbox("Chams", &g_chams_enabled);
        ImGui::ColorEdit3("Chams Color", &g_chams_r);
    }
    if (ImGui::CollapsingHeader("Weapon")) {
        ImGui::Checkbox("No Recoil", &g_no_recoil);
        ImGui::Checkbox("No Spread", &g_no_spread);
    }
    if (ImGui::CollapsingHeader("Movement")) {
        ImGui::Checkbox("Bunnyhop", &g_bunnyhop);
    }
    if (ImGui::CollapsingHeader("Chat")) {
        ImGui::Checkbox("Chat Spam", &g_chat_spam);
        ImGui::SliderFloat("Interval", &g_spam_interval, 1.0f, 10.0f, "%.1f s");
    }
    if (ImGui::CollapsingHeader("Anti-Ban")) {
        ImGui::Checkbox("HWID Spoof", &g_hwid_spoof);
        ImGui::Checkbox("Bypass ACTk", &g_bypass_actk);
    }
    if (ImGui::CollapsingHeader("Config")) {
        if (ImGui::Button("Save")) Config_Save();
        ImGui::SameLine();
        if (ImGui::Button("Load")) Config_Load();
    }
    ImGui::End();
}

extern "C" void DoRender(void* descriptor_ptr) {
    if (!g_menu_initialized || !descriptor_ptr) return;
    MTLRenderPassDescriptor* descriptor = (__bridge MTLRenderPassDescriptor*)descriptor_ptr;
    
    id<MTLCommandBuffer> commandBuffer = [g_command_queue commandBuffer];
    if (!commandBuffer) return;
    [commandBuffer enqueue];

    descriptor.colorAttachments[0].loadAction = MTLLoadActionLoad;
    descriptor.colorAttachments[0].storeAction = MTLStoreActionStore;

    id<MTLRenderCommandEncoder> commandEncoder = [commandBuffer renderCommandEncoderWithDescriptor:descriptor];
    [commandEncoder pushDebugGroup:@"ImGui"];

    ImGui_ImplMetal_NewFrame(descriptor);
    ImGui::NewFrame();
    MenuRender();
    ImGui::Render();
    ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), commandBuffer, commandEncoder);

    [commandEncoder popDebugGroup];
    [commandEncoder endEncoding];
    [commandBuffer commit];
}

extern "C" void MenuToggle() {
    g_menu_open = !g_menu_open;
}
