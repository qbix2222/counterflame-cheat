#pragma once

#include <cstdint>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>
#include <algorithm>
#include <random>

#include "il2cpp_resolver.h"

typedef uintptr_t UPtr;
typedef int32_t   S32;
typedef uint32_t  U32;
typedef uint8_t   U8;
typedef float     F32;
typedef int64_t   S64;
typedef uint64_t  U64;

struct Vec2  { F32 x, y; };
struct Vec3  { F32 x, y, z; };
struct Quat  { F32 x, y, z, w; };
struct Color { F32 r, g, b, a; };

static inline Vec3 V3(F32 x, F32 y, F32 z) { return Vec3{ x, y, z }; }

// === RVAs функций UnityFramework (из dump.cs) ===
struct Offsets {
    static constexpr UPtr GameObject_Find           = 0x2BAB388;
    static constexpr UPtr Component_GetComponent    = 0x2BAA268;
    static constexpr UPtr Component_get_transform   = 0x2BAA1E8;
    static constexpr UPtr Transform_get_position    = 0x2BB4894;
    static constexpr UPtr Transform_set_rotation    = 0x2BB4B58;
    static constexpr UPtr Transform_get_forward     = 0x2BB4DE4;
    static constexpr UPtr Camera_WorldToScreenPoint = 0x2B73E5C;
    static constexpr UPtr Camera_get_main           = 0x2B741F0;
    static constexpr UPtr Input_GetButton           = 0x24DCF0C;
    static constexpr UPtr SendChatMessage           = 0x158D6B0;
    static constexpr UPtr RootDetect_Checkroot      = 0x140EDF0;
    static constexpr UPtr ApkHashChecker_ctor       = 0x126F358;
    static constexpr UPtr SessionGuard_Revoke       = 0x13F0128;
    static constexpr UPtr SessionGuard_Reset        = 0x13F03D4;
    static constexpr UPtr ScarletVersion_cctor      = 0x13F044C;
    static constexpr UPtr ScarletVersion_GetVersion = 0x13F0658;
    static constexpr UPtr PublishLook               = 0x1275CBC;
    static constexpr UPtr WallHackDetector_Start    = 0x12388F0;
    static constexpr UPtr ShootRaycastModule        = 0x1239F20;
    static constexpr UPtr CharacterController_isGrounded = 0x2BFCC04;
    static constexpr UPtr SystemInfo_deviceUniqueIdentifier = 0x2BB19D8;
};

// === Cheat Config ===
struct CheatConfig {
    bool  silent_aim = false;
    float fov = 25.0f;
    int   target_bone = 0;
    float max_distance = 150.0f;
    bool  predictive_aim = false;
    int   prediction_ms = 100;

    bool  spin_enabled = false;
    float spin_speed = 180.0f;
    int   spin_mode = 1;
    bool  spin_invert_x = false;

    bool  esp_boxes = true;
    bool  esp_names = true;
    bool  esp_distance = true;
    bool  chams_enabled = false;
    float chams_r = 1.0f;
    float chams_g = 0.0f;
    float chams_b = 0.0f;

    bool  no_recoil = true;
    bool  no_spread = true;
    bool  bunnyhop = false;

    bool  chat_spam = false;
    float spam_interval = 5.0f;

    bool  hwid_spoof = false;
    bool  bypass_actk = true;
    bool  bypass_version = true;
    bool  bypass_rootdetect = true;

    bool  skin_changer_enabled = false;
    int   skin_id = 1;
    int   skin_seed = 1337;
    bool  skin_stattrak = true;
    int   stattrak_value = 1337;
    bool  skin_update_players = true;
};

extern UPtr g_unity_base;
extern CheatConfig g_config;

// === Глобальное состояние ===
extern bool  g_silent_aim;
extern float g_fov;
extern int   g_target_bone;
extern float g_max_distance;
extern bool  g_predictive_aim;
extern int   g_prediction_ms;

extern bool  g_spin_enabled;
extern float g_spin_speed;
extern int   g_spin_mode;
extern bool  g_spin_invert_x;

extern bool  g_esp_boxes;
extern bool  g_esp_names;
extern bool  g_esp_distance;
extern bool  g_chams_enabled;
extern float g_chams_r;
extern float g_chams_g;
extern float g_chams_b;

extern bool  g_no_recoil;
extern bool  g_no_spread;
extern bool  g_bunnyhop;

extern bool  g_chat_spam;
extern float g_spam_interval;

extern bool  g_hwid_spoof;
extern bool  g_bypass_actk;
extern bool  g_bypass_version;
extern bool  g_bypass_rootdetect;

extern bool  g_skin_changer_enabled;
extern int   g_skin_id;
extern int   g_skin_seed;
extern bool  g_skin_stattrak;
extern int   g_stattrak_value;
extern bool  g_skin_update_players;

// Состояние меню
extern bool  g_menu_visible;
extern float g_menu_opacity;
extern float g_menu_scale;
extern int   g_menu_position;
extern float g_menu_anim_speed;
extern int   g_menu_font_size;

// === Функции ===
void InstallAllPatches();
void Config_Load();
void Config_Save();

void  UpdatePlayerList();
void* GetLocalPlayer();
void* GetCamera();
int   GetPlayerCount();
void* GetPlayer(int idx);
Vec3  GetPlayerPosition(void* player);
Vec3  GetCameraForward();
Vec3  WorldToScreenPoint(Vec3 world);
float Distance3D(Vec3 a, Vec3 b);
void  ChatSpamUpdate(float dt);
void  UpdateFeaturesReal();
void  ApplySkinChanger(void* weapon, const char* skinName, void* stickers, int seed);

// === Lогирование (через stderr + буфер для ImGui меню) ===
void CFLog(const char* fmt, ...) __attribute__((format (printf, 1, 2)));

// Буфер логов для отображения в ImGui меню на экране iPad
extern char g_log_buffer[4096];
extern int  g_log_lines;