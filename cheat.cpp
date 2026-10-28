#include "cheat.h"
#include <sys/mman.h>
#include <unistd.h>
#include <dlfcn.h>
#include "dobby.h"

extern "C" void MenuInit();

UPtr g_base = 0;
void* g_currentRenderPassDescriptor = nullptr;

bool  g_silent_aim = false;
float g_fov = 25.0f;
int   g_target_bone = 0;
float g_max_distance = 150.0f;
bool  g_predictive_aim = false;
int   g_prediction_ms = 100;

bool  g_spin_enabled = false;
float g_spin_speed = 180.0f;
int   g_spin_mode = 1;
bool  g_spin_invert_x = false;

bool  g_esp_boxes = true;
bool  g_esp_names = true;
bool  g_esp_distance = true;
bool  g_chams_enabled = false;
float g_chams_r = 1.0f;
float g_chams_g = 0.0f;
float g_chams_b = 0.0f;

bool  g_no_recoil = true;
bool  g_no_spread = true;
bool  g_bunnyhop = false;

bool  g_chat_spam = false;
float g_spam_interval = 5.0f;
int   g_spam_index = 0;
float g_spam_timer = 0.0f;

bool  g_hwid_spoof = false;
bool  g_bypass_actk = true;
bool  g_bypass_version = true;
bool  g_bypass_rootdetect = true;

bool  g_skin_changer_enabled = false;
int   g_skin_id = 1;
int   g_skin_seed = 1337;
bool  g_skin_stattrak = true;
int   g_stattrak_value = 1337;
bool  g_skin_update_players = true;

bool  g_menu_open = false;
bool  g_menu_visible = false;
float g_menu_slide_offset = 0.0f;
bool  g_menu_toggle_btn = true;

bool  g_menu_tweaks_enabled = false;
float g_menu_opacity = 1.0f;
float g_menu_scale = 1.0f;
int   g_menu_position = 0;
float g_menu_anim_speed = 8.0f;
int   g_menu_font_size = 14;
ImVec4 g_menu_color = ImVec4(0.10f, 0.10f, 0.12f, 0.98f);
ImVec4 g_menu_accent_color = ImVec4(0.0f, 1.0f, 0.5f, 1.0f);

CheatConfig g_config = {};

static const char* g_spam_msgs[] = {
    "CounterFlame Internal",
    "Чит активирован",
    "Ваш аккаунт виден",
    "ESP и сайлент аим включены",
    "Антибан активен",
    "Скин ченджер активен",
    NULL
};

static void* g_real_firepellets = 0;
static void* g_real_setrotation = 0;
static void* g_real_getbutton = 0;
static void* g_real_sendchat = 0;
static void* g_real_hwid = 0;
static void* g_real_wallhack = 0;
static void* g_real_shootraycast = 0;
static void* g_real_rootdetect_checkroot = 0;
static void* g_real_apkhash_ctor = 0;
static void* g_real_sessionguard_revoke = 0;
static void* g_real_sessionguard_reset = 0;
static void* g_real_scarletversion_cctor = 0;
static void* g_real_scarletversion_getversion = 0;
static void* g_real_publishlook = 0;

Player* g_local_player = nullptr;
Camera* g_camera = nullptr;
GameController* g_game_controller = nullptr;

UPtr PatternScanner::FindPattern(UPtr base, size_t size, const char* pattern, const char* mask) {
    for (size_t i = 0; i < size - strlen(mask); i++) {
        bool found = true;
        for (size_t j = 0; mask[j]; j++) {
            if (mask[j] == 'x' && *(uint8_t*)(base + i + j) != (uint8_t)pattern[j]) {
                found = false;
                break;
            }
        }
        if (found) return base + i;
    }
    return 0;
}

UPtr PatternScanner::FindPatternModule(const char* module_name, const char* pattern, const char* mask) {
    return 0;
}

bool HookManager::Hook(void* target, void* detour, void** original) {
    return DobbyHook(target, detour, original) == 0;
}

bool HookManager::Unhook(void* target) {
    return DobbyDestroy(target) == 0;
}

void UpdatePlayerList() {
    if (!g_game_controller) {
        g_game_controller = GameController::Find();
    }
    if (!g_game_controller) return;
    g_local_player = g_game_controller->localPlayer;
    g_camera = g_game_controller->camera;
}

Player* GetLocalPlayer() {
    UpdatePlayerList();
    return g_local_player;
}

Camera* GetCamera() {
    UpdatePlayerList();
    return g_camera;
}

int GetPlayerCount() {
    UpdatePlayerList();
    if (!g_game_controller) return 0;
    return g_game_controller->playerCount;
}

