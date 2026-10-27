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

typedef uintptr_t UPtr;
typedef int32_t   S32;
typedef uint32_t  U32;
typedef uint8_t   U8;
typedef float     F32;
typedef int64_t   S64;
typedef uint64_t  U64;

struct Vec3 { F32 x, y, z; };
struct Vec2 { F32 x, y; };
struct Quat { F32 x, y, z, w; };
struct Color { F32 r, g, b, a; };

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

struct WeaponParameters {
    F32 spread;
    F32 recoil;
    F32 damage;
    U32  bulletCount;
    F32 fireRate;
    U32  ammoPerShot;
    U32  reserveAmmo;
    U32  currentAmmo;
    F32 reloadTime;
    F32 aimDownSightsTime;
    F32 hipFireAccuracy;
    F32 movementAccuracy;
    F32 jumpAccuracy;
    F32 crouchAccuracy;
    F32 proneAccuracy;
    F32 recoilRecovery;
    F32 recoilDecay;
    F32 recoilKick;
    F32 recoilX;
    F32 recoilY;
    F32 recoilZ;
    U8   pad[256];
};

struct Camera {
    UPtr klass;
    UPtr monitor;
    UPtr transform;
    UPtr gameObject;
    UPtr actualCamera;
    UPtr pixelRect;
    UPtr targetTexture;
    UPtr cullingMask;
    UPtr eventMask;
    UPtr depth;
    UPtr renderingPath;
    UPtr hdr;
    UPtr allowMSAA;
    UPtr allowDynamicResolution;
    UPtr forceIntoRT;
    UPtr orthographic;
    UPtr orthographicSize;
    UPtr fieldOfView;
    UPtr nearClipPlane;
    UPtr farClipPlane;
    UPtr clipPlanes;
    UPtr worldToCameraMatrix;
    UPtr projectionMatrix;
    UPtr cameraToWorldMatrix;
    UPtr pixelWidth;
    UPtr pixelHeight;
    UPtr scaledPixelWidth;
    UPtr scaledPixelHeight;
    UPtr activeTexture;
    UPtr targetDisplay;
    UPtr cameraType;
    UPtr clearFlags;
    UPtr backgroundColor;
    UPtr depthTextureMode;
    UPtr layerCullDistances;
    UPtr layerCullSpherical;
    UPtr scene;
    UPtr stereoConvergence;
    UPtr stereoSeparation;
    UPtr stereoMirrorVector;
    UPtr stereoProjectionCenter;
    UPtr stereoTargetEye;
    UPtr useOcclusionCulling;
    UPtr useOcclusionCullingMask;
    UPtr allowHDR;
    UPtr xr;
    UPtr pad[256];
};

struct Transform {
    UPtr klass;
    UPtr monitor;
    UPtr parent;
    UPtr root;
    UPtr localPosition;
    UPtr localRotation;
    UPtr localScale;
    UPtr position;
    UPtr rotation;
    UPtr lossyScale;
    UPtr forward;
    UPtr backward;
    UPtr right;
    UPtr up;
    UPtr down;
    UPtr left;
    UPtr eulerAngles;
    UPtr localEulerAngles;
    UPtr childCount;
    UPtr hierarchyCapacity;
    UPtr hierarchyCount;
    UPtr pad[256];
};

struct GameObject {
    UPtr klass;
    UPtr monitor;
    UPtr transform;
    UPtr activeSelf;
    UPtr activeInHierarchy;
    UPtr name;
    UPtr tag;
    UPtr layer;
    UPtr pad[256];
};

struct Component {
    UPtr klass;
    UPtr monitor;
    UPtr gameObject;
    UPtr transform;
    UPtr pad[256];
};

struct SkinnedMeshRenderer : Component {
    UPtr sharedMaterial;
    UPtr material;
    UPtr enabled;
    UPtr castShadows;
    UPtr receiveShadows;
    UPtr pad[256];
};

struct MeshRenderer : Component {
    UPtr sharedMaterial;
    UPtr material;
    UPtr enabled;
    UPtr castShadows;
    UPtr receiveShadows;
    UPtr pad[256];
};

struct Material {
    UPtr klass;
    UPtr monitor;
    UPtr shader;
    UPtr name;
    UPtr mainTexture;
    UPtr mainTextureScale;
    UPtr mainTextureOffset;
    UPtr color;
    UPtr renderQueue;
    UPtr enableInstancing;
    UPtr doubleSidedGI;
    UPtr customRenderTexture;
    UPtr doubleSided;
    UPtr pad[256];
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
    UPtr teamColor;
    UPtr pad[256];
};

struct GameController {
    UPtr klass;
    UPtr monitor;
    UPtr players;
    UPtr playerCount;
    UPtr localPlayer;
    UPtr localTransform;
    UPtr camera;
    UPtr pad[256];
};

typedef void* (*Find_t)(const char*);
typedef void* (*GetComponent_t)(void*, void*);
typedef void (*SetRotation_t)(void*, Quat);
typedef void (*SetEulerAngles_t)(void*, Vec3);
typedef Vec3 (*GetPosition_t)(void*);
typedef Vec3 (*GetForward_t)(void*);
typedef void* (*GetMain_t)();
typedef bool (*GetButton_t)(const char*);
typedef bool (*GetIsGrounded_t)(void*);
typedef void (*Jump_t)();
typedef void (*FirePellets_t)(void*, Vec3, Quat, ...);
typedef U8 (*PlayerDamage_t)(RaycastHit, Vec3, F32);
typedef void (*SendChatMessage_t)(const char*);
typedef const char* (*GetDeviceUniqueIdentifier_t)();
typedef void* (*WallHackDetector_t)(void*, Vec3);
typedef void (*ShootRaycastModule_t)();
typedef void (*Camera_Render_t)(void*, void*);

