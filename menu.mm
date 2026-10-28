#include "cheat.h"
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>

#include <imgui.h>
#include <imgui_impl_metal.h>

extern "C" void MenuInit();
extern "C" void MenuToggle();
extern "C" bool MenuIsOpen();
extern "C" void MenuSetOpen(bool open);

static bool g_menu_initialized = false;
static id<MTLDevice> g_metal_device = nil;
static id<MTLCommandQueue> g_metal_queue = nil;

static ImVec4 g_menu_color = ImVec4(0.15f, 0.15f, 0.17f, 0.95f);
static ImVec4 g_accent_color = ImVec4(0.0f, 1.0f, 0.5f, 1.0f);

extern "C" void MenuInit() {
    if (g_menu_initialized) return;
    g_metal_device = MTLCreateSystemDefaultDevice();
    if (!g_metal_device) return;
    g_metal_queue = [g_metal_device newCommandQueue];

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = NULL;
    io.LogFilename = NULL;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.MouseDrawCursor = true;

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(10, 10);
    style.WindowRounding = 10.0f;
    style.FramePadding = ImVec2(8, 5);
    style.FrameRounding = 6.0f;
    style.ItemSpacing = ImVec2(8, 6);
    style.ItemInnerSpacing = ImVec2(6, 4);
    style.TouchExtraPadding = ImVec2(0, 0);
    style.IndentSpacing = 12.0f;
    style.ScrollbarSize = 12.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabMinSize = 12.0f;
    style.GrabRounding = 6.0f;
    style.TabRounding = 8.0f;
    style.ColorButtonPosition = ImGuiDir_Left;
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.5f);

    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.11f, 0.97f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.23f, 0.50f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.17f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.20f, 0.23f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.25f, 0.28f, 1.00f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.12f, 0.13f, 0.95f);
    style.Colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.23f, 0.50f);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.17f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.20f, 0.23f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.25f, 0.28f, 1.00f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.12f, 0.13f, 0.95f);
    style.Colors[ImGuiCol_CheckBox] = ImVec4(0.20f, 0.20f, 0.23f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.0f, 1.0f, 0.5f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.0f, 1.0f, 0.6f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.23f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.25f, 0.28f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.0f, 1.0f, 0.5f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.23f, 1.00f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.25f, 0.28f, 1.00f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.0f, 1.0f, 0.5f, 1.0f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.20f, 0.23f, 1.00f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.25f, 0.25f, 0.28f, 1.00f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.0f, 1.0f, 0.5f, 1.0f);
    style.Colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.97f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.65f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.0f, 1.0f, 0.5f, 0.35f);

    ImGui_ImplMetal_Init(g_metal_device);
    g_menu_initialized = true;
}

extern "C" void MenuToggle() {
    if (!g_menu_initialized) MenuInit();
    g_menu_open = !g_menu_open;
}

extern "C" bool MenuIsOpen() {
    return g_menu_open;
}

extern "C" void MenuSetOpen(bool open) {
    g_menu_open = open;
}

static bool g_show_demo = false;

