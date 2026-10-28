#pragma once

#include <cstdint>
#include <cstring>
#include <cstdio>
#include <vector>
#include <string>
#include <unordered_map>
#include <dlfcn.h>

typedef uintptr_t UPtr;

struct IL2CPPResolver {
    void* hmodule = nullptr;
    void* domain = nullptr;
    std::unordered_map<std::string, void*> func_cache;

    enum class Mode { Il2Cpp };

    bool Initialize(void* module_handle) {
        hmodule = module_handle;
        if (!hmodule) return false;

        // Resolve core IL2CPP exports
        const char* exports[] = {
            "il2cpp_init",
            "il2cpp_domain_get",
            "il2cpp_thread_attach",
            "il2cpp_thread_detach",
            "il2cpp_class_from_name",
            "il2cpp_class_get_fields",
            "il2cpp_class_get_field_from_name",
            "il2cpp_class_get_methods",
            "il2cpp_class_get_method_from_name",
            "il2cpp_class_get_property_from_name",
            "il2cpp_class_get_type",
            "il2cpp_domain_get_assemblies",
            "il2cpp_image_get_class",
            "il2cpp_image_get_class_count",
            "il2cpp_resolve_icall",
            "il2cpp_string_new",
            "il2cpp_type_get_object",
            "il2cpp_object_new",
            "il2cpp_method_get_param_name",
            "il2cpp_method_get_param",
            "il2cpp_class_from_il2cpp_type",
            "il2cpp_field_static_get_value",
            "il2cpp_field_static_set_value",
            "il2cpp_runtime_invoke",
            "il2cpp_object_unbox",
            "il2cpp_assembly_get_image",
            "il2cpp_image_get_filename",
            "il2cpp_image_get_name",
            "il2cpp_class_get_name",
            "il2cpp_class_get_parent",
            "il2cpp_class_get_namespace",
            "il2cpp_field_get_name",
            "il2cpp_field_get_type",
            "il2cpp_field_get_offset",
            "il2cpp_method_get_name",
            "il2cpp_method_get_return_type",
            "il2cpp_method_get_flags",
            "il2cpp_method_get_param_count",
            "il2cpp_type_get_name",
            nullptr
        };

        for (int i = 0; exports[i]; i++) {
            void* addr = dlsym(hmodule, exports[i]);
            if (!addr) {
                printf("[IL2CPP] Failed to resolve %s\n", exports[i]);
                return false;
            }
            func_cache[exports[i]] = addr;
        }

        // Get domain and attach thread
        domain = Invoke<void*>("il2cpp_domain_get");
        if (!domain) return false;
        Invoke<void*>("il2cpp_thread_attach", domain);
        return true;
    }

    template <typename Return, typename... Args>
    Return Invoke(const char* func_name, Args... args) {
        auto it = func_cache.find(func_name);
        if (it == func_cache.end()) return Return();
        using Fn = Return(*)(Args...);
        return reinterpret_cast<Fn>(it->second)(args...);
    }

    template <typename Return, typename... Args>
    Return Invoke(void* func_ptr, Args... args) {
        if (!func_ptr) return Return();
        using Fn = Return(*)(Args...);
        return reinterpret_cast<Fn>(func_ptr)(args...);
    }

    void* GetMethod(const char* assembly_name, const char* namespace_name, const char* class_name, const char* method_name, int args_count = -1) {
        void* assembly = FindAssembly(assembly_name);
        if (!assembly) return nullptr;
        void* image = Invoke<void*>("il2cpp_assembly_get_image", assembly);
        void* klass = Invoke<void*>("il2cpp_class_from_name", image, namespace_name, class_name);
        if (!klass) return nullptr;
        void* method = Invoke<void*>("il2cpp_class_get_method_from_name", klass, method_name, args_count);
        return method;
    }

    void* GetFieldOffset(const char* assembly_name, const char* namespace_name, const char* class_name, const char* field_name) {
        void* assembly = FindAssembly(assembly_name);
        if (!assembly) return nullptr;
        void* image = Invoke<void*>("il2cpp_assembly_get_image", assembly);
        void* klass = Invoke<void*>("il2cpp_class_from_name", image, namespace_name, class_name);
        if (!klass) return nullptr;
        void* field = Invoke<void*>("il2cpp_class_get_field_from_name", klass, field_name);
        if (!field) return nullptr;
        return (void*)(uintptr_t)Invoke<int>("il2cpp_field_get_offset", field);
    }

    void* FindAssembly(const char* name) {
        size_t count = 0;
        void** assemblies = Invoke<void**>("il2cpp_domain_get_assemblies", domain, &count);
        for (size_t i = 0; i < count; i++) {
            void* img = Invoke<void*>("il2cpp_assembly_get_image", assemblies[i]);
            const char* aname = Invoke<const char*>("il2cpp_image_get_name", img);
            if (aname && strcmp(aname, name) == 0) return assemblies[i];
        }
        return nullptr;
    }

    template <typename T>
    T Read(void* obj, uintptr_t offset) {
        return *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(obj) + offset);
    }

    template <typename T>
    void Write(void* obj, uintptr_t offset, T value) {
        *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(obj) + offset) = value;
    }
};

inline IL2CPPResolver g_il2cpp;