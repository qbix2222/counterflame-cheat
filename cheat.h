#pragma once

#include <cstdint>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>

typedef uintptr_t UPtr;
typedef int32_t   S32;
typedef uint32_t  U32;
typedef uint8_t   U8;
typedef float     F32;
typedef int64_t   S64;
typedef uint64_t  U64;

struct Vec3  { F32 x, y, z; };
struct Quat  { F32 x, y, z, w; };
struct Color { F32 r, g, b, a; };

/* ---------- Хелперы ---------- */

static inline Vec3 V3(F32 x, F32 y, F32 z) { Vec3 v{ x,y,z }; return v; }

/* ---------- Игровые структуры (из Il2CppDumper dump.cs, поля могут смещаться) ---------- */

struct Transform {
    UPtr klass;
    UPtr monitor;
    UPtr pad[8];
};

struct GameObject {
    UPtr klass;
    UPtr monitor;
    UPtr pad[8];
};

struct Player {
    UPtr klass;
    UPtr monitor;
    UPtr gameObject;
    UPtr transform;
    UPtr health;
    UPtr maxHealth;
    UPtr team;
    UPtr name;
    UPtr actorNumber;
    UPtr isLocal;
    UPtr isAlive;
    UPtr isBot;
    UPtr pad[16];
};

struct GameController {
    UPtr klass;
    UPtr monitor;
    UPtr players;      // массив UPtr указателей на Player
    UPtr playerCount;
    UPtr localPlayer;  // UPtr на Player (local)
    UPtr camera;       // UPtr на Camera
    UPtr pad[16];
};

struct Camera {
    UPtr klass;
    UPtr monitor;
    UPtr pad[8];
};

struct WeaponParameters {
    F32 spread;
    F32 recoil;
    F32 damage;
    U32  bulletCount;
    F32 hipFireAccuracy;
    F32 movementAccuracy;
    F32 jumpAccuracy;
    F32 crouchAccuracy;
    F32 proneAccuracy;
    U8  pad[128];
};

/* ---------- Сигнатуры игровых методов (адрес = base + RVA) ---------- */

typedef void*   (*GameObject_Find_t)(void* name);
typedef void*   (*GetComponent_t)(void* go, void* type);
typedef Vec3    (*Transform_GetPosition_t)(void* tr);
typedef Vec3    (*Transform_GetForward_t)(void* tr);
typedef void    (*Transform_SetRotation_t)(void* tr, Quat q);
typedef Vec3    (*Camera_WorldToScreen_t)(void* cam, Vec3 world);
typedef bool    (*Input_GetButton_t)(void* name);
typedef bool    (*CharController_IsGrounded_t)(void* cc);
typedef void    (*FirePellets_t)(void* wp, Vec3 origin, Quat rot);
typedef void    (*SendChat_t)(void* msg);
typedef void*   (*GetDeviceUID_t)();
typedef void    (*WallHackDetector_t)(void* cb, Vec3 pos);
typedef void    (*ShootRaycastModule_t)();

/* ---------- Таблица RVA (из unpacked/il2cpp_dump/dump.cs) ---------- */

struct Offset { const char* name; UPtr rva; };

static const Offset g_offsets[] = {
    { "FirePellets",                        0x12783E4 },
    { "PlayerDamage",                       0x12796AC },
    { "AddVisible",                         0x15C9670 },
    { "WorldToScreenPoint",                 0x2B73E5C },
    { "Camera_main",                        0x2B741F0 },
    { "Transform_get_position",             0x2BB4894 },
    { "Transform_set_rotation",             0x2BB4B58 },
    { "Transform_get_forward",              0x2BA2D4C },
    { "Input_GetButton",                    0x24DCF0C },
    { "CharacterController_isGrounded",     0x2BFCC04 },
    { "SendChatMessage",                    0x158D6B0 },
    { "SystemInfo_deviceUniqueIdentifier",  0x2BB19D8 },
    { "WallHackDetector_StartDetection",    0x1238D04 },
    { "ShootRaycastModule",                 0x1239F20 },
    { "GameObject_Find",                    0x2BAB388 },
    { "Component_GetComponent",             0x2BAA268 },
    { 0, 0 }
};

/* ---------- Состояние чита ---------- */

extern UPtr g_base;

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

extern bool  g_menu_open;

struct CheatConfig {
    bool  silent_aim;
    float fov;
    int   target_bone;
    float max_distance;
    bool  predictive_aim;
    int   prediction_ms;
    bool  spin_enabled;
    float spin_speed;
    int   spin_mode;
    bool  spin_invert_x;
    bool  esp_boxes;
    bool  esp_names;
    bool  esp_distance;
    bool  chams_enabled;
    float chams_r;
    float chams_g;
    float chams_b;
    bool  no_recoil;
    bool  no_spread;
    bool  bunnyhop;
    bool  chat_spam;
    float spam_interval;
    bool  hwid_spoof;
    bool  bypass_actk;
};

extern CheatConfig g_config;

/* ---------- Публичный API ---------- */

void InstallAllHooks(UPtr base);
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
void  ChatSpamUpdate(F32 dt);

/* Меню (реализовано в menu.mm) */
extern "C" void MenuInit();
extern "C" void MenuToggle();
extern "C" bool MenuIsOpen();
extern "C" void MenuSetOpen(bool open);
