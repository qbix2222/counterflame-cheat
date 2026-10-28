#include "cheat.h"
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>

#include <imgui.h>
#include <imgui_impl_metal.h>

extern "C" void MenuInit();
extern "C" void MenuToggle();
extern "C" bool MenuIsOpen();
extern "C" void MenuSetOpen(bool open);
extern "C" void MenuRender(float dt, id<MTLRenderPassDescriptor> renderPassDescriptor);

/* ---------- Menu state ---------- */
static bool g_menu_initialized = false;
static id<MTLDevice> g_metal_device = nil;
static id<MTLCommandQueue> g_metal_queue = nil;

extern bool g_menu_visible;
static float g_menu_slide   = 0.0f;
static float g_menu_target  = 0.0f;

/* ---------- Menu theme (tweakable) ---------- */
static ImVec4 g_accent_color   = ImVec4(0.0f, 1.0f, 0.5f, 1.0f);
static ImVec4 g_accent_color_alt = ImVec4(0.0f, 0.7f, 1.0f, 1.0f);
static ImVec4 g_bg_color       = ImVec4(0.08f, 0.08f, 0.10f, 0.97f);
static ImVec4 g_panel_color    = ImVec4(0.10f, 0.10f, 0.12f, 0.98f);
static ImVec4 g_text_color     = ImVec4(0.95f, 0.95f, 0.97f, 1.00f);
static ImVec4 g_text_dim_color = ImVec4(0.60f, 0.60f, 0.65f, 1.00f);

extern int   g_menu_font_size;
extern float g_menu_opacity;
extern float g_menu_scale;
extern int   g_menu_position;
extern float g_menu_anim_speed;

/* ---------- Forward declarations ---------- */
static void DrawStyledCheckbox(const char* label, bool* value);

/* ---------- Menu init ---------- */
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
    io.MouseDrawCursor = true;

    ImGui::StyleColorsDark();
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowPadding      = ImVec2(12, 12);
    s.WindowRounding     = 12.0f;
    s.FramePadding       = ImVec2(10, 7);
    s.FrameRounding      = 8.0f;
    s.ItemSpacing        = ImVec2(10, 7);
    s.ItemInnerSpacing   = ImVec2(8, 6);
    s.GrabMinSize      = 14.0f;
    s.GrabRounding     = 8.0f;
    s.TabRounding      = 10.0f;
    s.ButtonTextAlign    = ImVec2(0.5f, 0.5f);

    s.Colors[ImGuiCol_WindowBg]          = g_bg_color;
    s.Colors[ImGuiCol_PopupBg]             = ImVec4(0.10f, 0.10f, 0.12f, 0.96f);
    s.Colors[ImGuiCol_Border]              = ImVec4(0.25f, 0.25f, 0.30f, 0.40f);
    s.Colors[ImGuiCol_FrameBg]             = ImVec4(0.15f, 0.15f, 0.17f, 1.00f);
    s.Colors[ImGuiCol_FrameBgHovered]      = ImVec4(0.20f, 0.20f, 0.23f, 1.00f);
    s.Colors[ImGuiCol_FrameBgActive]       = g_accent_color;
    s.Colors[ImGuiCol_TitleBg]             = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    s.Colors[ImGuiCol_TitleBgActive]       = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    s.Colors[ImGuiCol_TitleBgCollapsed]    = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    s.Colors[ImGuiCol_BorderShadow]        = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    s.Colors[ImGuiCol_Button]              = ImVec4(0.20f, 0.20f, 0.23f, 1.00f);
    s.Colors[ImGuiCol_ButtonHovered]       = g_accent_color;
    s.Colors[ImGuiCol_ButtonActive]        = g_accent_color;
    s.Colors[ImGuiCol_Header]              = ImVec4(0.20f, 0.20f, 0.23f, 1.00f);
    s.Colors[ImGuiCol_HeaderHovered]       = ImVec4(0.25f, 0.25f, 0.28f, 1.00f);
    s.Colors[ImGuiCol_HeaderActive]        = g_accent_color;
    s.Colors[ImGuiCol_Separator]           = ImVec4(0.20f, 0.20f, 0.23f, 1.00f);
    s.Colors[ImGuiCol_SeparatorActive]     = g_accent_color;
    s.Colors[ImGuiCol_Text]                = g_text_color;
    s.Colors[ImGuiCol_TextDisabled]        = g_text_dim_color;
    s.Colors[ImGuiCol_TextSelectedBg]      = ImVec4(0.0f, 1.0f, 0.5f, 0.35f);

    ImGui_ImplMetal_Init(g_metal_device);
    g_menu_initialized = true;
}

extern "C" void MenuToggle() {
    if (!g_menu_initialized) MenuInit();
    g_menu_visible = !g_menu_visible;
}