typedef uint32_t (*SetFloat_t)(void*, const char*, F32);
typedef uint32_t (*SetColor_t)(void*, const char*, Color);
typedef uint32_t (*SetInt_t)(void*, const char*, S32);

extern "C" {
    void* Il2CppString_New(const char*);
    void* Il2CppClass_GetMethod(void*, const char*, int);
    void* Il2CppMethod_GetPointer(void*);
    void Il2Cpp_Runtime_Invoke(void*, void*, void**, void**);
    void* Il2Cpp_String_Length(void*);
    const char* Il2Cpp_String_Chars(void*);
}

struct Offset {
    const char* name;
    UPtr rva;
};

static Offset g_offsets[] = {
    {"FirePellets", 0x12783E4},
    {"PlayerDamage", 0x12796AC},
    {"AddVisible", 0x15C9670},
    {"RemoveVisible", 0x15C9A64},
    {"RemoveAllVisible", 0x15C9BF0},
    {"WorldToScreenPoint", 0x2B73E5C},
    {"Camera_main", 0x2B741F0},
    {"Camera_Render", 0x2BA2D4C},
    {"Transform_position", 0x2BB4894},
    {"Transform_set_rotation", 0x2BB4B58},
    {"Transform_set_eulerAngles", 0x2BB4B1C},
    {"Transform_set_localEulerAngles", 0x2BB4C40},
    {"Transform_get_forward", 0x2BA2D4C},
    {"Transform_get_position", 0x2BB4894},
    {"Input_GetButton", 0x24DCF0C},
    {"Input_GetButtonName", 0x2BF3334},
    {"CharacterController_isGrounded", 0x2BFCC04},
    {"Jump", 0x15CEC98},
    {"SendChatMessage", 0x158D6B0},
    {"SystemInfo_deviceUniqueIdentifier", 0x2BB19D8},
    {"SystemInfo_deviceModel", 0x2BB1AA8},
    {"WallHackDetector_StartDetection", 0x1238D04},
    {"ShootRaycastModule", 0x1239F20},
    {"Material_SetFloat", 0x2B88894},
    {"Material_SetColor", 0x2B84974},
    {"Material_SetInt", 0x2B887B8},
    {"GameObject_Find", 0x2BAB388},
    {"Component_GetComponent", 0x2BAA268},
    {0, 0}
};

struct Hook {
    void* target;
    void* detour;
    void* trampoline;
    U32 orig_insn[4];
    U32 orig_size;
};

extern UPtr g_base;
extern void InstallAllHooks(UPtr base);

extern bool g_silent_aim;
extern float g_fov;
extern int g_target_bone;
extern float g_max_distance;
extern bool g_predictive_aim;
extern int g_prediction_ms;

extern bool g_spin_enabled;
extern float g_spin_speed;
extern int g_spin_mode;
extern bool g_spin_invert_x;

extern bool g_esp_boxes;
extern bool g_esp_names;
extern bool g_esp_distance;
extern bool g_chams_enabled;
extern float g_chams_r;
extern float g_chams_g;
extern float g_chams_b;

extern bool g_no_recoil;
extern bool g_no_spread;

extern bool g_bunnyhop;

extern bool g_chat_spam;
extern float g_spam_interval;
extern int g_spam_index;
extern float g_spam_timer;

extern bool g_hwid_spoof;
extern bool g_bypass_actk;

extern bool g_menu_open;

extern void* g_player_list;
extern int g_player_count;
extern void* g_local_player;
extern void* g_camera;

struct CheatConfig {
    bool silent_aim;
    float fov;
    int target_bone;
    float max_distance;
    bool predictive_aim;
    int prediction_ms;
    bool spin_enabled;
    float spin_speed;
    int spin_mode;
    bool spin_invert_x;
    bool esp_boxes;
    bool esp_names;
    bool esp_distance;
    bool chams_enabled;
    float chams_r;
    float chams_g;
    float chams_b;
    bool no_recoil;
    bool no_spread;
    bool bunnyhop;
    bool chat_spam;
    float spam_interval;
    bool hwid_spoof;
    bool bypass_actk;
};

extern CheatConfig g_config;

void Config_Load();
void Config_Save();

extern void UpdatePlayerList();
extern void* GetLocalPlayer();
extern void* GetCamera();
extern int GetPlayerCount();
extern void* GetPlayer(int idx);
extern Vec3 GetPlayerPosition(void* player);
extern Vec3 GetCameraForward();
extern Vec3 WorldToScreenPoint(Vec3 world);
extern float Distance3D(Vec3 a, Vec3 b);
extern void* FindClosestTargetInFOV(float fovDeg);
extern Vec3 CalculateDirectionTo(void* target);
extern void ZeroSpread(void* weaponParams);
extern void ApplyNoRecoil(F32* amount);
extern bool IsPlayerGrounded(void* controller);
extern void ChatSpamUpdate(F32 dt);

extern "C" void MenuInit();
extern void MenuRenderFrame();
extern "C" void DoRender(void* descriptor);
