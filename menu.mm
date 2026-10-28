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

// Modern cheat menu color palette (inspired by Phantom-Cheat-Menu-Design)
static ImVec4 g_accent_color     = ImVec4(0.0f, 1.0f, 0.5f, 1.0f);   // Neon green
static ImVec4 g_accent_color_alt = ImVec4(0.0f, 0.8f, 1.0f, 1.0f);   // Cyan
static ImVec4 g_bg_color         = ImVec4(0.08f, 0.08f, 0.10f, 0.97f);
static ImVec4 g_panel_color      = ImVec4(0.10f, 0.10f, 0.12f, 0.98f);
static ImVec4 g_text_color       = ImVec4(0.95f, 0.95f, 0.97f, 1.00f);
static ImVec4 g_text_dim_color   = ImVec4(0.60f, 0.60f, 0.65f, 1.00f);

static int g_menu_tab = 0;

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

    // Dark theme with neon accents (modern cheat menu style)
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding      = ImVec2(12, 12);
    style.WindowRounding     = 12.0f;
    style.FramePadding       = ImVec2(10, 8);
    style.FrameRounding      = 8.0f;
    style.ItemSpacing        = ImVec2(10, 8);
    style.ItemInnerSpacing   = ImVec2(8, 6);
    style.TouchExtraPadding  = ImVec2(0, 0);
    style.IndentSpacing      = 14.0f;
    style.ScrollbarSize      = 14.0f;
    style.ScrollbarRounding  = 8.0f;
    style.GrabMinSize        = 14.0f;
    style.GrabRounding       = 8.0f;
    style.TabRounding        = 10.0f;
    style.ColorButtonPosition = ImGuiDir_Left;
    style.ButtonTextAlign    = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.5f);

    // Dark theme with neon accents
    style.Colors[ImGuiCol_WindowBg]          = ImVec4(0.08f, 0.08f, 0.10f, 0.98f);
    style.Colors[ImGuiCol_MenuBarBg]         = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_Border]            = ImVec4(0.25f, 0.25f, 0.30f, 0.40f);
    style.Colors[ImGuiCol_FrameBg]           = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered]    = ImVec4(0.20f, 0.20f, 0.24f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive]     = ImVec4(0.28f, 0.25f, 0.32f, 1.00f);
    style.Colors[ImGuiCol_TitleBg]           = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive]     = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed]  = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_PopupBg]           = ImVec4(0.10f, 0.10f, 0.12f, 0.96f);
    style.Colors[ImGuiCol_BorderShadow]      = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_CheckBox]          = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab]        = g_accent_color;
    style.Colors[ImGuiCol_SliderGrabActive]  = ImVec4(0.0f, 0.8f, 0.6f, 1.0f);
    style.Colors[ImGuiCol_Button]            = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered]     = ImVec4(0.25f, 0.25f, 0.28f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive]      = g_accent_color;
    style.Colors[ImGuiCol_Header]            = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_HeaderHovered]     = ImVec4(0.25f, 0.25f, 0.28f, 1.00f);
    style.Colors[ImGuiCol_HeaderActive]      = g_accent_color;
    style.Colors[ImGuiCol_Separator]         = ImVec4(0.20f, 0.20f, 0.23f, 1.00f);
    style.Colors[ImGuiCol_SeparatorHovered]  = ImVec4(0.25f, 0.25f, 0.28f, 1.00f);
    style.Colors[ImGuiCol_SeparatorActive]   = g_accent_color;
    style.Colors[ImGuiCol_Text]              = g_text_color;
    style.Colors[ImGuiCol_TextDisabled]      = g_text_dim_color;
    style.Colors[ImGuiCol_TextSelectedBg]    = ImVec4(0.0f, 1.0f, 0.5f, 0.35f);

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

