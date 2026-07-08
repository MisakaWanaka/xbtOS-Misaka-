#ifndef XBTOS_APP_APP_H
#define XBTOS_APP_APP_H

#include <types.h>

namespace app {

enum AppPermission : uint32_t {
    APP_PERMISSION_NONE = 0,
    APP_PERMISSION_CONSOLE = 1 << 0,
    APP_PERMISSION_FILE_READ = 1 << 1,
    APP_PERMISSION_FILE_WRITE = 1 << 2,
    APP_PERMISSION_NETWORK = 1 << 3,
    APP_PERMISSION_HARDWARE = 1 << 4,
    APP_PERMISSION_PROCESS = 1 << 5
};

enum AppResult {
    APP_OK = 0,
    APP_ERR_INVALID = -1,
    APP_ERR_DENIED = -2,
    APP_ERR_NOT_FOUND = -3,
    APP_ERR_FULL = -4
};

struct AppManifest {
    const char* identifier;
    const char* display_name;
    const char* version;
    uint32_t requested_permissions;
};

struct AppSandbox {
    char container_path[64];
    uint32_t granted_permissions;
};

struct AppContext {
    const AppManifest* manifest;
    AppSandbox sandbox;
};

typedef int (*AppEntryPoint)(AppContext* context);

struct AppBundle {
    AppManifest manifest;
    AppEntryPoint entry;
    bool is_used;
};

class AppManager {
private:
    AppBundle apps[16];
    uint32_t app_count;

public:
    void init();
    AppResult register_app(const AppManifest* manifest, AppEntryPoint entry);
    AppResult launch(const char* identifier);
    bool has_permission(const AppContext* context, AppPermission permission) const;
};

AppManager& manager();

} // namespace app

#endif