static void RenderMenu() {
    if (!g_menu_open) return;

    ImGui::Begin("CounterFlame Internal v1.3.0.1", &g_menu_open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
    ImGui::SetWindowSize(ImVec2(700, 600), ImGuiCond_FirstUseEver);

    static int tab = 0;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
    if (ImGui::BeginTabBar("##MenuTabs", ImGuiTabBarFlags_FittingPolicyResize)) {
        if (ImGui::BeginTabItem("Aimbot")) {
            tab = 0;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Anti-Aim")) {
            tab = 1;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Visuals")) {
            tab = 2;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Weapon")) {
            tab = 3;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Movement")) {
            tab = 4;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Chat")) {
            tab = 5;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Anti-Ban")) {
            tab = 6;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Skin")) {
            tab = 7;
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::PopStyleVar();

    ImGui::Spacing();

    if (tab == 0) {
        ImGui::BeginGroup();
        ImGui::Checkbox("Silent Aim", &g_silent_aim);
        ImGui::SameLine();
        ImGui::TextDisabled("Кроссхэир на месте, пуля в цель");
        ImGui::SliderFloat("FOV", &g_fov, 1.0f, 90.0f, "%.1f°");
        ImGui::SliderFloat("Max Dist", &g_max_distance, 10.0f, 500.0f, "%.0f м");
        ImGui::Combo("Bone", &g_target_bone, "Head\0Body\0Neck\0");
        ImGui::Checkbox("Predictive Aim", &g_predictive_aim);
        ImGui::SliderInt("Pred (ms)", &g_prediction_ms, 0, 500, "%d мс");
        ImGui::EndGroup();
    }

    if (tab == 1) {
        ImGui::BeginGroup();
        ImGui::Checkbox("Spinbot", &g_spin_enabled);
        ImGui::SameLine();
        ImGui::TextDisabled("Крутилка вокруг оси Y");
        ImGui::Combo("Mode", &g_spin_mode, "Off\0Spin\0Jitter\0Static\0");
        ImGui::SliderFloat("Speed", &g_spin_speed, 10.0f, 720.0f, "%.0f deg/s");
        ImGui::Checkbox("Invert X", &g_spin_invert_x);
        ImGui::EndGroup();
    }

    if (tab == 2) {
        ImGui::BeginGroup();
        ImGui::Checkbox("ESP Boxes", &g_esp_boxes);
        ImGui::SameLine();
        ImGui::TextDisabled("Боксы вокруг врагов");
        ImGui::Checkbox("ESP Names", &g_esp_names);
        ImGui::Checkbox("ESP Distance", &g_esp_distance);
        ImGui::Checkbox("Chams", &g_chams_enabled);
        ImGui::SameLine();
        ImGui::ColorEdit3("Chams Color", &g_chams_r);
        ImGui::EndGroup();
    }

    if (tab == 3) {
        ImGui::BeginGroup();
        ImGui::Checkbox("No Recoil", &g_no_recoil);
        ImGui::SameLine();
        ImGui::TextDisabled("Убирает отдачу оружия");
        ImGui::Checkbox("No Spread", &g_no_spread);
        ImGui::SameLine();
        ImGui::TextDisabled("Убирает разброс пуль");
        ImGui::EndGroup();
    }

    if (tab == 4) {
        ImGui::BeginGroup();
        ImGui::Checkbox("Bunnyhop", &g_bunnyhop);
        ImGui::SameLine();
        ImGui::TextDisabled("Автопрыжок при приземлении");
        ImGui::EndGroup();
    }

    if (tab == 5) {
        ImGui::BeginGroup();
        ImGui::Checkbox("Chat Spam", &g_chat_spam);
        ImGui::SameLine();
        ImGui::TextDisabled("Автосообщения в чат");
        ImGui::SliderFloat("Interval", &g_spam_interval, 1.0f, 10.0f, "%.1f с");
        ImGui::EndGroup();
    }

    if (tab == 6) {
        ImGui::BeginGroup();
        ImGui::Checkbox("HWID Spoof", &g_hwid_spoof);
        ImGui::SameLine();
        ImGui::TextDisabled("Подмена ID устройства");
        ImGui::Checkbox("Bypass ACTk", &g_bypass_actk);
        ImGui::SameLine();
        ImGui::TextDisabled("Обход Anti-Cheat Toolkit");
        ImGui::Checkbox("Bypass Version", &g_bypass_version);
        ImGui::SameLine();
        ImGui::TextDisabled("Обход проверки версии");
        ImGui::Checkbox("Bypass RootDetect", &g_bypass_rootdetect);
        ImGui::SameLine();
        ImGui::TextDisabled("Обход детекта джейлбрейка");
        ImGui::EndGroup();
    }

    if (tab == 7) {
        ImGui::BeginGroup();
        ImGui::Checkbox("Skin Changer", &g_skin_changer_enabled);
        ImGui::SameLine();
        ImGui::TextDisabled("Скин виден ВСЕМ игрокам");
        ImGui::SliderInt("Skin ID", &g_skin_id, 1, 1000, "%d");
        ImGui::SliderInt("Seed", &g_skin_seed, 1, 9999, "%d");
        ImGui::Checkbox("StatTrak", &g_skin_stattrak);
        ImGui::SameLine();
        ImGui::SliderInt("StatTrak Value", &g_stattrak_value, 0, 9999, "%d");
        ImGui::Checkbox("Update Players", &g_skin_update_players);
        ImGui::SameLine();
        ImGui::TextDisabled("Обновлять скины всех игроков");
        ImGui::EndGroup();
    }

    ImGui::End();
}

extern "C" void DoRender(void* descriptor) {
    if (!g_menu_initialized || !descriptor) return;

    id<MTLCommandBuffer> commandBuffer = [g_metal_queue commandBuffer];
    if (!commandBuffer) return;

    MTLRenderPassDescriptor* rp = (__bridge MTLRenderPassDescriptor*)descriptor;
    id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:rp];
    [encoder pushDebugGroup:@"CounterFlame ImGui"];

    ImGui_ImplMetal_NewFrame(rp);
    ImGui::NewFrame();

    RenderMenu();

    ImGui::Render();
    ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), commandBuffer, encoder);

    [encoder popDebugGroup];
    [encoder endEncoding];
    [commandBuffer commit];
}
