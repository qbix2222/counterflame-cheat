#include "cheat.h"
#include <sys/mman.h>
#include <unistd.h>
#include <dlfcn.h>
#include <mach/mach.h>
#include <mach/vm_map.h>
#include <pthread.h>

// ========== GLOBAL VARIABLES (definitions) ==========
UPtr g_unity_base = 0;
CheatConfig g_config = {};

bool  g_silent_aim = false;   float g_fov = 25.0f;          int   g_target_bone = 0;
float g_max_distance = 150.0f; bool  g_predictive_aim = false; int   g_prediction_ms = 100;

bool  g_spin_enabled = false;  float g_spin_speed = 180.0f;  int   g_spin_mode = 1;
bool  g_spin_invert_x = false;

bool  g_esp_boxes = true;  bool  g_esp_names = true;  bool  g_esp_distance = true;
bool  g_chams_enabled = false; float g_chams_r = 1.0f, g_chams_g = 0.0f, g_chams_b = 0.0f;

bool  g_no_recoil = true;  bool  g_no_spread = true;  bool  g_bunnyhop = false;

bool  g_chat_spam = false; float g_spam_interval = 5.0f;
int   g_spam_index = 0; float g_spam_timer = 0.0f;

bool  g_hwid_spoof = false; bool  g_bypass_actk = true;
bool  g_bypass_version = true; bool  g_bypass_rootdetect = true;

bool  g_skin_changer_enabled = false; int g_skin_id = 1; int g_skin_seed = 1337;
bool  g_skin_stattrak = true; int g_stattrak_value = 1337; bool g_skin_update_players = true;

bool  g_menu_visible = false; float g_menu_slide_offset = 0.0f;
float g_menu_anim_speed = 8.0f; int g_menu_font_size = 14;

static const char* g_spam_msgs[] = {
    "CounterFlame Internal", "Чит активирован", "Ваш аккаунт виден",
    "ESP и сайлент аим включены", "Антибан активен", "Скин ченджер активен", NULL
};

// ========== OFFSETS from dump.cs ==========
struct Offsets {
    static constexpr uint32_t WP_spread = 0x50;
    static constexpr uint32_t WP_recoil = 0x54;
    static constexpr uint32_t WP_hipFireAccuracy = 0x58;
    static constexpr uint32_t WP_movementAccuracy = 0x5C;

    static constexpr uint32_t Player_transform = 0x38;
    static constexpr uint32_t Player_isAlive = 0x30;

    // Function RVAs
    static constexpr UPtr GameObject_Find = 0x2BAB388;
    static constexpr UPtr Component_GetComponent = 0x2BAA268;
    static constexpr UPtr Transform_get_position = 0x2BB4894;
    static constexpr UPtr Transform_set_rotation = 0x2BB4B58;
    static constexpr UPtr Transform_get_forward = 0x2BB4DE4;
    static constexpr UPtr Camera_WorldToScreenPoint = 0x2B73E5C;
    static constexpr UPtr Input_GetButton = 0x24DCF0C;
    static constexpr UPtr SendChatMessage = 0x158D6B0;
    static constexpr UPtr WallHackDetector_StartDetection = 0x12388F0;
    static constexpr UPtr ShootRaycastModule = 0x1239F20;
    static constexpr UPtr RootDetect_Checkroot = 0x140EDF0;
    static constexpr UPtr ApkHashChecker_ctor = 0x126F358;
    static constexpr UPtr SessionGuard_Revoke = 0x13F0128;
    static constexpr UPtr SessionGuard_Reset = 0x13F03D4;
    static constexpr UPtr ScarletVersion_cctor = 0x13F044C;
    static constexpr UPtr ScarletVersion_GetVersion = 0x13F0658;
    static constexpr UPtr PublishLook = 0x1275CBC;
    static constexpr UPtr CharacterController_isGrounded = 0x2BFCC04;
    static constexpr UPtr SystemInfo_deviceUniqueIdentifier = 0x2BB19D8;
    static constexpr UPtr Camera_main = 0x2B741F0;
};

// ========== LOGGING ==========
static FILE* g_log_file = nullptr;

static void Log(const char* fmt, ...) {
    if (!g_log_file) {
        g_log_file = fopen("/var/mobile/Containers/Data/Application/CounterFlame/cheat.log", "a");
    }
    if (!g_log_file) return;
    va_list args;
    va_start(args, fmt);
    vfprintf(g_log_file, fmt, args);
    fprintf(g_log_file, "\n");
    fflush(g_log_file);
    va_end(args);
}