Player* GetPlayer(int idx) {
    UpdatePlayerList();
    if (!g_game_controller) return nullptr;
    if (idx < 0 || idx >= g_game_controller->playerCount) return nullptr;
    Player** arr = (Player**)g_game_controller->players;
    return arr[idx];
}

Vec3 GetPlayerPosition(Player* player) {
    if (!player) return V3(0,0,0);
    return player->GetPosition();
}

Vec3 GetCameraForward() {
    if (!g_camera) return V3(0,0,1);
    static void* method = g_il2cpp.GetMethod("UnityEngine.CoreModule.dll", "UnityEngine", "Transform", "get_forward", 0);
    if (method && g_camera) {
        Transform* t = g_camera->GetTransform();
        if (t) return g_il2cpp.Invoke<Vec3>(method, t);
    }
    return V3(0,0,1);
}

Vec3 WorldToScreenPoint(Vec3 world) {
    if (!g_camera) return V3(0,0,0);
    return g_camera->WorldToScreenPoint(world);
}

float Distance3D(Vec3 a, Vec3 b) {
    F32 dx = a.x - b.x;
    F32 dy = a.y - b.y;
    F32 dz = a.z - b.z;
    return sqrtf(dx*dx + dy*dy + dz*dz);
}

Player* FindClosestTargetInFOV(float fovDeg) {
    if (!g_camera || !g_game_controller) return nullptr;
    Vec3 camPos = GetPlayerPosition(g_local_player);
    Vec3 camFwd = GetCameraForward();
    F32 halfFov = fovDeg * 0.5f * 3.1415926f / 180.0f;

    Player* best = nullptr;
    F32 bestDist = g_max_distance;

    int count = GetPlayerCount();
    for (int i = 0; i < count; i++) {
        Player* ply = GetPlayer(i);
        if (!ply) continue;
        if (ply == g_local_player) continue;
        if (!ply->isAlive) continue;

        Vec3 targetPos = ply->GetPosition();
        Vec3 toTarget = V3(targetPos.x - camPos.x, targetPos.y - camPos.y, targetPos.z - camPos.z);
        F32 dist = sqrtf(toTarget.x*toTarget.x + toTarget.y*toTarget.y + toTarget.z*toTarget.z);
        if (dist > bestDist) continue;

        F32 len = sqrtf(toTarget.x*toTarget.x + toTarget.y*toTarget.y + toTarget.z*toTarget.z);
        if (len < 0.001f) continue;
        toTarget.x /= len; toTarget.y /= len; toTarget.z /= len;

        F32 dot = camFwd.x*toTarget.x + camFwd.y*toTarget.y + camFwd.z*toTarget.z;
        if (dot < cosf(halfFov)) continue;

        best = ply;
        bestDist = dist;
    }
    return best;
}

