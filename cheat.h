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
#include <unordered_map>

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

static inline Vec3 V3(F32 x, F32 y, F32 z) { Vec3 v{ x,y,z }; return v; }

struct UnityObject {
    void* klass;
    void* monitor;
};

struct Transform : UnityObject {};
struct GameObject : UnityObject {};
struct Camera : UnityObject {};

struct Component : UnityObject {
    GameObject* GetGameObject() {
        static void* method = g_il2cpp.GetMethod("UnityEngine.CoreModule.dll", "UnityEngine", "Component", "get_gameObject", 0);
        if (method) return g_il2cpp.Invoke<GameObject*>(method, this);
        return nullptr;
    }
    Transform* GetTransform() {
        static void* method = g_il2cpp.GetMethod("UnityEngine.CoreModule.dll", "UnityEngine", "Component", "get_transform", 0);
        if (method) return g_il2cpp.Invoke<Transform*>(method, this);
        return nullptr;
    }
};

struct Behaviour : Component {
    bool GetEnabled() {
        static void* method = g_il2cpp.GetMethod("UnityEngine.CoreModule.dll", "UnityEngine", "Behaviour", "get_enabled", 0);
        if (method) return g_il2cpp.Invoke<bool>(method, this);
        return false;
    }
    void SetEnabled(bool value) {
        static void* method = g_il2cpp.GetMethod("UnityEngine.CoreModule.dll", "UnityEngine", "Behaviour", "set_enabled", 1);
        if (method) g_il2cpp.Invoke<void>(method, this, value);
    }
};

struct MonoBehaviour : Behaviour {};

struct Player : MonoBehaviour {
    UPtr health;
    UPtr maxHealth;
    UPtr team;
    UPtr name;
    UPtr actorNumber;
    UPtr isLocal;
    UPtr isAlive;
    UPtr isBot;
    UPtr pad[8];

    Vec3 GetPosition() {
        Transform* t = GetTransform();
        if (!t) return V3(0,0,0);
        static void* method = g_il2cpp.GetMethod("UnityEngine.CoreModule.dll", "UnityEngine", "Transform", "get_position", 0);
        if (method) return g_il2cpp.Invoke<Vec3>(method, t);
        return V3(0,0,0);
    }

    Vec3 GetForward() {
        Transform* t = GetTransform();
        if (!t) return V3(0,0,1);
        static void* method = g_il2cpp.GetMethod("UnityEngine.CoreModule.dll", "UnityEngine", "Transform", "get_forward", 0);
        if (method) return g_il2cpp.Invoke<Vec3>(method, t);
        return V3(0,0,1);
    }

    void SetRotation(Quat q) {
        Transform* t = GetTransform();
        if (!t) return;
        static void* method = g_il2cpp.GetMethod("UnityEngine.CoreModule.dll", "UnityEngine", "Transform", "set_rotation", 1);
        if (method) g_il2cpp.Invoke<void>(method, t, q);
    }
};

struct Camera : UnityObject {
    Vec3 WorldToScreenPoint(Vec3 world) {
        static void* method = g_il2cpp.GetMethod("UnityEngine.CoreModule.dll", "UnityEngine", "Camera", "WorldToScreenPoint", 1);
        if (method) return g_il2cpp.Invoke<Vec3>(method, this, world);
        return V3(0,0,0);
    }

    Vec3 ScreenToWorldPoint(Vec3 screen) {
        static void* method = g_il2cpp.GetMethod("UnityEngine.CoreModule.dll", "UnityEngine", "Camera", "ScreenToWorldPoint", 1);
        if (method) return g_il2cpp.Invoke<Vec3>(method, this, screen);
        return V3(0,0,0);
    }

    static Camera* GetMain() {
        static void* method = g_il2cpp.GetMethod("UnityEngine.CoreModule.dll", "UnityEngine", "Camera", "get_main", 0);
        if (method) return g_il2cpp.Invoke<Camera*>(method);
        return nullptr;
    }
};

struct GameObject : UnityObject {
    static GameObject* Find(const char* name) {
        static void* method = g_il2cpp.GetMethod("UnityEngine.CoreModule.dll", "UnityEngine", "GameObject", "Find", 1);
        if (method) return g_il2cpp.Invoke<GameObject*>(method, (void*)name);
        return nullptr;
    }

    template <typename T>
    T* GetComponent() {
        static void* method = g_il2cpp.GetMethod("UnityEngine.CoreModule.dll", "UnityEngine", "GameObject", "GetComponent", 1);
        if (method) return g_il2cpp.Invoke<T*>(method, this, g_il2cpp.GetType(typeof(T)));
        return nullptr;
    }
};

struct WeaponParameters : UnityObject {
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
};

struct GameController : UnityObject {
    void* players;
    int   playerCount;
    Player* localPlayer;
    Camera* camera;

    static GameController* Find() {
        GameObject* go = GameObject::Find("GameController");
        if (!go) return nullptr;
        return go->GetComponent<GameController>();
    }
};

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
Player* GetLocalPlayer();
Camera* GetCamera();
int   GetPlayerCount();
Player* GetPlayer(int idx);
Vec3  GetPlayerPosition(Player* player);
Vec3  GetCameraForward();
Vec3  WorldToScreenPoint(Vec3 world);
float Distance3D(Vec3 a, Vec3 b);
void  ChatSpamUpdate(F32 dt);
void  ApplySkinChanger(void* weapon, const char* skinName, void* stickers, int seed);

extern "C" void MenuInit();
extern "C" void MenuToggle();
extern "C" bool MenuIsOpen();
extern "C" void MenuSetOpen(bool open);
extern "C" void MenuRender(float dt, void* renderPassDescriptor);

extern void* g_currentRenderPassDescriptor;

struct PatternScanner {
    static UPtr FindPattern(UPtr base, size_t size, const char* pattern, const char* mask);
    static UPtr FindPatternModule(const char* module_name, const char* pattern, const char* mask);
};

struct HookManager {
    static bool Hook(void* target, void* detour, void** original);
    static bool Unhook(void* target);
};