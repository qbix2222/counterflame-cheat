ARCHS = arm64
TARGET = iphone:clang:15.0:15.0
DEBUG = 0
FINALPACKAGE = 1

THEOS_DEVICE_IP = localhost
THEOS_DEVICE_PORT = 22

INSTALL_TARGET_PROCESSES = CounterFlame

include $(THEOS)/makefiles/common.mk

TWEAK_NAME = CounterFlameCheat

CounterFlameCheat_FILES = Tweak.xm \
                          Hooks/HookManager.mm \
                          Hooks/IL2CPPResolver.mm \
                          Menu/Menu.mm \
                          Menu/ESP.mm \
                          Menu/Switches.mm \
                          Utils/KittyMemory/MemoryPatch.cpp \
                          Utils/KittyMemory/KittyMemory.cpp

CounterFlameCheat_CFLAGS = -fobjc-arc -Wno-deprecated-declarations -Wno-unused-variable -Wno-unused-function
CounterFlameCheat_CXXFLAGS = -std=c++20 -fvisibility=hidden
CounterFlameCheat_LDFLAGS = -lsubstrate
CounterFlameCheat_FRAMEWORKS = UIKit CoreGraphics QuartzCore Foundation

include $(THEOS_MAKE_PATH)/tweak.mk

after-install::
	install.exec "killall -9 CounterFlame"