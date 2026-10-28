#include "cheat.h"
#include <sys/mman.h>
#include <unistd.h>
#include <dlfcn.h>
#include "dobby.h"

extern "C" void MenuInit();

UPtr g_base = 0;

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

bool  g_menu_open = false;

CheatConfig g_config = {};

static const char* g_spam_msgs[] = {
    "CounterFlame Internal",
    "Чит активирован",
    "Ваш аккаунт виден",
    "ESP и сайлент аим включены",
    "Антибан активен",
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
}

static void* FindPlayerList() {
    void* go = ((GameObject_Find_t)(g_base + 0x2BAB388))((void*)"GameController");
    if (!go) return 0;
    void* comp = ((GetComponent_t)(g_base + 0x2BAA268))(go, 0);
    if (!comp) return 0;
    return comp;
}

void UpdatePlayerList() {
    if (!g_player_list) {
        g_player_list = FindPlayerList();
    }
    if (!g_player_list) return;
    GameController* gc = (GameController*)g_player_list;
    g_local_player = (void*)gc->localPlayer;
    g_camera = (void*)gc->camera;
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
    GameController* gc = (GameController*)g_player_list;
    return gc->playerCount;
}

void* GetPlayer(int idx) {
    if (!g_player_list) return 0;
    GameController* gc = (GameController*)g_player_list;
    UPtr* arr = (UPtr*)gc->players;
    if (idx < 0 || idx >= gc->playerCount) return 0;
    return (void*)arr[idx];
}

Vec3 GetPlayerPosition(void* player) {
    Transform* tr = (Transform*)((Player*)player)->transform;
    return ((Transform_GetPosition_t)(g_base + 0x2BB4894))(tr);
}

Vec3 GetCameraForward() {
    if (!g_camera) return V3(0,0,1);
    return ((Transform_GetForward_t)(g_base + 0x2BA2D4C))(g_camera);
}

Vec3 WorldToScreenPoint(Vec3 world) {
    if (!g_camera) return V3(0,0,0);
    return ((Camera_WorldToScreen_t)(g_base + 0x2B73E5C))(g_camera, world);
}

float Distance3D(Vec3 a, Vec3 b) {
    F32 dx = a.x - b.x;
    F32 dy = a.y - b.y;
    F32 dz = a.z - b.z;
    return sqrtf(dx*dx + dy*dy + dz*dz);
}