#define LOG(fmt, ...) Log("[CF] " fmt, ##__VA_ARGS__)

// ========== GAME STATE ==========
static void* g_game_controller = nullptr;
static void* g_local_player_obj = nullptr;
static void* g_camera_obj = nullptr;

static UPtr GetUnityBase() {
    for (uint32_t i = 0; i < _dyld_image_count(); i++) {
        const char* name = _dyld_get_image_name(i);
        if (name && strstr(name, "UnityFramework")) {
            return (UPtr)_dyld_get_image_header(i);
        }
    }
    return 0;
}

static void UpdateGameState() {
    if (g_game_controller) return; // already found once
    // GameObject.Find("GameController")
    void* go = ((void*(*)(void*))(g_unity_base + Offsets::GameObject_Find))((void*)"GameController");
    if (!go) return;
    void* comp = ((void*(*)(void*, void*))(g_unity_base + Offsets::Component_GetComponent))(go, 0);
    g_game_controller = comp;
}

static void* GetGameControllerPtr() {
    UpdateGameState();
    return g_game_controller;
}

static void UpdatePlayers() {
    void* gc = GetGameControllerPtr();
    if (!gc) return;
    // GameController fields: localPlayer at some offset, camera, players array
    // We'll read via IL2CPP resolver instead for safety
}

// ========== PLAYER ACCESS via IL2CPP ==========
static void* GetLocalPlayerObj() {
    if (g_local_player_obj) return g_local_player_obj;
    void* gc = GetGameControllerPtr();
    if (!gc) return nullptr;
    // localPlayer field - need offset. Use IL2CPP resolver
    return nullptr;
}

static void* GetCameraObj() {
    if (g_camera_obj) return g_camera_obj;
    void* gc = GetGameControllerPtr();
    if (!gc) return nullptr;
    return nullptr;
}

static int GetPlayerCountReal() {
    void* gc = GetGameControllerPtr();
    if (!gc) return 0;
    return 0;
}

static Vec3 GetPlayerPositionReal(void* player) {
    if (!player) return V3(0,0,0);
    void* tr = g_il2cpp.Read<void*>(player, Offsets::Player_transform);
    if (!tr) return V3(0,0,0);
    return ((Vec3(*)(void*))(g_unity_base + Offsets::Transform_get_position))(tr);
}

static Vec3 WorldToScreenPointReal(Vec3 world) {
    void* cam = GetCameraObj();
    if (!cam) return V3(0,0,0);
    return ((Vec3(*)(void*, Vec3))(g_unity_base + Offsets::Camera_WorldToScreenPoint))(cam, world);
}

static float Distance3DReal(Vec3 a, Vec3 b) {
    F32 dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return sqrtf(dx*dx + dy*dy + dz*dz);
}

// ========== CHAT SPAM ==========
static void ChatSpamUpdateReal(float dt) {
    if (!g_chat_spam) return;
    g_spam_timer += dt;
    if (g_spam_timer >= g_spam_interval) {
        g_spam_timer = 0.0f;
        const char* msg = g_spam_msgs[g_spam_index++];
        if (!g_spam_msgs[g_spam_index]) g_spam_index = 0;
        if (msg) ((void(*)(void*))(g_unity_base + Offsets::SendChatMessage))((void*)msg);
    }
}

// ========== ANTI-CHEAT BYPASS via vm_write ==========
static void ApplyAntiCheatBypasses() {
    if (!g_unity_base) return;

    // RootDetect_Checkroot -> return false
    if (g_bypass_rootdetect) {
        uint32_t patch[2] = { 0xD2800000, 0xD65F03C0 }; // MOV W0, #0; RET
        g_il2cpp.PatchBytes((void*)(g_unity_base + Offsets::RootDetect_Checkroot), patch, 8);
        LOG("Patched RootDetect_Checkroot");
    }

    // WallHackDetector_StartDetection -> NOP
    if (g_bypass_actk) {
        g_il2cpp.Nop((void*)(g_unity_base + Offsets::WallHackDetector_StartDetection), 2);
        g_il2cpp.Nop((void*)(g_unity_base + Offsets::ShootRaycastModule), 2);
        g_il2cpp.Nop((void*)(g_unity_base + Offsets::ApkHashChecker_ctor), 4);
        g_il2cpp.Nop((void*)(g_unity_base + Offsets::SessionGuard_Revoke), 2);
        g_il2cpp.Nop((void*)(g_unity_base + Offsets::SessionGuard_Reset), 2);
        LOG("Patched ACTk bypasses");
    }

    // ScarletVersion_cctor -> NOP
    if (g_bypass_version) {
        g_il2cpp.Nop((void*)(g_unity_base + Offsets::ScarletVersion_cctor), 2);
        LOG("Patched ScarletVersion");
    }
}

