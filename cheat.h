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

struct Vec2  { F32 x, y; };
struct Vec3  { F32 x, y, z; };
struct Quat  { F32 x, y, z, w; };
struct Color { F32 r, g, b, a; };

static inline Vec3 V3(F32 x, F32 y, F32 z) { return Vec3{ x, y, z }; }

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

// === Globals (mirror config for menu) ===
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

// Menu state
extern bool  g_menu_visible;
extern float g_menu_slide_offset;
extern float g_menu_anim_speed;
extern int   g_menu_font_size;

// === Functions ===
void InstallAllPatches();
void Config_Load();
void Config_Save();

void* GetLocalPlayer();
void* GetCamera();
int   GetPlayerCount();
void* GetPlayer(int idx);
Vec3  GetPlayerPosition(void* player);
Vec3  WorldToScreenPoint(Vec3 world);
float Distance3D(Vec3 a, Vec3 b);
void  ChatSpamUpdate(float dt);
void  UpdateFeatures();  // Вызывается каждый кадр для обновления читов

// Menu
extern "C" void MenuInit();
extern "C" void MenuToggle();
extern "C" bool MenuIsOpen();
extern "C" void MenuSetOpen(bool open);
extern "C" void MenuRender(float dt, void* renderPassDescriptor);