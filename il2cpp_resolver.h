#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <unordered_map>
#include <dlfcn.h>
#include <mach-o/dyld.h>

typedef uintptr_t UPtr;

struct IL2CPPResolver {
    void* hmod = nullptr;
    void* domain = nullptr;
    std::unordered_map<std::string, void*> cache;

    bool Init(const char* binary_name = "UnityFramework") {
        hmod = dlopen(binary_name, RTLD_LAZY | RTLD_NOLOAD);
        if (!hmod) {
            for (uint32_t i = 0; i < _dyld_image_count(); i++) {
                const char* name = _dyld_get_image_name(i);
                if (name && strstr(name, binary_name)) {
                    hmod = (void*)_dyld_get_image_header(i);
                    break;
                }
            }
        }
        if (!hmod) return false;

        static const char* exports[] = {
            "il2cpp_init", "il2cpp_domain_get", "il2cpp_thread_attach",
            "il2cpp_class_from_name", "il2cpp_class_get_fields",
            "il2cpp_class_get_method_from_name", "il2cpp_class_get_property_from_name",
            "il2cpp_field_get_offset", "il2cpp_field_get_name", "il2cpp_field_get_type",
            "il2cpp_method_get_name", "il2cpp_method_get_param_count",
            "il2cpp_method_get_param", "il2cpp_method_get_return_type",
            "il2cpp_type_get_name", "il2cpp_object_new",
            "il2cpp_runtime_invoke", "il2cpp_string_new",
            "il2cpp_assembly_get_image", "il2cpp_image_get_name",
            "il2cpp_domain_get_assemblies", "il2cpp_class_get_type",
            "il2cpp_image_get_class_count", "il2cpp_image_get_class",
            nullptr
        };

        for (int i = 0; exports[i]; i++) {
            void* addr = dlsym(hmod, exports[i]);
            if (!addr) return false;
            cache[exports[i]] = addr;
        }

        domain = Invoke<void*>("il2cpp_domain_get");
        if (domain) Invoke<void*>("il2cpp_thread_attach", domain);
        return domain != nullptr;
    }

    template <typename Ret, typename... Args>
    Ret Invoke(const char* name, Args... args) {
        auto it = cache.find(name);
        if (it == cache.end()) return Ret{};
        return ((Ret(*)(Args...))it->second)(args...);
    }

    void* FindAssembly(const char* name) {
        size_t count = 0;
        void** asses = Invoke<void**>("il2cpp_domain_get_assemblies", domain, &count);
        for (size_t i = 0; i < count; i++) {
            void* img = Invoke<void*>("il2cpp_assembly_get_image", asses[i]);
            const char* aname = Invoke<const char*>("il2cpp_image_get_name", img);
            if (aname && strcmp(aname, name) == 0) return asses[i];
        }
        return nullptr;
    }

    void* GetClass(const char* assembly, const char* ns, const char* klass) {
        void* ass = FindAssembly(assembly);
        if (!ass) return nullptr;
        void* img = Invoke<void*>("il2cpp_assembly_get_image", ass);
        return Invoke<void*>("il2cpp_class_from_name", img, ns, klass);
    }

    void* GetMethod(void* klass, const char* name, int argc = -1) {
        return Invoke<void*>("il2cpp_class_get_method_from_name", klass, name, argc);
    }

    uint32_t GetFieldOffset(void* klass, const char* field_name) {
        void* field = Invoke<void*>("il2cpp_class_get_field_from_name", klass, field_name);
        if (!field) return 0;
        return Invoke<uint32_t>("il2cpp_field_get_offset", field);
    }

    // Чтение/запись памяти через vm_write (non-jb safe)
    template <typename T>
    T Read(void* base, uint32_t offset) {
        return *reinterpret_cast<T*>((uintptr_t)base + offset);
    }

    template <typename T>
    void Write(void* base, uint32_t offset, T value) {
        *reinterpret_cast<T*>((uintptr_t)base + offset) = value;
    }

    // Патчинг байтов в памяти (vm_write для non-jb)
    bool PatchBytes(void* addr, const void* data, size_t size) {
        if (!addr) return false;
        kern_return_t kr = mach_vm_protect(mach_task_self(), (mach_vm_address_t)addr, size, FALSE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_COPY);
        if (kr != KERN_SUCCESS) return false;
        kr = mach_vm_write(mach_task_self(), (mach_vm_address_t)addr, (vm_offset_t)data, (mach_msg_type_number_t)size);
        return kr == KERN_SUCCESS;
    }

    // NOP патч (4 байта ARM64 NOP = 0xD503201F)
    bool Nop(void* addr, size_t count = 1) {
        uint32_t nop = 0xD503201F;
        for (size_t i = 0; i < count; i++) {
            if (!PatchBytes((char*)addr + i * 4, &nop, 4)) return false;
        }
        return true;
    }
};

inline IL2CPPResolver g_il2cpp;