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

void* g_player_list = 0;
int   g_player_count = 0;
void* g_local_player = 0;
void* g_camera = 0;

static void FindOffsetByName(const char* name, UPtr* out) {
    for (int i = 0; g_offsets[i].name; i++) {
        if (strcmp(g_offsets[i].name, name) == 0) {
            *out = g_base + g_offsets[i].rva;
            return;
        }
    }
    *out = 0;
}

static void* FindPlayerList() {
    void* go = ((void*(*)(void*))(g_base + 0x2BAB388))((void*)"GameController");
    if (!go) return 0;
    void* comp = ((void*(*)(void*, void*))(g_base + 0x2BAA268))(go, 0);
    if (!comp) return 0;
    return comp;
}

void UpdatePlayerList() {
    if (!g_player_list) {
        g_player_list = FindPlayerList();
    }
    if (!g_player_list) return;
    // GameController structure
    void** arr = (void**)((uintptr_t)g_player_list + 0x18); // players array
    g_local_player = (void*)((uintptr_t)g_player_list + 0x20); // localPlayer
    g_camera = (void*)((uintptr_t)g_player_list + 0x28); // camera
}

void* GetLocalPlayer() {
    UpdatePlayerList();
    return g_local_player;
}

void* GetCamera() {
    UpdatePlayerList();
    return g_camera;
}

int GetPlayerCount() {
    if (!g_player_list) return 0;
    return *(int*)((uintptr_t)g_player_list + 0x10); // playerCount
}

void* GetPlayer(int idx) {
    if (!g_player_list) return 0;
    void** arr = (void**)((uintptr_t)g_player_list + 0x18); // players array
    if (idx < 0 || idx >= *(int*)((uintptr_t)g_player_list + 0x10)) return 0;
    return arr[idx];
}

Vec3 GetPlayerPosition(void* player) {
    if (!player) return V3(0,0,0);
    return ((Vec3(*)(void*))(g_base + 0x2BB4894))((void*)((uintptr_t)player + 0x18)); // transform -> get_position
}

Vec3 GetCameraForward() {
    if (!g_camera) return V3(0,0,1);
    return ((Vec3(*)(void*))(g_base + 0x2BB4DE4))((void*)((uintptr_t)g_camera + 0x18)); // transform -> get_forward
}

Vec3 WorldToScreenPoint(Vec3 world) {
    if (!g_camera) return V3(0,0,0);
    return ((Vec3(*)(void*, Vec3))(g_base + 0x2B73E5C))(g_camera, world);
}

float Distance3D(Vec3 a, Vec3 b) {
    F32 dx = a.x - b.x;
    F32 dy = a.y - b.y;
    F32 dz = a.z - b.z;
    return sqrtf(dx*dx + dy*dy + dz*dz);
}