// ========== WEAPON PATCHES via vm_write ==========
static void ApplyWeaponPatches() {
    if (!g_no_recoil && !g_no_spread) return;

    // Find WeaponController / WeaponParameters via IL2CPP
    // For now, patch common weapon parameter offsets
    // This is a simplified approach — real implementation would iterate all WeaponParameters
    LOG("Weapon patches: no_recoil=%d no_spread=%d", g_no_recoil, g_no_spread);
}

// ========== SPINBOT ==========
static void UpdateSpinbot() {
    if (!g_spin_enabled) return;
    void* local = GetLocalPlayerObj();
    if (!local) return;

    static float spin_yaw = 0.0f;
    spin_yaw += g_spin_speed * 0.016f;
    if (spin_yaw > 360.0f) spin_yaw -= 360.0f;
    float rad = spin_yaw * 3.1415926f / 180.0f;
    float new_y = sinf(rad);
    float new_w = cosf(rad);

    Quat rot = {0, new_y, 0, new_w};
    if (g_spin_invert_x) rot.x = -rot.x;

    void* tr = g_il2cpp.Read<void*>(local, Offsets::Player_transform);
    if (tr) {
        ((void(*)(void*, Quat))(g_unity_base + Offsets::Transform_set_rotation))(tr, rot);
    }
}

// ========== SILENT AIM ==========
static void UpdateSilentAim() {
    if (!g_silent_aim) return;
    // Silent aim via silent direction modification — requires hooking FirePellets
    // For non-jb without Dobby, we can't easily hook the call.
    // Alternative: patch the aim direction in memory before the game reads it.
    LOG("Silent aim active (needs hook for full effect)");
}

// ========== MAIN UPDATE LOOP ==========
static void UpdateFeatures() {
    if (!g_unity_base) {
        g_unity_base = GetUnityBase();
        if (!g_unity_base) return;
        LOG("UnityFramework base: 0x%lx", g_unity_base);
        if (!g_il2cpp.Init()) {
            LOG("IL2CPP resolver init failed");
            return;
        }
        LOG("IL2CPP resolver initialized");
    }

    ApplyAntiCheatBypasses();
    ApplyWeaponPatches();
    UpdateSpinbot();
    UpdateSilentAim();
    ChatSpamUpdate(1.0f/60.0f);
}

// ========== EXTERN FUNCTIONS (matching cheat.h) ==========
void InstallAllPatches() {
    g_unity_base = GetUnityBase();
    if (!g_unity_base) {
        LOG("Failed to find UnityFramework base");
        return;
    }
    if (!g_il2cpp.Init()) {
        LOG("IL2CPP resolver init failed");
        return;
    }
    ApplyAntiCheatBypasses();
    ApplyWeaponPatches();
    LOG("All patches installed");
}

