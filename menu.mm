#include "cheat.h"
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIKit.h>

#include <imgui.h>
#import <imgui_impl_metal.h>

// ========== ГЛОБАЛЫ МЕНЮ ==========
static bool g_menu_initialized = false;
static bool g_menu_shown = false;
static id<MTLDevice> g_metal_device = nil;
static id<MTLCommandQueue> g_metal_queue = nil;
static MTKView* g_mtk_view = nil;
static CADisplayLink* g_display_link = nil;
static DisplayLinkTarget* g_display_target = nil;
static UIWindow* g_overlay_window = nil;

extern bool g_menu_visible;
float g_menu_slide = 0.0f;
float g_menu_target = 0.0f;

// ========== DisplayLink Target ==========
@interface DisplayLinkTarget : NSObject
@end
@implementation DisplayLinkTarget
- (void)fire:(CADisplayLink *)link {
    extern void UpdateFeaturesReal();
    UpdateFeaturesReal();
}
@end

// ========== MTKView Delegate ==========
@interface ImGuiMTKViewDelegate : NSObject <MTKViewDelegate>
@end
@implementation ImGuiMTKViewDelegate
- (void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size {
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(size.width, size.height);
}
- (void)drawInMTKView:(MTKView*)view {
    if (!g_menu_initialized) return;
    MTLRenderPassDescriptor *rpd = view.currentRenderPassDescriptor;
    id<MTLDrawable> drawable = view.currentDrawable;
    if (!rpd || !drawable) return;

    // Прозрачная очистка кадра, чтобы не закрашивать игру
    rpd.colorAttachments[0].loadAction = MTLLoadActionClear;
    rpd.colorAttachments[0].clearColor = MTLClearColorMake(0.0f, 0.0f, 0.0f, 0.0f);

    ImGui_ImplMetal_NewFrame(rpd);
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 ws = io.DisplaySize;

    float panelW = ws.x * 0.92f;
    float panelH = ws.y * 0.45f;
    float panelX = (ws.x - panelW) * 0.5f;
    float panelY = ws.y - panelH + panelH * (1.0f - g_menu_slide);

    ImGui::SetNextWindowPos(ImVec2(panelX, panelY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(panelW, panelH), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, g_menu_opacity * g_menu_slide);

    ImGui::Begin("CounterFlame Internal", &g_menu_visible,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                 ImGuiWindowFlags_NoBringToFrontOnFocus |
                 ImGuiWindowFlags_NoSavedSettings);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 winSize = ImGui::GetWindowSize();

    dl->AddRectFilled(winPos, ImVec2(winPos.x + winSize.x, winPos.y + winSize.y),
                      ImGui::ColorConvertFloat4ToU32(ImVec4(0.10f, 0.10f, 0.12f, 0.98f)), 12.0f);
    dl->AddRectFilled(ImVec2(winPos.x, winPos.y),
                      ImVec2(winPos.x + winSize.x, winPos.y + 3.0f),
                      ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 1.0f, 0.5f, 1.0f)), 2.0f);

    // Tabs
    static int tab = 0;
    const char* tabs[] = { "Aimbot", "Anti-Aim", "Visuals", "Weapon", "Movement", "Chat", "Anti-Ban", "Skin", "LOG" };
    int tabCount = 9;
    float tabW = (winSize.x - 32.0f) / tabCount;
    float tabH = 40.0f;

    for (int i = 0; i < tabCount; i++) {
        ImVec2 tp(winPos.x + 16.0f + i * tabW, winPos.y + 30.0f);
        bool active = (tab == i);
        ImVec4 col = active ? ImVec4(0.0f, 1.0f, 0.5f, 1.0f) : ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
        dl->AddRectFilled(ImVec2(tp.x, tp.y),
                          ImVec2(tp.x + tabW - 16.0f, tp.y + tabH),
                          ImGui::ColorConvertFloat4ToU32(col), 6.0f);
        ImGui::SetCursorPos(ImVec2(tp.x + 8.0f, tp.y + 8.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, active ? ImVec4(0.05f, 0.05f, 0.07f, 1.0f) : ImVec4(0.60f, 0.60f, 0.65f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0,0,0,0));
        if (ImGui::Button(tabs[i], ImVec2(tabW - 16.0f, tabH - 16.0f))) tab = i;
        ImGui::PopStyleColor(4); // исправлено: было 3, теперь 4
    }

    ImGui::SetCursorPos(ImVec2(20.0f, 46.0f));
    ImGui::BeginChild("##MenuContent", ImVec2(winSize.x - 40.0f, winSize.y - 56.0f),
                      false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);

    if (tab == 0) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "AIMBOT");
        ImGui::Separator();
        ImGui::Checkbox("Silent Aim", &g_silent_aim);
        ImGui::SameLine(); ImGui::TextColored(ImVec4(0.6f,0.6f,0.65f,1.0f), "Кроссхэир на месте, пуля в цель");
        ImGui::SliderFloat("FOV", &g_fov, 1.0f, 90.0f, "%.1f°");
        ImGui::SliderFloat("Max Dist", &g_max_distance, 10.0f, 500.0f, "%.0f м");
        ImGui::Combo("Bone", &g_target_bone, "Head\0Body\0Neck\0");
        ImGui::Checkbox("Predictive Aim", &g_predictive_aim);
        ImGui::SliderInt("Pred (ms)", &g_prediction_ms, 0, 500, "%d мс");
    } else if (tab == 1) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "ANTI-AIM");
        ImGui::Separator();
        ImGui::Checkbox("Spinbot", &g_spin_enabled);
        ImGui::Combo("Mode", &g_spin_mode, "Off\0Spin\0Jitter\0Static\0");
        ImGui::SliderFloat("Speed", &g_spin_speed, 10.0f, 720.0f, "%.0f deg/s");
        ImGui::Checkbox("Invert X", &g_spin_invert_x);
    } else if (tab == 2) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "VISUALS / ESP");
        ImGui::Separator();
        ImGui::Checkbox("ESP Boxes", &g_esp_boxes);
        ImGui::SameLine(); ImGui::TextColored(ImVec4(0.6f,0.6f,0.65f,1.0f), "Боксы вокруг врагов");
        ImGui::Checkbox("ESP Names", &g_esp_names);
        ImGui::Checkbox("ESP Distance", &g_esp_distance);
        ImGui::Checkbox("Chams", &g_chams_enabled);
        ImGui::SameLine(); ImGui::ColorEdit3("Chams Color", &g_chams_r);
    } else if (tab == 3) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "WEAPON");
        ImGui::Separator();
        ImGui::Checkbox("No Recoil", &g_no_recoil);
        ImGui::SameLine(); ImGui::TextColored(ImVec4(0.6f,0.6f,0.65f,1.0f), "Убирает отдачу");
        ImGui::Checkbox("No Spread", &g_no_spread);
        ImGui::SameLine(); ImGui::TextColored(ImVec4(0.6f,0.6f,0.65f,1.0f), "Убирает разброс");
    } else if (tab == 4) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "MOVEMENT");
        ImGui::Separator();
        ImGui::Checkbox("Bunnyhop", &g_bunnyhop);
        ImGui::SameLine(); ImGui::TextColored(ImVec4(0.6f,0.6f,0.65f,1.0f), "Автопрыжок");
    } else if (tab == 5) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "CHAT");
        ImGui::Separator();
        ImGui::Checkbox("Chat Spam", &g_chat_spam);
        ImGui::SameLine(); ImGui::TextColored(ImVec4(0.6f,0.6f,0.65f,1.0f), "Автосообщения");
        ImGui::SliderFloat("Interval", &g_spam_interval, 1.0f, 10.0f, "%.1f с");
    } else if (tab == 6) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "ANTI-BAN");
        ImGui::Separator();
        ImGui::Checkbox("HWID Spoof", &g_hwid_spoof);
        ImGui::SameLine(); ImGui::TextColored(ImVec4(0.6f,0.6f,0.65f,1.0f), "Подмена ID");
        ImGui::Checkbox("Bypass ACTk", &g_bypass_actk);
        ImGui::SameLine(); ImGui::TextColored(ImVec4(0.6f,0.6f,0.65f,1.0f), "Обход Anti-Cheat Toolkit");
        ImGui::Checkbox("Bypass Version", &g_bypass_version);
        ImGui::SameLine(); ImGui::TextColored(ImVec4(0.6f,0.6f,0.65f,1.0f), "Обход версии");
        ImGui::Checkbox("Bypass RootDetect", &g_bypass_rootdetect);
        ImGui::SameLine(); ImGui::TextColored(ImVec4(0.6f,0.6f,0.65f,1.0f), "Обход джейлбрейк детекта");
    } else if (tab == 7) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "SKIN CHANGER");
        ImGui::Separator();
        ImGui::Checkbox("Skin Changer", &g_skin_changer_enabled);
        ImGui::SameLine(); ImGui::TextColored(ImVec4(0.6f,0.6f,0.65f,1.0f), "Видно всем");
        ImGui::SliderInt("Skin ID", &g_skin_id, 1, 1000, "%d");
        ImGui::SliderInt("Seed", &g_skin_seed, 1, 9999, "%d");
        ImGui::Checkbox("StatTrak", &g_skin_stattrak);
        ImGui::SameLine(); ImGui::SliderInt("StatTrak Value", &g_stattrak_value, 0, 9999, "%d");
        ImGui::Checkbox("Update Players", &g_skin_update_players);
    } else if (tab == 8) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "LOG");
        ImGui::Separator();
        ImGui::TextUnformatted(g_log_buffer);
    }

    ImGui::EndChild();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.6f,0.6f,0.65f,1.0f), "Chronicle Engine v3.0 | CounterFlame 1.3.0.1");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "STATUS: ACTIVE");
    ImGui::End();
    ImGui::PopStyleVar(); // соответствует PushStyleVar(ImGuiStyleVar_Alpha)

    // Render
    id<MTLCommandBuffer> cmdBuf = [g_metal_queue commandBuffer];
    if (cmdBuf) {
        id<MTLRenderCommandEncoder> enc = [cmdBuf renderCommandEncoderWithDescriptor:rpd];
        if (enc) {
            ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), cmdBuf, enc);
            [enc endEncoding];
        }
        [cmdBuf presentDrawable:drawable];
        [cmdBuf commit];
    }
}
@end