extern "C" bool MenuIsOpen() {
    return g_menu_visible;
}

extern "C" void MenuSetOpen(bool open) {
    g_menu_visible = open;
}

static void DrawStyledCheckbox(const char* label, bool* value) {
    ImGui::Checkbox(label, value);
    if (*value) {
        ImGui::SameLine();
        ImGui::TextColored(g_accent_color, "ON");
    } else {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.65f, 1.0f), "OFF");
    }
}

extern "C" void MenuRender(float dt, id<MTLRenderPassDescriptor> renderPassDescriptor) {
    if (!g_menu_initialized) return;

    // Update animation
    float target = g_menu_visible ? 1.0f : 0.0f;
    g_menu_slide += (target - g_menu_slide) * g_menu_anim_speed * dt;
    if (fabsf(g_menu_slide - target) < 0.005f) g_menu_slide = target;

    if (!g_menu_visible && g_menu_slide < 0.001f) return;

    if (!renderPassDescriptor) return;

    id<MTLCommandBuffer> commandBuffer = [g_metal_queue commandBuffer];
    if (!commandBuffer) return;

    ImGui_ImplMetal_NewFrame(renderPassDescriptor);
    ImGui::NewFrame();

    // Build menu
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 ws = io.DisplaySize;

    float panelW = ws.x * 0.92f;
    float panelH = ws.y * 0.45f;
    float panelX = (ws.x - panelW) * 0.5f;
    float panelY = ws.y - panelH + panelH * (1.0f - g_menu_slide);

    ImGui::SetNextWindowPos(ImVec2(panelX, panelY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(panelW, panelH), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, g_menu_opacity * g_menu_slide);

    ImGui::Begin("CounterFlame Internal", &g_menu_visible,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                 ImGuiWindowFlags_NoBringToFrontOnFocus |
                 ImGuiWindowFlags_NoSavedSettings);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 winSize = ImGui::GetWindowSize();

    dl->AddRectFilled(winPos, ImVec2(winPos.x + winSize.x, winPos.y + winSize.y),
                      ImGui::ColorConvertFloat4ToU32(g_panel_color), 12.0f);
    dl->AddRectFilled(ImVec2(winPos.x, winPos.y),
                      ImVec2(winPos.x + winSize.x, winPos.y + 3.0f),
                      ImGui::ColorConvertFloat4ToU32(g_accent_color), 2.0f);

    // Tabs
    static int tab = 0;
    const char* tabs[] = { "Aimbot", "Anti-Aim", "Visuals", "Weapon", "Movement", "Chat", "Anti-Ban", "Skin" };
    int tabCount = 8;
    float tabW = (winSize.x - 32.0f) / tabCount;
    float tabH = 40.0f;

    for (int i = 0; i < tabCount; i++) {
        ImVec2 tp(winPos.x + 16.0f + i * tabW, winPos.y + 30.0f);
        bool active = (tab == i);
        ImVec4 col = active ? g_accent_color : ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
        dl->AddRectFilled(ImVec2(tp.x, tp.y),
                          ImVec2(tp.x + tabW - 16.0f, tp.y + tabH),
                          ImGui::ColorConvertFloat4ToU32(col), 6.0f);
        ImGui::SetCursorPos(ImVec2(tp.x + 8.0f, tp.y + 8.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, active ? ImVec4(0.05f, 0.05f, 0.07f, 1.0f) : g_text_dim_color);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0,0,0,0));
        if (ImGui::Button(tabs[i], ImVec2(tabW - 16.0f, tabH - 16.0f))) {
            tab = i;
        }
        ImGui::PopStyleColor(3);
    }

    ImGui::SetCursorPos(ImVec2(20.0f, 46.0f));
    ImGui::BeginChild("##MenuContent", ImVec2(winSize.x - 40.0f, winSize.y - 56.0f),
                      false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);

    if (tab == 0) {
        ImGui::TextColored(g_accent_color, "AIMBOT");
        ImGui::Separator();
        DrawStyledCheckbox("Silent Aim", &g_silent_aim);
        ImGui::SameLine();
        ImGui::TextColored(g_text_dim_color, "Кроссхэир на месте, пуля в цель");
        ImGui::SliderFloat("FOV", &g_fov, 1.0f, 90.0f, "%.1f°");
        ImGui::SliderFloat("Max Dist", &g_max_distance, 10.0f, 500.0f, "%.0f м");
        ImGui::Combo("Bone", &g_target_bone, "Head\0Body\0Neck\0");
        DrawStyledCheckbox("Predictive Aim", &g_predictive_aim);
        ImGui::SliderInt("Pred (ms)", &g_prediction_ms, 0, 500, "%d мс");
    } else if (tab == 1) {
        ImGui::TextColored(g_accent_color, "ANTI-AIM");
        ImGui::Separator();
        DrawStyledCheckbox("Spinbot (крутилка)", &g_spin_enabled);
        ImGui::Combo("Mode", &g_spin_mode, "Off\0Spin\0Jitter\0Static\0");
        ImGui::SliderFloat("Speed", &g_spin_speed, 10.0f, 720.0f, "%.0f deg/s");
        DrawStyledCheckbox("Invert X", &g_spin_invert_x);
    } else if (tab == 2) {
        ImGui::TextColored(g_accent_color, "VISUALS / ESP");
        ImGui::Separator();
        DrawStyledCheckbox("ESP Boxes", &g_esp_boxes);
        ImGui::SameLine();
        ImGui::TextColored(g_text_dim_color, "Боксы вокруг врагов");
        DrawStyledCheckbox("ESP Names", &g_esp_names);
        DrawStyledCheckbox("ESP Distance", &g_esp_distance);
        DrawStyledCheckbox("Chams (Wallhack)", &g_chams_enabled);
        ImGui::SameLine();
        ImGui::ColorEdit3("Chams Color", &g_chams_r);
    } else if (tab == 3) {
        ImGui::TextColored(g_accent_color, "WEAPON");
        ImGui::Separator();
        DrawStyledCheckbox("No Recoil", &g_no_recoil);
        ImGui::SameLine();
        ImGui::TextColored(g_text_dim_color, "Убирает отдачу оружия");
        DrawStyledCheckbox("No Spread", &g_no_spread);
        ImGui::SameLine();
        ImGui::TextColored(g_text_dim_color, "Убирает разброс пуль");
    } else if (tab == 4) {
        ImGui::TextColored(g_accent_color, "MOVEMENT");
        ImGui::Separator();
        DrawStyledCheckbox("Bunnyhop", &g_bunnyhop);
        ImGui::SameLine();
        ImGui::TextColored(g_text_dim_color, "Автопрыжок при приземлении");
    } else if (tab == 5) {
        ImGui::TextColored(g_accent_color, "CHAT");
        ImGui::Separator();
        DrawStyledCheckbox("Chat Spam", &g_chat_spam);
        ImGui::SameLine();
        ImGui::TextColored(g_text_dim_color, "Автосообщения в чат");
        ImGui::SliderFloat("Interval", &g_spam_interval, 1.0f, 10.0f, "%.1f с");
    } else if (tab == 6) {
        ImGui::TextColored(g_accent_color, "ANTI-BAN");
        ImGui::Separator();
        DrawStyledCheckbox("HWID Spoof", &g_hwid_spoof);
        ImGui::SameLine();
        ImGui::TextColored(g_text_dim_color, "Подмена ID устройства");
        DrawStyledCheckbox("Bypass ACTk", &g_bypass_actk);
        ImGui::SameLine();
        ImGui::TextColored(g_text_dim_color, "Обход Anti-Cheat Toolkit");
        DrawStyledCheckbox("Bypass Version", &g_bypass_version);
        ImGui::SameLine();
        ImGui::TextColored(g_text_dim_color, "Обход проверки версии");
        DrawStyledCheckbox("Bypass RootDetect", &g_bypass_rootdetect);
        ImGui::SameLine();
        ImGui::TextColored(g_text_dim_color, "Обход детекта джейлбрейка");
    } else if (tab == 7) {
        ImGui::TextColored(g_accent_color, "SKIN CHANGER (VISIBLE TO ALL)");
        ImGui::Separator();
        DrawStyledCheckbox("Skin Changer", &g_skin_changer_enabled);
        ImGui::SameLine();
        ImGui::TextColored(g_text_dim_color, "Меняет скин оружия для всех");
        ImGui::SliderInt("Skin ID", &g_skin_id, 1, 1000, "%d");
        ImGui::SliderInt("Seed", &g_skin_seed, 1, 9999, "%d");
        DrawStyledCheckbox("StatTrak", &g_skin_stattrak);
        ImGui::SameLine();
        ImGui::SliderInt("StatTrak Value", &g_stattrak_value, 0, 9999, "%d");
        DrawStyledCheckbox("Update Players", &g_skin_update_players);
    }

    ImGui::EndChild();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextColored(g_text_dim_color, "Chronicle Engine v3.0 | CounterFlame 1.3.0.1");
    ImGui::SameLine();
    ImGui::TextColored(g_accent_color, "STATUS: ACTIVE");
    ImGui::End();

    // Render ImGui draw data
    id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
    if (encoder) {
        ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), commandBuffer, encoder);
        [encoder endEncoding];
    }
    [commandBuffer commit];
}