void* FindClosestTargetInFOV(float fovDeg) {
    if (!g_camera || !g_player_list) return 0;
    Camera* cam = (Camera*)g_camera;
    Vec3 camPos = GetPlayerPosition(g_local_player);
    Vec3 camFwd = GetCameraForward();
    F32 halfFov = fovDeg * 0.5f * 3.1415926f / 180.0f;

    void* best = 0;
    F32 bestDist = g_max_distance;

    int count = GetPlayerCount();
    for (int i = 0; i < count; i++) {
        void* ply = GetPlayer(i);
        if (!ply) continue;
        Player* p = (Player*)ply;
        if (p == g_local_player) continue;
        if (!p->isAlive) continue;

        Vec3 targetPos = GetPlayerPosition(p);
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
    Player* p = (Player*)target;
    Vec3 targetPos = GetPlayerPosition(p);

    Vec3 dir = V3(targetPos.x - camPos.x, targetPos.y - camPos.y, targetPos.z - camPos.z);
    F32 len = sqrtf(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
    if (len < 0.001f) return V3(0,0,1);
    dir.x /= len; dir.y /= len; dir.z /= len;
    return dir;
}

void ZeroSpread(void* weaponParams) {
    if (!weaponParams || !g_no_spread) return;
    WeaponParameters* wp = (WeaponParameters*)weaponParams;
    wp->spread = 0.0f;
    wp->hipFireAccuracy = 1.0f;
    wp->movementAccuracy = 1.0f;
    wp->jumpAccuracy = 1.0f;
    wp->crouchAccuracy = 1.0f;
    wp->proneAccuracy = 1.0f;
}

void ApplyNoRecoil(F32* amount) {
    if (!g_no_recoil) return;
    *amount = 0.0f;
}

bool IsPlayerGrounded(void* controller) {
    if (!controller) return false;
    return ((CharController_IsGrounded_t)(g_base + 0x2BFCC04))(controller);
}

void DetourFirePellets(void* weaponParams, Vec3 origin, Quat rotation) {
    if (g_silent_aim) {
        void* target = FindClosestTargetInFOV(g_fov);
        if (target) {
            Vec3 dir = CalculateDirectionTo(target);
            WeaponParameters* wp = (WeaponParameters*)weaponParams;
            if (g_no_spread) ZeroSpread(wp);
            ((FirePellets_t)g_real_firepellets)(weaponParams, dir, rotation);
            return;
        }
    }
    ((FirePellets_t)g_real_firepellets)(weaponParams, origin, rotation);
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
        ((Transform_SetRotation_t)g_real_setrotation)(transform, value);
        return;
    }
    ((Transform_SetRotation_t)g_real_setrotation)(transform, value);
}

bool DetourGetButton(const char* button) {
    if (g_bunnyhop && strcmp(button, "Jump") == 0) {
        void* local = GetLocalPlayer();
        if (local) {
            Transform* tr = (Transform*)((Player*)local)->transform;
            if (IsPlayerGrounded(tr)) return true;
        }
    }
    return ((Input_GetButton_t)g_real_getbutton)((void*)button);
}

void DetourSendChatMessage(void* message) {
    ((SendChat_t)g_real_sendchat)(message);
}

void ChatSpamUpdate(F32 dt) {
    if (!g_chat_spam) return;
    g_spam_timer += dt;
    if (g_spam_timer >= g_spam_interval) {
        g_spam_timer = 0.0f;
        const char* msg = g_spam_msgs[g_spam_index];
        g_spam_index++;
        if (!g_spam_msgs[g_spam_index]) g_spam_index = 0;
        if (msg) ((SendChat_t)g_real_sendchat)((void*)msg);
    }
}

const char* DetourGetDeviceUniqueIdentifier() {
    if (g_hwid_spoof) {
        static char buf[64];
        snprintf(buf, 64, "SPOOFED_%08X", rand());
        return buf;
    }
    return (const char*)((GetDeviceUID_t)g_real_hwid)();
}

void DetourWallHackDetector(void* callback, Vec3 spawnPos) {
    if (!g_bypass_actk) {
        ((WallHackDetector_t)g_real_wallhack)(callback, spawnPos);
    }
    // Если обход включен — просто не вызываем оригинал (NOP)
}

void DetourShootRaycastModule() {
    if (!g_bypass_actk) {
        ((ShootRaycastModule_t)g_real_shootraycast)();
    }
}

// --- АНТИ-БАН: RootDetect.Checkroot — всегда возвращаем false (не обнаружен) ---
bool DetourRootDetect_Checkroot() {
    if (g_bypass_rootdetect) {
        return false;  // Античит не обнаруживает джейлбрейк/рут
    }
    return ((RootDetect_Checkroot_t)g_real_rootdetect_checkroot)();
}

// --- АНТИ-БАН: ApkHashChecker.ctor — пустой конструктор (не проверяем хеш клиента) ---
void DetourApkHashChecker_ctor() {
    // Ничего не делаем — обходим проверку хеша APK
}

// --- АНТИ-БАН: SessionGuard.Revoke — не отзываем сессию ---
void DetourSessionGuard_Revoke(const char* reason) {
    if (g_bypass_actk) {
        // Игнорируем отзыв сессии
        return;
    }
    ((SessionGuard_Revoke_t)g_real_sessionguard_revoke)(reason);
}

// --- АНТИ-БАН: SessionGuard.Reset — не сбрасываем сессию ---
void DetourSessionGuard_Reset() {
    if (g_bypass_actk) {
        return;
    }
    ((SessionGuard_Reset_t)g_real_sessionguard_reset)();
}

// --- АНТИ-БАН: ScarletVersion.cctor — не инициализируем проверку версии ---
void DetourScarletVersion_cctor() {
    if (g_bypass_version) {
        return;  // Не запускаем проверку версии игры
    }
    ((ScarletVersion_cctor_t)g_real_scarletversion_cctor)();
}

// --- АНТИ-БАН: ScarletVersion.GetVersion — возвращаем актуальную версию ---
const char* DetourScarletVersion_GetVersion() {
    if (g_bypass_version) {
        return "1.3.0.1";  // Возвращаем актуальную версию, чтобы сервер не блокировал
    }
    return ((ScarletVersion_GetVersion_t)g_real_scarletversion_getversion)();
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
}

void Config_Save() {
    g_config.silent_aim = g_silent_aim;
    g_config.fov = g_fov;
    g_config.target_bone = g_target_bone;
    g_config.max_distance = g_config.max_distance;
    g_config.predictive_aim = g_config.predictive_aim;
    g_config.prediction_ms = g_config.prediction_ms;
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

    FILE* f = fopen("/var/mobile/Containers/Data/Application/CounterFlame/config.cfg", "wb");
    if (!f) return;
    fwrite(&g_config, sizeof(CheatConfig), 1, f);
    fclose(f);
}

void InstallAllHooks(UPtr base) {
    g_base = base;
    Config_Load();
    srand((U32)time(0));

    UPtr addr;

    /* --- Читы --- */
    FindOffsetByName("FirePellets", &addr);
    g_real_firepellets = (void*)addr;
    if (DobbyHook((void*)addr, (void*)DetourFirePellets, (void**)&g_real_firepellets) != 0) {
        printf("[!] DobbyHook failed: FirePellets\n");
    }

    FindOffsetByName("Transform_set_rotation", &addr);
    g_real_setrotation = (void*)addr;
    if (DobbyHook((void*)addr, (void*)DetourSetRotation, (void**)&g_real_setrotation) != 0) {
        printf("[!] DobbyHook failed: Transform_set_rotation\n");
    }

    FindOffsetByName("Input_GetButton", &addr);
    g_real_getbutton = (void*)addr;
    if (DobbyHook((void*)addr, (void*)DetourGetButton, (void**)&g_real_getbutton) != 0) {
        printf("[!] DobbyHook failed: Input_GetButton\n");
    }

    FindOffsetByName("SendChatMessage", &addr);
    g_real_sendchat = (void*)addr;
    if (DobbyHook((void*)addr, (void*)DetourSendChatMessage, (void**)&g_real_sendchat) != 0) {
        printf("[!] DobbyHook failed: SendChatMessage\n");
    }

    FindOffsetByName("SystemInfo_deviceUniqueIdentifier", &addr);
    g_real_hwid = (void*)addr;
    if (DobbyHook((void*)addr, (void*)DetourGetDeviceUniqueIdentifier, (void**)&g_real_hwid) != 0) {
        printf("[!] DobbyHook failed: SystemInfo_deviceUniqueIdentifier\n");
    }

    /* --- Античит ACTk --- */
    FindOffsetByName("WallHackDetector_StartDetection", &addr);
    g_real_wallhack = (void*)addr;
    if (DobbyHook((void*)addr, (void*)DetourWallHackDetector, (void**)&g_real_wallhack) != 0) {
        printf("[!] DobbyHook failed: WallHackDetector_StartDetection\n");
    }

    FindOffsetByName("ShootRaycastModule", &addr);
    g_real_shootraycast = (void*)addr;
    if (DobbyHook((void*)addr, (void*)DetourShootRaycastModule, (void**)&g_real_shootraycast) != 0) {
        printf("[!] DobbyHook failed: ShootRaycastModule\n");
    }

    /* --- Анти-БАН --- */
    FindOffsetByName("RootDetect_Checkroot", &addr);
    g_real_rootdetect_checkroot = (void*)addr;
    if (DobbyHook((void*)addr, (void*)DetourRootDetect_Checkroot, (void**)&g_real_rootdetect_checkroot) != 0) {
        printf("[!] DobbyHook failed: RootDetect_Checkroot\n");
    }

    FindOffsetByName("ApkHashChecker_ctor", &addr);
    g_real_apkhash_ctor = (void*)addr;
    if (DobbyHook((void*)addr, (void*)DetourApkHashChecker_ctor, (void**)&g_real_apkhash_ctor) != 0) {
        printf("[!] DobbyHook failed: ApkHashChecker_ctor\n");
    }

    FindOffsetByName("SessionGuard_Revoke", &addr);
    g_real_sessionguard_revoke = (void*)addr;
    if (DobbyHook((void*)addr, (void*)DetourSessionGuard_Revoke, (void**)&g_real_sessionguard_revoke) != 0) {
        printf("[!] DobbyHook failed: SessionGuard_Revoke\n");
    }

    FindOffsetByName("SessionGuard_Reset", &addr);
    g_real_sessionguard_reset = (void*)addr;
    if (DobbyHook((void*)addr, (void*)DetourSessionGuard_Reset, (void**)&g_real_sessionguard_reset) != 0) {
        printf("[!] DobbyHook failed: SessionGuard_Reset\n");
    }

    FindOffsetByName("ScarletVersion_cctor", &addr);
    g_real_scarletversion_cctor = (void*)addr;
    if (DobbyHook((void*)addr, (void*)DetourScarletVersion_cctor, (void**)&g_real_scarletversion_cctor) != 0) {
        printf("[!] DobbyHook failed: ScarletVersion_cctor\n");
    }

    FindOffsetByName("ScarletVersion_GetVersion", &addr);
    g_real_scarletversion_getversion = (void*)addr;
    if (DobbyHook((void*)addr, (void*)DetourScarletVersion_GetVersion, (void**)&g_real_scarletversion_getversion) != 0) {
        printf("[!] DobbyHook failed: ScarletVersion_GetVersion\n");
    }

    UpdatePlayerList();
}

extern "C" void __attribute__((constructor)) CounterFlameCheatInit() {
    g_base = 0x100000000;  // Временно, пока не найден реальный базовый адрес
    InstallAllHooks(g_base);
    MenuInit();
}
