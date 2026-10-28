#include "cheat.h"
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <sys/mman.h>
#include <unistd.h>

extern "C" void MenuInit();

static UPtr GetModuleBase(const char* name) {
    U32 count = _dyld_image_count();
    for (U32 i = 0; i < count; i++) {
        const char* path = _dyld_get_image_name(i);
        if (path && strstr(path, name)) {
            return (UPtr)_dyld_get_image_header(i);
        }
    }
    return 0;
}

int main(int argc, char** argv) {
    UPtr base = GetModuleBase("UnityFramework");
    if (!base) base = 0x100000000;
    InstallAllHooks(base);
    MenuInit();
    return 0;
}
