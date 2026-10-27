#include "cheat.h"

#include <imgui.h>
#include <imgui_impl_metal.h>

static bool g_menu_init = false;
static void* g_metal_device = 0;
static void* g_metal_command_queue = 0;

static void MenuRender() {
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

static void ESPRender() {
    if (!g_esp_boxes && !g_esp_names && !g_esp_distance) return;
    if (!g_camera || !g_player_list) return;

    int count = GetPlayerCount();
    for (int i = 0; i < count; i++) {
        void* ply = GetPlayer(i);
        if (!ply) continue;
        Player* p = (Player*)ply;
        if (p == g_local_player) continue;
        if (!p->isAlive) continue;

        Vec3 pos = GetPlayerPosition(p);
        Vec3 screen = WorldToScreenPoint(pos);
        if (screen.x < 0 || screen.y < 0) continue;

        ImVec2 pos2(screen.x, screen.y);

        if (g_esp_boxes) {
            ImGui::GetBackgroundDrawList()->AddRect(
                ImVec2(pos2.x - 30, pos2.y - 60),
                ImVec2(pos2.x + 30, pos2.y + 30),
                IM_COL32(255, 0, 0, 255), 0, 0, 2.0f);
        }

        if (g_esp_names) {
            char name[64];
            snprintf(name, 64, "Player %lu", (unsigned long)p->actorNumber);
            ImGui::GetBackgroundDrawList()->AddText(
                ImVec2(pos2.x - 20, pos2.y - 70),
                IM_COL32(255, 255, 255, 255), name);
        }

        if (g_esp_distance) {
            Vec3 camPos = GetPlayerPosition(g_local_player);
            F32 dist = Distance3D(camPos, pos);
            char distStr[32];
            snprintf(distStr, 32, "%.0f m", dist);
            ImGui::GetBackgroundDrawList()->AddText(
                ImVec2(pos2.x - 20, pos2.y + 35),
                IM_COL32(0, 255, 0, 255), distStr);
        }
    }
}

static void ChamsApply() {
    if (!g_chams_enabled) return;
    if (!g_player_list) return;

    int count = GetPlayerCount();
    for (int i = 0; i < count; i++) {
        void* ply = GetPlayer(i);
        if (!ply) continue;
        Player* p = (Player*)ply;
        if (p == g_local_player) continue;
        if (!p->isAlive) continue;

        Transform* tr = (Transform*)p->transform;
        GameObject* go = (GameObject*)p->gameObject;

        void* renderer = 0;
        void* comp = ((GetComponent_t)(g_base + 0x2BAA268))(go, 0);
        while (comp) {
            Component* c = (Component*)comp;
            if (c->klass == 0) break;
            renderer = comp;
            break;
        }

        if (!renderer) continue;

        Color chamsColor = {g_chams_r, g_chams_g, g_chams_b, 1.0f};
        ((SetColor_t)(g_base + 0x2B84974))(renderer, "_Color", chamsColor);
        ((SetFloat_t)(g_base + 0x2B88894))(renderer, "_ZTest", 4.0f);
        ((SetFloat_t)(g_base + 0x2BA7DAC))(renderer, "_Emission", 1.0f);
    }
}

static void MenuUpdate(F32 dt) {
    ChatSpamUpdate(dt);
    ESPRender();
    ChamsApply();
}

static bool g_menu_visible = false;

extern "C" void MenuToggle() {
    g_menu_open = !g_menu_open;
}

extern "C" void MenuRenderFrame() {
    MenuRender();
}
