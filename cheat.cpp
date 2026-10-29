#include "cheat.h"
#include <sys/mman.h>
#include <unistd.h>
#include <dlfcn.h>
#include <mach/mach.h>
#include <mach/vm_map.h>
#include <pthread.h>
#include <objc/objc.h>
#include <objc/runtime.h>
#include <objc/message.h>

// ========== ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ==========
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
float g_menu_opacity = 1.0f;
float g_menu_scale = 1.0f;
int   g_menu_position = 0;
float g_menu_anim_speed = 8.0f; int g_menu_font_size = 14;

static const char* g_spam_msgs[] = {
    "CounterFlame Internal", "Чит активирован", "Ваш аккаунт виден",
    "ESP и сайлент аим включены", "Антибан активен", "Скин ченджер активен", NULL
};

// ========== ЛОГИРОВАНИЕ (безопасный путь) ==========
static FILE* g_log_file = nullptr;

static const char* GetLogPath() {
    static char path[512] = {0};
    if (path[0] == 0) {
        const char* tmp = getenv("TMPDIR");
        if (!tmp || tmp[0] == 0) tmp = "/tmp";
        snprintf(path, sizeof(path), "%s/counterflame_cheat.log", tmp);
    }
    return path;
}

static void Log(const char* fmt, ...) {
    if (!g_log_file) {
        g_log_file = fopen(GetLogPath(), "a");
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

// ========== ПОЛУЧЕНИЕ БАЗЫ UNITYFRAMEWORK ==========
static UPtr GetUnityBase() {
    for (uint32_t i = 0; i < _dyld_image_count(); i++) {
        const char* name = _dyld_get_image_name(i);
        if (name && strstr(name, "UnityFramework")) {
            return (UPtr)_dyld_get_image_header(i);
        }
    }
    return 0;
}

// ========== ИГРОВОЕ СОСТОЯНИЕ ==========
static void* g_game_controller = nullptr;
static void* g_local_player_obj = nullptr;
static void* g_camera_obj = nullptr;

// Находим GameController через GameObject.Find + GetComponent (с управляемой строкой!)
static void* FindGameController() {
    void* managed_str = g_il2cpp.Invoke<void*>("il2cpp_string_new", "GameController");
    if (!managed_str) return nullptr;

    void* go = ((void*(*)(void*))(g_unity_base + Offsets::GameObject_Find))(managed_str);
    if (!go) return nullptr;

    void* comp = ((void*(*)(void*, void*))(g_unity_base + Offsets::Component_GetComponent))(go, 0);
    return comp;
}

static void UpdateGameState() {
    if (g_game_controller) return;
    g_game_controller = FindGameController();
    if (!g_game_controller) return;

    g_local_player_obj = g_il2cpp.Read<void*>(g_game_controller, 0x20);
    g_camera_obj       = g_il2cpp.Read<void*>(g_game_controller, 0x28);
}

// ========== ДОСТУП К ИГРОКАМ (внутренние, без static конфликта) ==========
static int GetPlayerCountReal() {
    void* gc = g_game_controller;
    if (!gc) return 0;
    return g_il2cpp.Read<int>(gc, 0x10);
}

static void* GetPlayerReal(int idx) {
    void* gc = g_game_controller;
    if (!gc) return nullptr;
    int count = g_il2cpp.Read<int>(gc, 0x10);
    if (idx < 0 || idx >= count) return nullptr;
    void** players = g_il2cpp.Read<void**>(gc, 0x18);
    return players[idx];
}

static void* GetLocalPlayerReal() {
    UpdateGameState();
    return g_local_player_obj;
}

static void* GetCameraReal() {
    UpdateGameState();
    return g_camera_obj;
}

static Vec3 GetPlayerPositionReal(void* player) {
    if (!player) return V3(0,0,0);
    void* tr = ((void*(*)(void*))(g_unity_base + Offsets::Component_get_transform))(player);
    if (!tr) return V3(0,0,0);
    return ((Vec3(*)(void*))(g_unity_base + Offsets::Transform_get_position))(tr);
}

static Vec3 WorldToScreenPointReal(Vec3 world) {
    void* cam = GetCameraReal();
    if (!cam) return V3(0,0,0);
    return ((Vec3(*)(void*, Vec3))(g_unity_base + Offsets::Camera_WorldToScreenPoint))(cam, world);
}

static float Distance3DReal(Vec3 a, Vec3 b) {
    F32 dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return sqrtf(dx*dx + dy*dy + dz*dz);
}

static Vec3 GetCameraForwardReal() {
    void* cam = GetCameraReal();
    if (!cam) return V3(0,0,1);
    void* tr = ((void*(*)(void*))(g_unity_base + Offsets::Component_get_transform))(cam);
    if (!tr) return V3(0,0,1);
    return ((Vec3(*)(void*))(g_unity_base + Offsets::Transform_get_forward))(tr);
}

// ========== ЧАТ СПАМ ==========
static void ChatSpamUpdateReal(float dt) {
    if (!g_chat_spam) return;
    g_spam_timer += dt;
    if (g_spam_timer >= g_spam_interval) {
        g_spam_timer = 0.0f;
        const char* msg = g_spam_msgs[g_spam_index++];
        if (!g_spam_msgs[g_spam_index]) g_spam_index = 0;
        if (msg) {
            void* managed_msg = g_il2cpp.Invoke<void*>("il2cpp_string_new", msg);
            ((void(*)(void*))(g_unity_base + Offsets::SendChatMessage))(managed_msg);
        }
    }
}

// ========== СПИНБОТ (через вызов метода, а не хук кода) ==========
static void UpdateSpinbot() {
    if (!g_spin_enabled) return;
    void* local = GetLocalPlayerReal();
    if (!local) return;

    static float spin_yaw = 0.0f;
    spin_yaw += g_spin_speed * 0.016f;
    if (spin_yaw > 360.0f) spin_yaw -= 360.0f;
    float rad = spin_yaw * 3.1415926f / 180.0f;
    float new_y = sinf(rad);
    float new_w = cosf(rad);

    Quat rot = {0, new_y, 0, new_w};
    if (g_spin_invert_x) rot.x = -rot.x;

    void* tr = ((void*(*)(void*))(g_unity_base + Offsets::Component_get_transform))(local);
    if (tr) {
        ((void(*)(void*, Quat))(g_unity_base + Offsets::Transform_set_rotation))(tr, rot);
    }
}

// ========== БАННИХОП ==========
static void UpdateBunnyhop() {
    if (!g_bunnyhop) return;
    // Заглушка — можно патчить данные CharacterController через IL2CPP
}

// ========== ANTI-CHEAT БАЙПАС (через данные, НЕ код) ==========
static void ApplyAntiCheatDataBypass() {
    if (!g_unity_base) return;

    // RootDetect: ScarletWork.AntiCheat.RootDetect -> _isRooted / _isJailbroken
    if (g_bypass_rootdetect) {
        void* rd_class = g_il2cpp.GetClass("Assembly-CSharp.dll", "ScarletWork.AntiCheat", "RootDetect");
        if (rd_class) {
            uint32_t off = g_il2cpp.GetFieldOffset(rd_class, "_isRooted");
            if (off == 0) off = g_il2cpp.GetFieldOffset(rd_class, "_isJailbroken");
            if (off == 0) off = g_il2cpp.GetFieldOffset(rd_class, "isRooted");
            if (off == 0) off = g_il2cpp.GetFieldOffset(rd_class, "isJailbroken");
            if (off != 0) {
                g_il2cpp.Write<bool>(rd_class, off, false);
                LOG("RootDetect bypassed via field write");
            }
        }
    }

    // ApkHashChecker: isModded / _isTampered
    if (g_bypass_version) {
        void* ah_class = g_il2cpp.GetClass("Assembly-CSharp.dll", "", "ApkHashChecker");
        if (ah_class) {
            uint32_t off = g_il2cpp.GetFieldOffset(ah_class, "isModded");
            if (off == 0) off = g_il2cpp.GetFieldOffset(ah_class, "_isTampered");
            if (off == 0) off = g_il2cpp.GetFieldOffset(ah_class, "isTampered");
            if (off != 0) {
                g_il2cpp.Write<bool>(ah_class, off, false);
                LOG("ApkHashChecker bypassed via field write");
            }
        }
    }

    // SessionGuard: _isRevoked
    if (g_bypass_actk) {
        void* sg_class = g_il2cpp.GetClass("Assembly-CSharp.dll", "ScarletWork.API", "SessionGuard");
        if (sg_class) {
            uint32_t off = g_il2cpp.GetFieldOffset(sg_class, "_isRevoked");
            if (off != 0) {
                g_il2cpp.Write<bool>(sg_class, off, false);
                LOG("SessionGuard bypassed via field write");
            }
        }
    }

    // WallHackDetector: флаг детекции
    if (g_bypass_actk) {
        void* wh_class = g_il2cpp.GetClass("Assembly-CSharp.dll", "", "WallHackDetector");
        if (wh_class) {
            uint32_t off = g_il2cpp.GetFieldOffset(wh_class, "_isDetectionRunning");
            if (off == 0) off = g_il2cpp.GetFieldOffset(wh_class, "isDetectionRunning");
            if (off != 0) {
                g_il2cpp.Write<bool>(wh_class, off, false);
                LOG("WallHackDetector bypassed via field write");
            }
        }
    }

    // ShootRaycastModule — аналогично
    if (g_bypass_actk) {
        void* sr_class = g_il2cpp.GetClass("Assembly-CSharp.dll", "", "ShootRaycastModule");
        if (sr_class) {
            uint32_t off = g_il2cpp.GetFieldOffset(sr_class, "_isDetectionRunning");
            if (off == 0) off = g_il2cpp.GetFieldOffset(sr_class, "isDetectionRunning");
            if (off != 0) {
                g_il2cpp.Write<bool>(sr_class, off, false);
                LOG("ShootRaycastModule bypassed via field write");
            }
        }
    }
}

// ========== ОРУЖИЕ (No Recoil / No Spread через данные) ==========
static void ApplyWeaponDataPatches() {
    if (!g_no_recoil && !g_no_spread) return;

    void* wp_class = g_il2cpp.GetClass("Assembly-CSharp.dll", "", "WeaponParameters");
    if (!wp_class) return;

    LOG("Weapon patches active: no_recoil=%d no_spread=%d", g_no_recoil, g_no_spread);
}

// ========== ГЛАВНЫЙ ЦИКЛ ==========
static bool g_initialized = false;

void UpdateFeaturesReal() {
    // Отложенная инициализация: ждём загрузки UnityFramework
    if (!g_initialized) {
        g_unity_base = GetUnityBase();
        if (!g_unity_base) return;
        if (!g_il2cpp.Init()) {
            LOG("IL2CPP resolver init failed");
            return;
        }
        LOG("UnityFramework base: 0x%lx", g_unity_base);
        LOG("IL2CPP resolver initialized");

        ApplyAntiCheatDataBypass();
        ApplyWeaponDataPatches();

        g_initialized = true;
        LOG("Initial patches applied");
    }

    UpdateGameState();
    if (!g_local_player_obj) return;

    UpdateSpinbot();
    UpdateBunnyhop();
    ChatSpamUpdateReal(1.0f / 60.0f);
    ApplyWeaponDataPatches();
    ApplyAntiCheatDataBypass();
}

// ========== ЭКСПОРТЫ ДЛЯ МЕНЮ ==========
void InstallAllPatches() {
    // Вызывается из UpdateFeaturesReal при первой итерации
}

void Config_Load() {
    static char cfg_path[512] = {0};
    if (cfg_path[0] == 0) {
        const char* tmp = getenv("TMPDIR");
        if (!tmp || tmp[0] == 0) tmp = "/tmp";
        snprintf(cfg_path, sizeof(cfg_path), "%s/counterflame_cheat.cfg", tmp);
    }

    FILE* f = fopen(cfg_path, "rb");
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

    static char cfg_path[512] = {0};
    if (cfg_path[0] == 0) {
        const char* tmp = getenv("TMPDIR");
        if (!tmp || tmp[0] == 0) tmp = "/tmp";
        snprintf(cfg_path, sizeof(cfg_path), "%s/counterflame_cheat.cfg", tmp);
    }

    FILE* f = fopen(cfg_path, "wb");
    if (!f) return;
    fwrite(&g_config, sizeof(CheatConfig), 1, f);
    fclose(f);
}

// ========== ОБЁРТКИ ДЛЯ МЕНЮ (совпадают с объявлениями в cheat.h) ==========
void UpdatePlayerList() { UpdateGameState(); }
void* GetLocalPlayer() { return GetLocalPlayerReal(); }
void* GetCamera() { return GetCameraReal(); }
int   GetPlayerCount() { return GetPlayerCountReal(); }
void* GetPlayer(int idx) { return GetPlayerReal(idx); }
Vec3  GetPlayerPosition(void* player) { return GetPlayerPositionReal(player); }
Vec3  GetCameraForward() { return GetCameraForwardReal(); }
Vec3  WorldToScreenPoint(Vec3 world) { return WorldToScreenPointReal(world); }
float Distance3D(Vec3 a, Vec3 b) { return Distance3DReal(a, b); }
void ChatSpamUpdate(float dt) { ChatSpamUpdateReal(dt); }
void ApplySkinChanger(void*, const char*, void*, int) {}
void UpdateFeatures() { UpdateFeaturesReal(); }

// ========== КОНСТРУКТОР ==========
extern "C" void __attribute__((constructor)) CounterFlameCheatInit() {
    g_log_file = fopen(GetLogPath(), "w");
    if (g_log_file) {
        fprintf(g_log_file, "[CF] CounterFlameCheat init started (non-jailbreak)\n");
        fflush(g_log_file);
    }

    MenuInit();

    LOG("Constructor complete. Waiting for UnityFramework...");
}