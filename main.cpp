#include "cheat.h"
#include <mach-o/dyld.h>
#include <sys/mman.h>
#include <unistd.h>

static UPtr GetModuleBase(const char* name) {
    U32 count = _dyld_image_count();
    for (U32 i = 0; i < count; i++) {
        const char* path = _dyld_get_image_name(i);
        if (strstr(path, name)) {
            return (UPtr)_dyld_get_image_vmaddr_slide(i);
        }
    }
    return 0;
}

extern "C" int main(int argc, char** argv) {
    UPtr base = GetModuleBase("UnityFramework");
    if (!base) base = 0x100000000;
    InstallAllHooks(base);
    return 0;
}