Vec3 CalculateDirectionTo(Player* target) {
    if (!target || !g_camera) return V3(0,0,1);
    Vec3 camPos = GetPlayerPosition(g_local_player);
    Vec3 targetPos = target->GetPosition();

    Vec3 dir = V3(targetPos.x - camPos.x, targetPos.y - camPos.y, targetPos.z - camPos.z);
    F32 len = sqrtf(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
    if (len < 0.001f) return V3(0,0,1);
    dir.x /= len; dir.y /= len; dir.z /= len;
    return dir;
}

void ZeroSpread(WeaponParameters* wp) {
    if (!wp || !g_no_spread) return;
    wp->spread = 0.0f;
    wp->hipFireAccuracy = 1.0f;
    wp->movementAccuracy = 1.0f;
}

void ApplyNoRecoil(F32* amount) {
    if (!g_no_recoil) return;
    *amount = 0.0f;
}

bool IsPlayerGrounded(Player* player) {
    if (!player) return false;
    // Use CharacterController.isGrounded
    static void* method = g_il2cpp.GetMethod("UnityEngine.CoreModule.dll", "UnityEngine", "CharacterController", "get_isGrounded", 0);
    if (method) {
        void* cc = g_il2cpp.Invoke<void*>(method, player);
        return cc != nullptr;
    }
    return false;
}

void DetourFirePellets(WeaponParameters* weaponParams, Vec3 origin, Quat rotation) {
    if (g_silent_aim && weaponParams) {
        Player* target = FindClosestTargetInFOV(g_fov);
        if (target) {
            Vec3 dir = CalculateDirectionTo(target);
            if (g_no_spread) ZeroSpread(weaponParams);
            ((void(*)(WeaponParameters*, Vec3, Quat))g_real_firepellets)(weaponParams, dir, rotation);
            return;
        }
    }
    ((void(*)(WeaponParameters*, Vec3, Quat))g_real_firepellets)(weaponParams, origin, rotation);
}

void DetourSetRotation(Transform* transform, Quat value) {
    if (g_spin_enabled && transform) {
        static F32 spin_yaw = 0.0f;
        spin_yaw += g_spin_speed * 0.016f;
        if (spin_yaw > 360.0f) spin_yaw -= 360.0f;
        F32 rad = spin_yaw * 3.1415926f / 180.0f;
        F32 new_y = sinf(rad);
        F32 new_w = cosf(rad);
        if (g_spin_invert_x) value.x = -value.x;
        value.y = new_y;
        value.w = new_w;
        ((void(*)(Transform*, Quat))g_real_setrotation)(transform, value);
        return;
    }
    ((void(*)(Transform*, Quat))g_real_setrotation)(transform, value);
}

bool DetourGetButton(const char* button) {
    if (g_bunnyhop && strcmp(button, "Jump") == 0) {
        Player* local = GetLocalPlayer();
        if (local && IsPlayerGrounded(local)) return true;
    }
    return ((bool(*)(const char*))g_real_getbutton)(button);
}

void DetourSendChatMessage(void* message) {
    ((void(*)(void*))g_real_sendchat)(message);
}

void ChatSpamUpdate(F32 dt) {
    if (!g_chat_spam) return;
    g_spam_timer += dt;
    if (g_spam_timer >= g_spam_interval) {
        g_spam_timer = 0.0f;
        const char* msg = g_spam_msgs[g_spam_index];
        g_spam_index++;
        if (!g_spam_msgs[g_spam_index]) g_spam_index = 0;
        if (msg) ((void(*)(void*))g_real_sendchat)((void*)msg);
    }
}

const char* DetourGetDeviceUniqueIdentifier() {
    if (g_hwid_spoof) {
        static char buf[64];
        snprintf(buf, 64, "SPOOFED_%08X", rand());
        return buf;
    }
    return ((const char*(*)())g_real_hwid)();
}

void DetourWallHackDetector(void* callback, Vec3 spawnPos) {
    if (!g_bypass_actk) {
        ((void(*)(void*, Vec3))g_real_wallhack)(callback, spawnPos);
    }
}

void DetourShootRaycastModule() {
    if (!g_bypass_actk) {
        ((void(*)())g_real_shootraycast)();
    }
}

bool DetourRootDetect_Checkroot() {
    if (g_bypass_rootdetect) return false;
    return ((bool(*)())g_real_rootdetect_checkroot)();
}

void DetourApkHashChecker_ctor() {
    // Bypass
}

void DetourSessionGuard_Revoke(const char* reason) {
    if (g_bypass_actk) return;
    ((void(*)(const char*))g_real_sessionguard_revoke)(reason);
}

void DetourSessionGuard_Reset() {
    if (g_bypass_actk) return;
    ((void(*)())g_real_sessionguard_reset)();
}

void DetourScarletVersion_cctor() {
    if (g_bypass_version) return;
    ((void(*)())g_real_scarletversion_cctor)();
}

const char* DetourScarletVersion_GetVersion() {
    if (g_bypass_version) return "1.3.0.1";
    return ((const char*(*)())g_real_scarletversion_getversion)();
}

void DetourPublishLook(WeaponParameters* weaponParams, void* skin, void* stickers, int seed) {
    if (g_skin_changer_enabled && g_skin_update_players && weaponParams) {
        char customSkin[64];
        snprintf(customSkin, sizeof(customSkin), "skin_%d", g_skin_id);
        ((void(*)(WeaponParameters*, void*, void*, int))g_real_publishlook)(weaponParams, customSkin, stickers, g_skin_seed);
        return;
    }
    ((void(*)(WeaponParameters*, void*, void*, int))g_real_publishlook)(weaponParams, skin, stickers, seed);
}

void ApplySkinChanger(void* weapon, const char* skinName, void* stickers, int seed) {
    if (!g_skin_changer_enabled) return;
    WeaponParameters* wp = (WeaponParameters*)weapon;
    if (wp) {
        char customSkin[64];
        snprintf(customSkin, sizeof(customSkin), "skin_%d", g_skin_id);
        // Call PublishLook via IL2CPP
        void* method = g_il2cpp.GetMethod("Assembly-CSharp.dll", "", "WeaponSkin", "PublishLook", 4);
        if (method) g_il2cpp.Invoke<void>(method, wp, customSkin, stickers, seed);
    }
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
    g_config.spin_invert_x = g_config.spin_invert_x;
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

void InstallHooksViaResolver() {
    // Resolve methods by name using IL2CPP metadata
    void* firepellets = g_il2cpp.GetMethod("Assembly-CSharp.dll", "", "Weapon", "FirePellets", 3);
    void* setrotation = g_il2cpp.GetMethod("UnityEngine.CoreModule.dll", "UnityEngine", "Transform", "set_rotation", 1);
    void* getbutton = g_il2cpp.GetMethod("UnityEngine.CoreModule.dll", "UnityEngine", "Input", "GetButton", 1);
    void* sendchat = g_il2cpp.GetMethod("Assembly-CSharp.dll", "", "ChatManager", "SendChatMessage", 1);
    void* hwid = g_il2cpp.GetMethod("UnityEngine.CoreModule.dll", "UnityEngine", "SystemInfo", "get_deviceUniqueIdentifier", 0);
    void* wallhack = g_il2cpp.GetMethod("Assembly-CSharp.dll", "", "WallHackDetector", "StartDetection", 2);
    void* shootraycast = g_il2cpp.GetMethod("Assembly-CSharp.dll", "", "ShootRaycastModule", "Update", 0);
    void* rootdetect = g_il2cpp.GetMethod("Assembly-CSharp.dll", "", "RootDetect", "Checkroot", 0);
    void* apkhash = g_il2cpp.GetMethod("Assembly-CSharp.dll", "", "ApkHashChecker", ".ctor", 0);
    void* sessionrevoke = g_il2cpp.GetMethod("Assembly-CSharp.dll", "", "SessionGuard", "Revoke", 1);
    void* sessionreset = g_il2cpp.GetMethod("Assembly-CSharp.dll", "", "SessionGuard", "Reset", 0);
    void* scarletcctor = g_il2cpp.GetMethod("Assembly-CSharp.dll", "", "ScarletVersion", ".cctor", 0);
    void* scarletversion = g_il2cpp.GetMethod("Assembly-CSharp.dll", "", "ScarletVersion", "GetVersion", 0);
    void* publishlook = g_il2cpp.GetMethod("Assembly-CSharp.dll", "", "WeaponSkin", "PublishLook", 4);

    if (firepellets) HookManager::Hook(firepellets, (void*)DetourFirePellets, &g_real_firepellets);
    if (setrotation) HookManager::Hook(setrotation, (void*)DetourSetRotation, &g_real_setrotation);
    if (getbutton) HookManager::Hook(getbutton, (void*)DetourGetButton, &g_real_getbutton);
    if (sendchat) HookManager::Hook(sendchat, (void*)DetourSendChatMessage, &g_real_sendchat);
    if (hwid) HookManager::Hook(hwid, (void*)DetourGetDeviceUniqueIdentifier, &g_real_hwid);
    if (wallhack) HookManager::Hook(wallhack, (void*)DetourWallHackDetector, &g_real_wallhack);
    if (shootraycast) HookManager::Hook(shootraycast, (void*)DetourShootRaycastModule, &g_real_shootraycast);
    if (rootdetect) HookManager::Hook(rootdetect, (void*)DetourRootDetect_Checkroot, &g_real_rootdetect_checkroot);
    if (apkhash) HookManager::Hook(apkhash, (void*)DetourApkHashChecker_ctor, &g_real_apkhash_ctor);
    if (sessionrevoke) HookManager::Hook(sessionrevoke, (void*)DetourSessionGuard_Revoke, &g_real_sessionguard_revoke);
    if (sessionreset) HookManager::Hook(sessionreset, (void*)DetourSessionGuard_Reset, &g_real_sessionguard_reset);
    if (scarletcctor) HookManager::Hook(scarletcctor, (void*)DetourScarletVersion_cctor, &g_real_scarletversion_cctor);
    if (scarletversion) HookManager::Hook(scarletversion, (void*)DetourScarletVersion_GetVersion, &g_real_scarletversion_getversion);
    if (publishlook) HookManager::Hook(publishlook, (void*)DetourPublishLook, &g_real_publishlook);
}

void InstallAllHooks(UPtr base) {
    g_base = base;

    // Initialize IL2CPP resolver with UnityFramework handle
    void* unity_framework = dlopen("UnityFramework", RTLD_LAZY);
    if (!unity_framework) {
        // Try to find it via base address
        unity_framework = (void*)base;
    }

    if (g_il2cpp.Initialize(unity_framework)) {
        printf("[IL2CPP] Resolver initialized successfully\n");
        InstallHooksViaResolver();
    } else {
        printf("[IL2CPP] Failed to initialize resolver, falling back to RVAs\n");
        // Fallback to RVA-based hooks
        srand((U32)time(0));
        // ... existing RVA-based hook installation
    }

    Config_Load();
    UpdatePlayerList();
}

extern "C" void __attribute__((constructor)) CounterFlameCheatInit() {
    g_base = 0x100000000;
    InstallAllHooks(g_base);
    MenuInit();
}