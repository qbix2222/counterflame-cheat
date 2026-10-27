#include "cheat.h"
#include <sys/mman.h>
#include <unistd.h>
#include <dlfcn.h>

UPtr g_base = 0;

bool g_silent_aim = false;
float g_fov = 25.0f;
int g_target_bone = 0;
float g_max_distance = 150.0f;
bool g_predictive_aim = false;
int g_prediction_ms = 100;

bool g_spin_enabled = false;
float g_spin_speed = 180.0f;
int g_spin_mode = 1;
bool g_spin_invert_x = false;

bool g_esp_boxes = true;
bool g_esp_names = true;
bool g_esp_distance = true;
bool g_chams_enabled = false;
float g_chams_r = 1.0f;
float g_chams_g = 0.0f;
float g_chams_b = 0.0f;

bool g_no_recoil = true;
bool g_no_spread = true;

bool g_bunnyhop = false;

bool g_chat_spam = false;
float g_spam_interval = 5.0f;
int g_spam_index = 0;
float g_spam_timer = 0.0f;

bool g_hwid_spoof = false;
bool g_bypass_actk = true;

bool g_menu_open = false;

CheatConfig g_config = {};

static const char* g_spam_msgs[] = {
    "CounterFlame Internal",
    "Чит активирован",
    "Ваш аккаунт виден",
    "ESP и сайлент аим включены",
    "Антибан активен",
    NULL
};

static void* g_trampoline_firepellets = 0;
static void* g_trampoline_setrotation = 0;
static void* g_trampoline_getbutton = 0;
static void* g_trampoline_sendchat = 0;
static void* g_trampoline_hwid = 0;
static void* g_trampoline_wallhack = 0;
static void* g_trampoline_shootraycast = 0;

static void* g_real_firepellets = 0;
static void* g_real_setrotation = 0;
static void* g_real_getbutton = 0;
static void* g_real_sendchat = 0;
static void* g_real_hwid = 0;
static void* g_real_wallhack = 0;
static void* g_real_shootraycast = 0;

static void* g_player_list = 0;
static int g_player_count = 0;
static void* g_local_player = 0;
static void* g_camera = 0;

static void FindOffsetByName(const char* name, UPtr* out) {
    for (int i = 0; g_offsets[i].name; i++) {
        if (strcmp(g_offsets[i].name, name) == 0) {
            *out = g_base + g_offsets[i].rva;
            return;
        }
    }
}

