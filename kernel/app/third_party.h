#ifndef XBTOS_APP_THIRD_PARTY_H
#define XBTOS_APP_THIRD_PARTY_H

#include "app.h"

#define XBTOS_APP_API_VERSION 1

extern "C" {

typedef struct XbtosAppHost {
    uint32_t api_version;
    void (*print)(const char* text);
    int (*has_permission)(uint32_t permission);
} XbtosAppHost;

typedef int (*XbtosThirdPartyMain)(XbtosAppHost* host);

}

#endif
