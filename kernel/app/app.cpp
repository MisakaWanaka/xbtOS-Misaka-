#include "app.h"
#include <string.h>
#include <hal/console.h>

namespace app {

static AppManager global_manager;

static uint32_t default_allowed_permissions(uint32_t requested) {
    const uint32_t safe_permissions =
        APP_PERMISSION_CONSOLE |
        APP_PERMISSION_FILE_READ |
        APP_PERMISSION_FILE_WRITE;

    return requested & safe_permissions;
}

static void build_container_path(const char* identifier, char* out_path, uint32_t out_size) {
    const char* prefix = "/Applications/";
    uint32_t pos = 0;

    while (prefix[pos] != '\0' && pos + 1 < out_size) {
        out_path[pos] = prefix[pos];
        ++pos;
    }

    for (uint32_t i = 0; identifier && identifier[i] != '\0' && pos + 1 < out_size; ++i) {
        char c = identifier[i];
        out_path[pos++] = (c == '.') ? '/' : c;
    }

    if (pos + 5 < out_size) {
        out_path[pos++] = '.';
        out_path[pos++] = 'a';
        out_path[pos++] = 'p';
        out_path[pos++] = 'p';
    }
    out_path[pos] = '\0';
}

void AppManager::init() {
    app_count = 0;
    kmemset(apps, 0, sizeof(apps));
}

AppResult AppManager::register_app(const AppManifest* manifest, AppEntryPoint entry) {
    if (!manifest || !manifest->identifier || !entry) {
        return APP_ERR_INVALID;
    }

    if (app_count >= 16) {
        return APP_ERR_FULL;
    }

    for (uint32_t i = 0; i < app_count; ++i) {
        if (apps[i].is_used && kstrcmp(apps[i].manifest.identifier, manifest->identifier) == 0) {
            return APP_ERR_INVALID;
        }
    }

    apps[app_count].manifest = *manifest;
    apps[app_count].entry = entry;
    apps[app_count].is_used = true;
    ++app_count;
    return APP_OK;
}

AppResult AppManager::launch(const char* identifier) {
    for (uint32_t i = 0; i < app_count; ++i) {
        if (!apps[i].is_used || kstrcmp(apps[i].manifest.identifier, identifier) != 0) {
            continue;
        }

        AppContext context;
        context.manifest = &apps[i].manifest;
        context.sandbox.granted_permissions =
            default_allowed_permissions(apps[i].manifest.requested_permissions);
        build_container_path(identifier, context.sandbox.container_path, sizeof(context.sandbox.container_path));

        hal::print_string("Launching app: ");
        hal::print_string(apps[i].manifest.display_name);
        hal::print_string("\nSandbox: ");
        hal::print_string(context.sandbox.container_path);
        hal::print_string("\n");

        apps[i].entry(&context);
        return APP_OK;
    }

    return APP_ERR_NOT_FOUND;
}

bool AppManager::has_permission(const AppContext* context, AppPermission permission) const {
    if (!context) {
        return false;
    }
    return (context->sandbox.granted_permissions & permission) == permission;
}

AppManager& manager() {
    return global_manager;
}

} // namespace app