void* FindClosestTargetInFOV(float fovDeg) {
    if (!g_camera || !g_player_list) return 0;
    Vec3 camPos = GetPlayerPosition(g_local_player);
    Vec3 camFwd = GetCameraForward();
    F32 halfFov = fovDeg * 0.5f * 3.1415926f / 180.0f;

    void* best = 0;
    F32 bestDist = g_max_distance;

    int count = GetPlayerCount();
    for (int i = 0; i < count; i++) {
        void* ply = GetPlayer(i);
        if (!ply) continue;
        if (ply == g_local_player) continue;
        // Check isAlive
        bool isAlive = *(bool*)((uintptr_t)ply + 0x30);
        if (!isAlive) continue;

        Vec3 targetPos = GetPlayerPosition(ply);
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

Vec3 CalculateDirectionTo(void* target) {
    if (!target || !g_camera) return V3(0,0,1);
    Vec3 camPos = GetPlayerPosition(g_local_player);
    Vec3 targetPos = GetPlayerPosition(target);

    Vec3 dir = V3(targetPos.x - camPos.x, targetPos.y - camPos.y, targetPos.z - camPos.z);
    F32 len = sqrtf(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
    if (len < 0.001f) return V3(0,0,1);
    dir.x /= len; dir.y /= len; dir.z /= len;
    return dir;
}

void ZeroSpread(void* weaponParams) {
    if (!weaponParams || !g_no_spread) return;
    // WeaponParameters structure: spread at offset 0x50
    *(float*)((uintptr_t)weaponParams + 0x50) = 0.0f;
    *(float*)((uintptr_t)weaponParams + 0x5C) = 1.0f; // hipFireAccuracy
    *(float*)((uintptr_t)weaponParams + 0x60) = 1.0f; // movementAccuracy
}

void ApplyNoRecoil(float* amount) {
    if (!g_no_recoil) return;
    *amount = 0.0f;
}

bool IsPlayerGrounded(void* controller) {
    if (!controller) return false;
    return ((bool(*)(void*))(g_base + 0x2BFCC04))(controller);
}

void DetourFirePellets(void* weaponParams, Vec3 origin, Quat rotation) {
    if (g_silent_aim && weaponParams) {
        void* target = FindClosestTargetInFOV(g_fov);
        if (target) {
            Vec3 dir = CalculateDirectionTo(target);
            if (g_no_spread) ZeroSpread(weaponParams);
            ((void(*)(void*, Vec3, Quat))g_real_firepellets)(weaponParams, dir, rotation);
            return;
        }
    }
    ((void(*)(void*, Vec3, Quat))g_real_firepellets)(weaponParams, origin, rotation);
}

void DetourSetRotation(void* transform, Quat value) {
    if (g_spin_enabled) {
        static F32 spin_yaw = 0.0f;
        spin_yaw += g_spin_speed * 0.016f;
        if (spin_yaw > 360.0f) spin_yaw -= 360.0f;
        F32 rad = spin_yaw * 3.1415926f / 180.0f;
        F32 new_y = sinf(rad);
        F32 new_w = cosf(rad);
        if (g_spin_invert_x) value.x = -value.x;
        value.y = new_y;
        value.w = new_w;
        ((void(*)(void*, Quat))g_real_setrotation)(transform, value);
        return;
    }
    ((void(*)(void*, Quat))g_real_setrotation)(transform, value);
}

bool DetourGetButton(const char* button) {
    if (g_bunnyhop && strcmp(button, "Jump") == 0) {
        void* local = GetLocalPlayer();
        if (local) {
            void* transform = (void*)((uintptr_t)local + 0x18);
            if (IsPlayerGrounded(transform)) return true;
        }
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
    // Bypass - do nothing
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

void DetourPublishLook(void* weaponParams, void* skin, void* stickers, int seed) {
    if (g_skin_changer_enabled && g_skin_update_players && weaponParams) {
        char customSkin[64];
        snprintf(customSkin, sizeof(customSkin), "skin_%d", g_skin_id);
        ((void(*)(void*, void*, void*, int))g_real_publishlook)(weaponParams, customSkin, stickers, g_skin_seed);
        return;
    }
    ((void(*)(void*, void*, void*, int))g_real_publishlook)(weaponParams, skin, stickers, seed);
}

void ApplySkinChanger(void* weapon, const char* skinName, void* stickers, int seed) {
    if (!g_skin_changer_enabled) return;
    // Hook handled by DetourPublishLook
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

void InstallAllHooks(UPtr base) {
    g_base = base;
    Config_Load();
    srand((U32)time(0));

    UPtr addr = 0;

    // FirePellets
    FindOffsetByName("FirePellets", &addr);
    if (addr) {
        g_real_firepellets = (void*)addr;
        if (DobbyHook((void*)addr, (void*)DetourFirePellets, (void**)&g_real_firepellets) != 0) {
            printf("[!] DobbyHook failed: FirePellets at 0x%lx\n", addr);
        } else {
            printf("[+] Hooked FirePellets at 0x%lx\n", addr);
        }
    }

    // Transform_set_rotation
    FindOffsetByName("Transform_set_rotation", &addr);
    if (addr) {
        g_real_setrotation = (void*)addr;
        if (DobbyHook((void*)addr, (void*)DetourSetRotation, (void**)&g_real_setrotation) != 0) {
            printf("[!] DobbyHook failed: Transform_set_rotation at 0x%lx\n", addr);
        } else {
            printf("[+] Hooked Transform_set_rotation at 0x%lx\n", addr);
        }
    }

    // Input_GetButton
    FindOffsetByName("Input_GetButton", &addr);
    if (addr) {
        g_real_getbutton = (void*)addr;
        if (DobbyHook((void*)addr, (void*)DetourGetButton, (void**)&g_real_getbutton) != 0) {
            printf("[!] DobbyHook failed: Input_GetButton at 0x%lx\n", addr);
        } else {
            printf("[+] Hooked Input_GetButton at 0x%lx\n", addr);
        }
    }

    // SendChatMessage
    FindOffsetByName("SendChatMessage", &addr);
    if (addr) {
        g_real_sendchat = (void*)addr;
        if (DobbyHook((void*)addr, (void*)DetourSendChatMessage, (void**)&g_real_sendchat) != 0) {
            printf("[!] DobbyHook failed: SendChatMessage at 0x%lx\n", addr);
        } else {
            printf("[+] Hooked SendChatMessage at 0x%lx\n", addr);
        }
    }

    // SystemInfo_deviceUniqueIdentifier
    FindOffsetByName("SystemInfo_deviceUniqueIdentifier", &addr);
    if (addr) {
        g_real_hwid = (void*)addr;
        if (DobbyHook((void*)addr, (void*)DetourGetDeviceUniqueIdentifier, (void**)&g_real_hwid) != 0) {
            printf("[!] DobbyHook failed: SystemInfo_deviceUniqueIdentifier at 0x%lx\n", addr);
        } else {
            printf("[+] Hooked SystemInfo_deviceUniqueIdentifier at 0x%lx\n", addr);
        }
    }

    // WallHackDetector_StartDetection
    FindOffsetByName("WallHackDetector_StartDetection", &addr);
    if (addr) {
        g_real_wallhack = (void*)addr;
        if (DobbyHook((void*)addr, (void*)DetourWallHackDetector, (void**)&g_real_wallhack) != 0) {
            printf("[!] DobbyHook failed: WallHackDetector_StartDetection at 0x%lx\n", addr);
        } else {
            printf("[+] Hooked WallHackDetector_StartDetection at 0x%lx\n", addr);
        }
    }

    // ShootRaycastModule
    FindOffsetByName("ShootRaycastModule", &addr);
    if (addr) {
        g_real_shootraycast = (void*)addr;
        if (DobbyHook((void*)addr, (void*)DetourShootRaycastModule, (void**)&g_real_shootraycast) != 0) {
            printf("[!] DobbyHook failed: ShootRaycastModule at 0x%lx\n", addr);
        } else {
            printf("[+] Hooked ShootRaycastModule at 0x%lx\n", addr);
        }
    }

    // RootDetect_Checkroot
    FindOffsetByName("RootDetect_Checkroot", &addr);
    if (addr) {
        g_real_rootdetect_checkroot = (void*)addr;
        if (DobbyHook((void*)addr, (void*)DetourRootDetect_Checkroot, (void**)&g_real_rootdetect_checkroot) != 0) {
            printf("[!] DobbyHook failed: RootDetect_Checkroot at 0x%lx\n", addr);
        } else {
            printf("[+] Hooked RootDetect_Checkroot at 0x%lx\n", addr);
        }
    }

    // ApkHashChecker_ctor
    FindOffsetByName("ApkHashChecker_ctor", &addr);
    if (addr) {
        g_real_apkhash_ctor = (void*)addr;
        if (DobbyHook((void*)addr, (void*)DetourApkHashChecker_ctor, (void**)&g_real_apkhash_ctor) != 0) {
            printf("[!] DobbyHook failed: ApkHashChecker_ctor at 0x%lx\n", addr);
        } else {
            printf("[+] Hooked ApkHashChecker_ctor at 0x%lx\n", addr);
        }
    }

    // SessionGuard_Revoke
    FindOffsetByName("SessionGuard_Revoke", &addr);
    if (addr) {
        g_real_sessionguard_revoke = (void*)addr;
        if (DobbyHook((void*)addr, (void*)DetourSessionGuard_Revoke, (void**)&g_real_sessionguard_revoke) != 0) {
            printf("[!] DobbyHook failed: SessionGuard_Revoke at 0x%lx\n", addr);
        } else {
            printf("[+] Hooked SessionGuard_Revoke at 0x%lx\n", addr);
        }
    }

    // SessionGuard_Reset
    FindOffsetByName("SessionGuard_Reset", &addr);
    if (addr) {
        g_real_sessionguard_reset = (void*)addr;
        if (DobbyHook((void*)addr, (void*)DetourSessionGuard_Reset, (void**)&g_real_sessionguard_reset) != 0) {
            printf("[!] DobbyHook failed: SessionGuard_Reset at 0x%lx\n", addr);
        } else {
            printf("[+] Hooked SessionGuard_Reset at 0x%lx\n", addr);
        }
    }

    // ScarletVersion_cctor
    FindOffsetByName("ScarletVersion_cctor", &addr);
    if (addr) {
        g_real_scarletversion_cctor = (void*)addr;
        if (DobbyHook((void*)addr, (void*)DetourScarletVersion_cctor, (void**)&g_real_scarletversion_cctor) != 0) {
            printf("[!] DobbyHook failed: ScarletVersion_cctor at 0x%lx\n", addr);
        } else {
            printf("[+] Hooked ScarletVersion_cctor at 0x%lx\n", addr);
        }
    }

    // ScarletVersion_GetVersion
    FindOffsetByName("ScarletVersion_GetVersion", &addr);
    if (addr) {
        g_real_scarletversion_getversion = (void*)addr;
        if (DobbyHook((void*)addr, (void*)DetourScarletVersion_GetVersion, (void**)&g_real_scarletversion_getversion) != 0) {
            printf("[!] DobbyHook failed: ScarletVersion_GetVersion at 0x%lx\n", addr);
        } else {
            printf("[+] Hooked ScarletVersion_GetVersion at 0x%lx\n", addr);
        }
    }

    // PublishLook
    FindOffsetByName("PublishLook", &addr);
    if (addr) {
        g_real_publishlook = (void*)addr;
        if (DobbyHook((void*)addr, (void*)DetourPublishLook, (void**)&g_real_publishlook) != 0) {
            printf("[!] DobbyHook failed: PublishLook at 0x%lx\n", addr);
        } else {
            printf("[+] Hooked PublishLook at 0x%lx\n", addr);
        }
    }

    UpdatePlayerList();
    printf("[+] All hooks installed successfully\n");
}

extern "C" void __attribute__((constructor)) CounterFlameCheatInit() {
    g_base = 0x100000000;
    InstallAllHooks(g_base);
    MenuInit();
}