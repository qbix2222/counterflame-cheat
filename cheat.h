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

static inline Vec3 V3(F32 x, F32 y, F32 z) { Vec3 v{ x,y,z }; return v; }

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
    UPtr players;
    UPtr playerCount;
    UPtr localPlayer;
    UPtr camera;
    UPtr pad[16];
};

struct Camera {
    UPtr klass;
    UPtr monitor;
    UPtr pad[8];
};

struct WeaponParameters {
    UPtr klass;
    UPtr monitor;
    int    weaponId;
    int    skinId;
    int    stattrak;
    int    seed;
    float  spread;
    float  recoil;
    float  damage;
    U32    bulletCount;
    float  fireRate;
    U32    ammoPerShot;
    U32    currentAmmo;
    U32    reserveAmmo;
    F32    reloadTime;
    F32    hipFireAccuracy;
    F32    movementAccuracy;
    U8     pad[128];
};

struct StickerData {
    UPtr klass;
    UPtr monitor;
    int    stickerId;
    int    slot;
    float  wear;
    U8     pad[64];
};

struct RaycastHit {
    Vec3 point;
    Vec3 normal;
    UPtr collider;
    UPtr rigidbody;
    UPtr transform;
    U32  triangleIndex;
    F32  distance;
    Vec2 textureCoord;
    Vec2 lightmapCoord;
    U32  layer;
};

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
typedef bool    (*RootDetect_Checkroot_t)();
typedef void    (*SessionGuard_Revoke_t)(const char* reason);
typedef void    (*SessionGuard_Reset_t)();
typedef const char* (*ScarletVersion_GetVersion_t)();
typedef void    (*ScarletVersion_cctor_t)();
typedef void    (*ApkHashChecker_ctor_t)();

typedef void    (*PublishLook_t)(void* weaponParams, void* skin, void* stickers, int seed);
typedef void    (*SetSkin_t)(void* weaponParams, void* skin, void* stattrak, bool isPickup, void* stickers, int seed, int owner);
typedef void    (*ApplySkin_t)(void* weaponParams, void* skin, void* stattrak, void* stickers, int seed, int generation);
typedef void    (*ApplyStickers_t)(int index, void* weapon, void* weaponParams, void* stickers);

struct Offset { const char* name; UPtr rva; };

static const Offset g_offsets[] = {
    { "FirePellets",                       0x12783E4 },
    { "PlayerDamage",                      0x12796AC },
    { "AddVisible",                        0x15C9670 },
    { "WorldToScreenPoint",                0x2B73E5C },
    { "Camera_main",                       0x2B741F0 },
    { "Transform_get_position",            0x2BB4894 },
    { "Transform_set_rotation",            0x2BB4B58 },
    { "Transform_get_forward",             0x2BA2D4C },
    { "Input_GetButton",                   0x24DCF0C },
    { "CharacterController_isGrounded",    0x2BFCC04 },
    { "SendChatMessage",                   0x158D6B0 },
    { "SystemInfo_deviceUniqueIdentifier", 0x2BB19D8 },
    { "WallHackDetector_StartDetection",   0x1234B94 },
    { "ShootRaycastModule",                0x1239F20 },
    { "RootDetect_Start",                  0x140ED60 },
    { "RootDetect_Checkroot",              0x140EDF0 },
    { "ApkHashChecker_ctor",               0x126F358 },
    { "SessionGuard_Revoke",               0x13F00CC },
    { "SessionGuard_Reset",                0x13F03D4 },
    { "ScarletVersion_cctor",              0x13F044C },
    { "ScarletVersion_GetVersion",         0x13F00CC },
    { "GameObject_Find",                   0x2BAB388 },
    { "Component_GetComponent",            0x2BAA268 },
    { "PublishLook",                       0x1275CBC },
    { "SetSkin_Full",                      0x12727E0 },
    { "ApplySkin",                         0x12759E4 },
    { "SetSkin_Simple",                    0x1276368 },
    { "ApplyStickers",                     0x143057C },
    { "SetInventory",                      0x146F390 },
    { 0, 0 }
};

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
extern bool  g_bypass_version;
extern bool  g_bypass_rootdetect;

extern bool  g_skin_changer_enabled;
extern int   g_skin_id;
extern int   g_skin_seed;
extern bool  g_skin_stattrak;
extern int   g_stattrak_value;
extern bool  g_skin_update_players;

extern bool  g_menu_open;
extern bool  g_menu_visible;
extern float g_menu_slide_offset;
extern bool  g_menu_toggle_btn;
extern bool  g_menu_tweaks_enabled;
extern float g_menu_opacity;
extern float g_menu_scale;
extern int   g_menu_position;
extern float g_menu_anim_speed;
extern int   g_menu_font_size;

extern CheatConfig g_config;

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
void  ApplySkinChanger(void* weapon, const char* skinName, void* stickers, int seed);

extern "C" void MenuInit();
extern "C" void MenuToggle();
extern "C" bool MenuIsOpen();
extern "C" void MenuSetOpen(bool open);
extern "C" void MenuRender(float dt, id<MTLRenderPassDescriptor> renderPassDescriptor);
extern id<MTLRenderPassDescriptor> g_currentRenderPassDescriptor;
extern id<MTLRenderPassDescriptor> g_currentRenderPassDescriptor;