static void DrawStyledCheckbox(const char* label, bool* value) {
    ImGui::PushStyleColor(ImGuiCol_CheckBox, ImVec4(0.20f, 0.20f, 0.23f, 1.00f));
    if (ImGui::Checkbox(label, value)) {
        // Toggle handled by caller
    }
    if (*value) {
        ImGui::SameLine();
        ImGui::TextColored(g_accent_color, "ON");
    } else {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.65f, 1.0f), "OFF");
    }
    ImGui::PopStyleColor();
}

static void RenderMenu() {
    if (!g_menu_open) return;

    ImGui::Begin("CounterFlame Internal v1.3.0", &g_menu_open,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
    ImGui::SetWindowSize(ImVec2(820, 680), ImGuiCond_FirstUseEver);

    // Header
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
    ImGui::TextColored(g_accent_color, "COUNTERFLAME");
    ImGui::SameLine();
    ImGui::TextColored(g_text_color, "INTERNAL CHEAT v1.3.0.1");
    ImGui::PopFont();
    ImGui::Separator();
    ImGui::Spacing();

    // Tab bar
    static int tab = 0;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));
    if (ImGui::BeginTabBar("##MenuTabs", ImGuiTabBarFlags_FittingPolicyResizeDown)) {
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

    // ================= AIMBOT TAB =================
    if (tab == 0) {
        ImGui::BeginGroup();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.5f, 1.0f));
        ImGui::Text("AIMBOT SETTINGS");
        ImGui::PopStyleColor();
        ImGui::Separator();

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.14f, 0.14f, 0.17f, 1.00f));
        ImGui::Checkbox("Silent Aim", &g_silent_aim);
        ImGui::SameLine();
        ImGui::TextColored(g_text_dim_color, "Кроссхэир на месте, пуля в цель");
        ImGui::SliderFloat("FOV", &g_fov, 1.0f, 90.0f, "%.1f°");
        ImGui::SliderFloat("Max Dist", &g_max_distance, 10.0f, 500.0f, "%.0f м");
        ImGui::Combo("Bone", &g_target_bone, "Head\0Body\0Neck\0");
        ImGui::Checkbox("Predictive Aim", &g_predictive_aim);
        ImGui::SliderInt("Pred (ms)", &g_prediction_ms, 0, 500, "%d мс");
        ImGui::PopStyleColor();
        ImGui::EndGroup();
    }

    // ================= ANTI-AIM TAB =================
    if (tab == 1) {
        ImGui::BeginGroup();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.5f, 1.0f));
        ImGui::Text("ANTI-AIM / SPINBOT");
        ImGui::PopStyleColor();
        ImGui::Separator();

        ImGui::Checkbox("Spinbot (крутилка)", &g_spin_enabled);
        ImGui::Combo("Mode", &g_spin_mode, "Off\0Spin\0Jitter\0Static\0");
        ImGui::SliderFloat("Speed", &g_spin_speed, 10.0f, 720.0f, "%.0f deg/s");
        ImGui::Checkbox("Invert X", &g_spin_invert_x);
        ImGui::EndGroup();
    }

    // ================= VISUALS TAB =================
    if (tab == 2) {
        ImGui::BeginGroup();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.5f, 1.0f));
        ImGui::Text("VISUALS / ESP");
        ImGui::PopStyleColor();
        ImGui::Separator();

        ImGui::Checkbox("ESP Boxes", &g_esp_boxes);
        ImGui::SameLine();
        ImGui::TextColored(g_text_dim_color, "Боксы вокруг врагов");
        ImGui::Checkbox("ESP Names", &g_esp_names);
        ImGui::Checkbox("ESP Distance", &g_esp_distance);
        ImGui::Checkbox("Chams (Wallhack)", &g_chams_enabled);
        ImGui::SameLine();
        ImGui::ColorEdit3("Chams Color", &g_chams_r);
        ImGui::EndGroup();
    }

    // ================= WEAPON TAB =================
    if (tab == 3) {
        ImGui::BeginGroup();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.5f, 1.0f));
        ImGui::Text("WEAPON SETTINGS");
        ImGui::PopStyleColor();
        ImGui::Separator();

        ImGui::Checkbox("No Recoil", &g_no_recoil);
        ImGui::SameLine();
        ImGui::TextColored(g_text_dim_color, "Убирает отдачу оружия");
        ImGui::Checkbox("No Spread", &g_no_spread);
        ImGui::SameLine();
        ImGui::TextColored(g_text_dim_color, "Убирает разброс пуль");
        ImGui::EndGroup();
    }

    // ================= MOVEMENT TAB =================
    if (tab == 4) {
        ImGui::BeginGroup();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.5f, 1.0f));
        ImGui::Text("MOVEMENT");
        ImGui::PopStyleColor();
        ImGui::Separator();

        ImGui::Checkbox("Bunnyhop", &g_bunnyhop);
        ImGui::SameLine();
        ImGui::TextColored(g_text_dim_color, "Автопрыжок при приземлении");
        ImGui::EndGroup();
    }

    // ================= CHAT TAB =================
    if (tab == 5) {
        ImGui::BeginGroup();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.5f, 1.0f));
        ImGui::Text("CHAT SETTINGS");
        ImGui::PopStyleColor();
        ImGui::Separator();

        ImGui::Checkbox("Chat Spam", &g_chat_spam);
        ImGui::SameLine();
        ImGui::TextColored(g_text_dim_color, "Автосообщения в чат");
        ImGui::SliderFloat("Interval", &g_spam_interval, 1.0f, 10.0f, "%.1f с");
        ImGui::EndGroup();
    }

    // ================= ANTI-BAN TAB =================
    if (tab == 6) {
        ImGui::BeginGroup();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.5f, 1.0f));
        ImGui::Text("ANTI-BAN PROTECTION");
        ImGui::PopStyleColor();
        ImGui::Separator();

        ImGui::Checkbox("HWID Spoof", &g_hwid_spoof);
        ImGui::SameLine();
        ImGui::TextColored(g_text_dim_color, "Подмена ID устройства");
        ImGui::Checkbox("Bypass ACTk", &g_bypass_actk);
        ImGui::SameLine();
        ImGui::TextColored(g_text_dim_color, "Обход Anti-Cheat Toolkit");
        ImGui::Checkbox("Bypass Version", &g_bypass_version);
        ImGui::SameLine();
        ImGui::TextColored(g_text_dim_color, "Обход проверки версии");
        ImGui::Checkbox("Bypass RootDetect", &g_bypass_rootdetect);
        ImGui::SameLine();
        ImGui::TextColored(g_text_dim_color, "Обход детекта джейлбрейка");
        ImGui::EndGroup();
    }

    // ================= SKIN CHANGER TAB =================
    if (tab == 7) {
        ImGui::BeginGroup();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.5f, 1.0f));
        ImGui::Text("SKIN CHANGER (VISIBLE TO ALL)");
        ImGui::PopStyleColor();
        ImGui::Separator();

        ImGui::Checkbox("Enable Skin Changer", &g_skin_changer_enabled);
        ImGui::SameLine();
        ImGui::TextColored(g_text_dim_color, "Меняет скин оружия для всех");
        ImGui::SliderInt("Skin ID", &g_skin_id, 1, 1000, "%d");
        ImGui::SliderInt("Seed", &g_skin_seed, 1, 9999, "%d");
        ImGui::Checkbox("StatTrak", &g_skin_stattrak);
        ImGui::SameLine();
        ImGui::SliderInt("StatTrak Value", &g_stattrak_value, 0, 9999, "%d");
        ImGui::Checkbox("Update Players", &g_skin_update_players);
        ImGui::SameLine();
        ImGui::TextColored(g_text_dim_color, "Обновляет скины всех игроков");
        ImGui::EndGroup();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Footer
    ImGui::TextColored(g_text_dim_color, "Chronicle Engine v3.0 | CounterFlame 1.3.0.1");
    ImGui::SameLine();
    ImGui::TextColored(g_accent_color, "STATUS: ACTIVE");
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