static void PatchBytes(void* addr, const U8* data, U32 size) {
    U32 old;
    if (mprotect((void*)((UPtr)addr & ~0xFFF), 4096, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) return;
    memcpy(addr, data, size);
    __builtin___clear_cache((char*)addr, (char*)addr + size);
    mprotect((void*)((UPtr)addr & ~0xFFF), 4096, PROT_READ | PROT_EXEC);
}

static U32 Arm64Branch(UPtr from, UPtr to) {
    S64 off = (S64)(to - from);
    if (off % 4 != 0) return 0;
    U32 imm26 = (U32)((off >> 2) & 0x03FFFFFF);
    return 0x08000000 | imm26;
}

static void InstallHook(void* target, void* detour, void** trampoline_out, void** real_out) {
    U32 orig[4];
    memcpy(orig, target, 16);

    void* tramp = mmap(0, 4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, -1, 0);
    U8* t = (U8*)tramp;
    memcpy(t, orig, 16);
    t += 16;
    U32 br = Arm64Branch((UPtr)t, (UPtr)target + 16);
    memcpy(t, &br, 4);

    mprotect(tramp, 4096, PROT_READ | PROT_EXEC);

    mprotect(target, 16, PROT_READ | PROT_WRITE | PROT_EXEC);
    U32 b = Arm64Branch((UPtr)target, (UPtr)detour);
    memcpy(target, &b, 4);
    for (U32 i = 4; i < 16; i += 4) {
        U32 nop = 0xD280001F;
        memcpy((U8*)target + i, &nop, 4);
    }
    mprotect(target, 16, PROT_READ | PROT_EXEC);

    __builtin___clear_cache((char*)target, (char*)target + 16);
    __builtin___clear_cache((char*)tramp, (char*)tramp + 4096);

    if (trampoline_out) *trampoline_out = tramp;
    if (real_out) *real_out = target;
}

static void* FindPlayerList() {
    void* go = ((Find_t)(g_base + 0x2BAB388))("GameController");
    if (!go) return 0;
    void* comp = ((GetComponent_t)(g_base + 0x2BAA268))(go, 0);
    if (!comp) return 0;
    return comp;
}

static void UpdatePlayerList() {
    if (!g_player_list) {
        g_player_list = FindPlayerList();
    }
    if (!g_player_list) return;
    GameController* gc = (GameController*)g_player_list;
    g_local_player = gc->localPlayer;
    g_camera = gc->camera;
}

static void* GetLocalPlayer() {
    UpdatePlayerList();
    return g_local_player;
}

static void* GetCamera() {
    UpdatePlayerList();
    return g_camera;
}

static int GetPlayerCount() {
    if (!g_player_list) return 0;
    GameController* gc = (GameController*)g_player_list;
    return gc->playerCount;
}

static void* GetPlayer(int idx) {
    if (!g_player_list) return 0;
    GameController* gc = (GameController*)g_player_list;
    UPtr* arr = (UPtr*)gc->players;
    if (idx < 0 || idx >= gc->playerCount) return 0;
    return (void*)arr[idx];
}

static Vec3 GetPlayerPosition(void* player) {
    Transform* tr = (Transform*)((Player*)player)->transform;
    Vec3 pos = ((GetPosition_t)(g_base + 0x2BB4894))(tr);
    return pos;
}

static Vec3 GetCameraForward() {
    if (!g_camera) return Vec3{0,0,1};
    Transform* tr = (Transform*)((Camera*)g_camera)->transform;
    Vec3 fwd = ((GetForward_t)(g_base + 0x2BA2D4C))(tr);
    return fwd;
}

static Vec3 WorldToScreenPoint(Vec3 world) {
    if (!g_camera) return Vec3{0,0,0};
    return ((Vec3 (*)(void*, Vec3, int))(g_base + 0x2B73E5C))(g_camera, world, 0);
}

static float Distance3D(Vec3 a, Vec3 b) {
    F32 dx = a.x - b.x;
    F32 dy = a.y - b.y;
    F32 dz = a.z - b.z;
    return sqrtf(dx*dx + dy*dy + dz*dz);
}

static void* FindClosestTargetInFOV(float fovDeg) {
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
        Vec3 toTarget = {targetPos.x - camPos.x, targetPos.y - camPos.y, targetPos.z - camPos.z};
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

static Vec3 CalculateDirectionTo(void* target) {
    if (!target || !g_camera) return Vec3{0,0,1};
    Vec3 camPos = GetPlayerPosition(g_local_player);
    Player* p = (Player*)target;
    Vec3 targetPos = GetPlayerPosition(p);

    Vec3 dir = {targetPos.x - camPos.x, targetPos.y - camPos.y, targetPos.z - camPos.z};
    F32 len = sqrtf(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
    if (len < 0.001f) return Vec3{0,0,1};
    dir.x /= len; dir.y /= len; dir.z /= len;
    return dir;
}

static void ZeroSpread(void* weaponParams) {
    if (!weaponParams || !g_no_spread) return;
    WeaponParameters* wp = (WeaponParameters*)weaponParams;
    wp->spread = 0.0f;
    wp->hipFireAccuracy = 1.0f;
    wp->movementAccuracy = 1.0f;
    wp->jumpAccuracy = 1.0f;
    wp->crouchAccuracy = 1.0f;
    wp->proneAccuracy = 1.0f;
}

static void ApplyNoRecoil(F32* amount) {
    if (!g_no_recoil) return;
    *amount = 0.0f;
}

static bool IsPlayerGrounded(void* controller) {
    if (!controller) return false;
    return ((GetIsGrounded_t)(g_base + 0x2BFCC04))(controller);
}

static void DetourFirePellets(void* weaponParams, Vec3 origin, Quat rotation, ...) {
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

static void DetourSetRotation(void* transform, Quat value) {
    if (g_spin_enabled) {
        static F32 spin_yaw = 0.0f;
        spin_yaw += g_spin_speed * 0.016f;
        if (spin_yaw > 360.0f) spin_yaw -= 360.0f;
        F32 rad = spin_yaw * 3.1415926f / 180.0f;
        F32 new_y = sinf(rad);
        F32 new_w = cosf(rad);
        if (g_spin_invert_x) value.x = -value.x;
        ((SetRotation_t)g_real_setrotation)(transform, (Quat){value.x, new_y, value.z, new_w});
        return;
    }
    ((SetRotation_t)g_real_setrotation)(transform, value);
}

static bool DetourGetButton(const char* button) {
    if (g_bunnyhop && strcmp(button, "Jump") == 0) {
        void* local = GetLocalPlayer();
        if (local) {
            Transform* tr = (Transform*)((Player*)local)->transform;
            if (IsPlayerGrounded(tr)) return true;
        }
    }
    return ((GetButton_t)g_real_getbutton)(button);
}

static void DetourSendChatMessage(const char* message) {
    ((SendChatMessage_t)g_real_sendchat)(message);
}

static void ChatSpamUpdate(F32 dt) {
    if (!g_chat_spam) return;
    g_spam_timer += dt;
    if (g_spam_timer >= g_spam_interval) {
        g_spam_timer = 0.0f;
        const char* msg = g_spam_msgs[g_spam_index];
        g_spam_index++;
        if (!g_spam_msgs[g_spam_index]) g_spam_index = 0;
        if (msg) ((SendChatMessage_t)g_real_sendchat)(msg);
    }
}

static const char* DetourGetDeviceUniqueIdentifier() {
    if (g_hwid_spoof) {
        static char buf[64];
        snprintf(buf, 64, "SPOOFED_%08X", rand());
        return buf;
    }
    return ((GetDeviceUniqueIdentifier_t)g_real_hwid)();
}

static void DetourWallHackDetector(void* callback, Vec3 spawnPos) {
    if (!g_bypass_actk) {
        ((WallHackDetector_t)g_real_wallhack)(callback, spawnPos);
    }
}

static void DetourShootRaycastModule() {
    if (!g_bypass_actk) {
        ((ShootRaycastModule_t)g_real_shootraycast)();
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

    FindOffsetByName("FirePellets", &addr);
    g_real_firepellets = (void*)addr;
    InstallHook((void*)addr, (void*)DetourFirePellets, &g_trampoline_firepellets, &g_real_firepellets);

    FindOffsetByName("Transform_set_rotation", &addr);
    g_real_setrotation = (void*)addr;
    InstallHook((void*)addr, (void*)DetourSetRotation, &g_trampoline_setrotation, &g_real_setrotation);

    FindOffsetByName("Input_GetButton", &addr);
    g_real_getbutton = (void*)addr;
    InstallHook((void*)addr, (void*)DetourGetButton, &g_trampoline_getbutton, &g_real_getbutton);

    FindOffsetByName("SendChatMessage", &addr);
    g_real_sendchat = (void*)addr;
    InstallHook((void*)addr, (void*)DetourSendChatMessage, &g_trampoline_sendchat, &g_real_sendchat);

    FindOffsetByName("SystemInfo_deviceUniqueIdentifier", &addr);
    g_real_hwid = (void*)addr;
    InstallHook((void*)addr, (void*)DetourGetDeviceUniqueIdentifier, &g_trampoline_hwid, &g_real_hwid);

    if (g_bypass_actk) {
        FindOffsetByName("WallHackDetector_StartDetection", &addr);
        g_real_wallhack = (void*)addr;
        InstallHook((void*)addr, (void*)DetourWallHackDetector, &g_trampoline_wallhack, &g_real_wallhack);

        FindOffsetByName("ShootRaycastModule", &addr);
        g_real_shootraycast = (void*)addr;
        InstallHook((void*)addr, (void*)DetourShootRaycastModule, &g_trampoline_shootraycast, &g_real_shootraycast);
    }

    UpdatePlayerList();
}

extern "C" void __attribute__((constructor)) CounterFlameCheatInit() {
    g_base = 0x100000000;
    InstallAllHooks(g_base);
}