static ImGuiMTKViewDelegate* g_delegate = nil;

// ========== МЕНЮ ФУНКЦИИ ==========
extern "C" void MenuInit() {
    if (g_menu_initialized) return;

    // Получаем ключевое окно через connectedScenes (iOS 13+)
    UIWindow* window = nil;
    UIWindowScene* windowScene = nil;
    if (@available(iOS 13.0, *)) {
        for (UIScene* scene in [UIApplication sharedApplication].connectedScenes) {
            if ([scene isKindOfClass:[UIWindowScene class]] &&
                scene.activationState == UISceneActivationStateForegroundActive) {
                windowScene = (UIWindowScene*)scene;
                break;
            }
        }
    } else {
        window = [UIApplication sharedApplication].keyWindow;
    }

    if (!windowScene && !window) {
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.5 * NSEC_PER_SEC)),
                       dispatch_get_main_queue(), ^{
                           MenuInit();
                       });
        return;
    }

    g_metal_device = MTLCreateSystemDefaultDevice();
    if (!g_metal_device) return;
    g_metal_queue = [g_metal_device newCommandQueue];

    // MTKView для рендеринга ImGui через Metal
    CGRect frame = window ? window.bounds : windowScreenBounds(windowScene);
    g_mtk_view = [[MTKView alloc] initWithFrame:frame device:g_metal_device];
    g_mtk_view.framebufferOnly = YES;
    g_mtk_view.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
    g_mtk_view.preferredFramesPerSecond = 60;
    g_mtk_view.enableSetNeedsDisplay = NO;
    g_mtk_view.hidden = !g_menu_visible;
    g_mtk_view.backgroundColor = [UIColor clearColor];
    g_mtk_view.opaque = NO;
    g_mtk_view.layer.opaque = NO;
    g_mtk_view.layer.backgroundColor = [UIColor clearColor].CGColor;

    g_delegate = [[ImGuiMTKViewDelegate alloc] init];
    g_mtk_view.delegate = g_delegate;

    // Создаём отдельное окно поверх Unity
    if (windowScene) {
        g_overlay_window = [[UIWindow alloc] initWithWindowScene:windowScene];
        g_overlay_window.windowLevel = UIWindowLevelAlert + 1;
        g_overlay_window.backgroundColor = [UIColor clearColor];
        UIViewController* rootVC = [[UIViewController alloc] init];
        rootVC.view.backgroundColor = [UIColor clearColor];
        g_overlay_window.rootViewController = rootVC;
        [g_overlay_window makeKeyAndVisible];
        g_mtk_view.frame = g_overlay_window.bounds;
        [g_overlay_window.rootViewController.view addSubview:g_mtk_view];
    } else {
        [window addSubview:g_mtk_view];
        [window bringSubviewToFront:g_mtk_view];
    }

    // ImGui контекст
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = NULL;
    io.LogFilename = NULL;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.DisplaySize = ImVec2(frame.size.width, frame.size.height);

    ImGui::StyleColorsDark();
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowPadding = ImVec2(12, 12);
    s.WindowRounding = 12.0f;
    s.FramePadding = ImVec2(10, 7);
    s.FrameRounding = 8.0f;
    s.ItemSpacing = ImVec2(10, 7);
    s.GrabMinSize = 14.0f;
    s.GrabRounding = 8.0f;
    s.TabRounding = 10.0f;

    ImVec4* colors = s.Colors;
    colors[ImGuiCol_WindowBg]      = ImVec4(0.08f, 0.08f, 0.10f, 0.97f);
    colors[ImGuiCol_FrameBg]       = ImVec4(0.15f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]= ImVec4(0.20f, 0.20f, 0.23f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.0f, 1.0f, 0.5f, 1.00f);
    colors[ImGuiCol_Button]        = ImVec4(0.20f, 0.20f, 0.23f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 1.0f, 0.5f, 1.00f);
    colors[ImGuiCol_ButtonActive]  = ImVec4(0.0f, 1.0f, 0.5f, 1.00f);
    colors[ImGuiCol_Header]        = ImVec4(0.20f, 0.20f, 0.23f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.25f, 0.28f, 1.00f);
    colors[ImGuiCol_HeaderActive]  = ImVec4(0.0f, 1.0f, 0.5f, 1.00f);
    colors[ImGuiCol_Text]          = ImVec4(0.95f, 0.95f, 0.97f, 1.00f);
    colors[ImGuiCol_TextDisabled]  = ImVec4(0.60f, 0.60f, 0.65f, 1.00f);

    ImGui_ImplMetal_Init(g_metal_device);

    // CADisplayLink
    g_display_target = [[DisplayLinkTarget alloc] init];
    g_display_link = [CADisplayLink displayLinkWithTarget:g_display_target selector:@selector(fire:)];
    g_display_link.preferredFramesPerSecond = 60;
    [g_display_link addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];

    g_menu_initialized = true;
}

extern "C" void MenuToggle() {
    if (!g_menu_initialized) MenuInit();
    g_menu_visible = !g_menu_visible;
    if (g_mtk_view) g_mtk_view.hidden = !g_menu_visible;
}

extern "C" bool MenuIsOpen() { return g_menu_visible; }

extern "C" void MenuSetOpen(bool open) {
    g_menu_visible = open;
    if (g_mtk_view) g_mtk_view.hidden = !g_menu_visible;
}

extern "C" void MenuRender(float dt, void* renderPassDescriptor) {
    (void)dt; (void)renderPassDescriptor;
}

// Вспомогательная функция для получения фрейма окна сцены
static CGRect windowScreenBounds(UIWindowScene* scene) {
    if (@available(iOS 13.0, *)) {
        return [scene.coordinateSpace convertRect:scene.coordinateSpace.bounds fromCoordinateSpace:nil];
    }
    return CGRectZero;
}