void Config_Load() {
    FILE* f = fopen("/var/mobile/Containers/Data/Application/CounterFlame/config.cfg", "rb");
    if (!f) return;
    fread(&g_config, sizeof(CheatConfig), 1, f);
    fclose(f);
    g_silent_aim = g_config.silent_aim;
    g_fov = g_config.fov;
    g_target_bone = g_config.target_bone;
    g_max_distance = g_config.max_distance;
    g_predictive_aim = g_config.predictive_aim;
    g_prediction_ms = g_config.prediction_ms;
    g_spin_enabled = g_config.spin_enabled;
    g_spin_speed = g_config.spin_speed;
    g_spin_mode = g_config.spin_mode;
    g_spin_invert_x = g_config.spin_invert_x;
    g_esp_boxes = g_config.esp_boxes;
    g_esp_names = g_config.esp_names;
    g_esp_distance = g_config.esp_distance;
    g_chams_enabled = g_config.chams_enabled;
    g_chams_r = g_config.chams_r;
    g_chams_g = g_config.chams_g;
    g_chams_b = g_config.chams_b;
    g_no_recoil = g_config.no_recoil;
    g_no_spread = g_config.no_spread;
    g_bunnyhop = g_config.bunnyhop;
    g_chat_spam = g_config.chat_spam;
    g_spam_interval = g_config.spam_interval;
    g_hwid_spoof = g_config.hwid_spoof;
    g_bypass_actk = g_config.bypass_actk;
    g_bypass_version = g_config.bypass_version;
    g_bypass_rootdetect = g_config.bypass_rootdetect;
    g_skin_changer_enabled = g_config.skin_changer_enabled;
    g_skin_id = g_config.skin_id;
    g_skin_seed = g_config.skin_seed;
    g_skin_stattrak = g_config.skin_stattrak;
    g_stattrak_value = g_config.stattrak_value;
    g_skin_update_players = g_config.skin_update_players;
}

void Config_Save() {
    g_config.silent_aim = g_silent_aim;
    g_config.fov = g_fov;
    g_config.target_bone = g_target_bone;
    g_config.max_distance = g_max_distance;
    g_config.predictive_aim = g_predictive_aim;
    g_config.prediction_ms = g_prediction_ms;
    g_config.spin_enabled = g_spin_enabled;
    g_config.spin_speed = g_spin_speed;
    g_config.spin_mode = g_spin_mode;
    g_config.spin_invert_x = g_spin_invert_x;
    g_config.esp_boxes = g_esp_boxes;
    g_config.esp_names = g_esp_names;
    g_config.esp_distance = g_esp_distance;
    g_config.chams_enabled = g_chams_enabled;
    g_config.chams_r = g_chams_r;
    g_config.chams_g = g_chams_g;
    g_config.chams_b = g_chams_b;
    g_config.no_recoil = g_no_recoil;
    g_config.no_spread = g_no_spread;
    g_config.bunnyhop = g_bunnyhop;
    g_config.chat_spam = g_chat_spam;
    g_config.spam_interval = g_spam_interval;
    g_config.hwid_spoof = g_hwid_spoof;
    g_config.bypass_actk = g_bypass_actk;
    g_config.bypass_version = g_bypass_version;
    g_config.bypass_rootdetect = g_bypass_rootdetect;
    g_config.skin_changer_enabled = g_skin_changer_enabled;
    g_config.skin_id = g_skin_id;
    g_config.skin_seed = g_skin_seed;
    g_config.skin_stattrak = g_skin_stattrak;
    g_config.stattrak_value = g_stattrak_value;
    g_config.skin_update_players = g_skin_update_players;

    FILE* f = fopen("/var/mobile/Containers/Data/Application/CounterFlame/config.cfg", "wb");
    if (!f) return;
    fwrite(&g_config, sizeof(CheatConfig), 1, f);
    fclose(f);
}

// ========== EXTERN WRAPPERS for menu.mm ==========
void UpdatePlayerList() { /* handled internally */ }
void* GetLocalPlayer() { return GetLocalPlayerObj(); }
void* GetCamera() { return GetCameraObj(); }
int GetPlayerCount() { return GetPlayerCountReal(); }
void* GetPlayer(int idx) { return nullptr; }
Vec3 GetPlayerPosition(void* player) { return GetPlayerPositionReal(player); }
Vec3 GetCameraForward() { return V3(0,0,1); }
Vec3 WorldToScreenPoint(Vec3 world) { return WorldToScreenPointReal(world); }
float Distance3D(Vec3 a, Vec3 b) { return Distance3DReal(a, b); }
void ChatSpamUpdate(float dt) { ChatSpamUpdateReal(dt); }
void ApplySkinChanger(void*, const char*, void*, int) {}

void UpdateFeatures() { UpdateFeatures(); }

// ========== CONSTRUCTOR ==========
extern "C" void __attribute__((constructor)) CounterFlameCheatInit() {
    g_log_file = fopen("/var/mobile/Containers/Data/Application/CounterFlame/cheat.log", "w");
    if (g_log_file) {
        fprintf(g_log_file, "[CF] CounterFlameCheat init started\n");
        fflush(g_log_file);
    }

    InstallAllPatches();
    Config_Load();
    MenuInit();

    LOG("Initialization complete");
}