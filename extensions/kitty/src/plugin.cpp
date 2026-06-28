#include "types.h"
#include "version.h"

#include <stdint.h>

#if defined(__clang__)
#define WOKI_IMPORT(module, name) __attribute__((import_module(module), import_name(name)))
#define WOKI_EXPORT(name) __attribute__((export_name(name)))
#else
#define WOKI_IMPORT(module, name)
#define WOKI_EXPORT(name)
#endif



extern "C" {

WOKI_IMPORT("woki_host", "host_log")
int32_t host_log(woki_ext_log_level_t level, const char* message, uint32_t len);

static uint8_t g_event_payload[4096];

WOKI_EXPORT("ext_alloc")
uint32_t ext_alloc(uint32_t len) {
    if (len == 0 || len > sizeof(g_event_payload)) {
        return 0;
    }
    return reinterpret_cast<uint32_t>(g_event_payload);
}

WOKI_EXPORT("ext_free")
void ext_free(uint32_t ptr, uint32_t len) {
    (void)ptr;
    (void)len;
}

WOKI_EXPORT("ext_api_version")
uint32_t ext_api_version(void) { return WOKI_EXT_API_VERSION; }

WOKI_EXPORT("ext_init")
int32_t ext_init(void) {
    static constexpr char kMessage[] = "hello from wokiext";
    return host_log(WOKI_EXT_LOG_INFO, kMessage, sizeof(kMessage) - 1);
}

WOKI_EXPORT("ext_on_tick")
void ext_on_tick(double dt_ms) { 
    (void)dt_ms;
    static constexpr char kMessage[] = "loop";
    host_log(WOKI_EXT_LOG_INFO, kMessage, sizeof(kMessage) - 1);
}

WOKI_EXPORT("ext_on_event")
void ext_on_event(uint32_t type, const uint8_t* payload, uint32_t len) {
    (void)type;
    (void)payload;
    (void)len;
}

WOKI_EXPORT("ext_on_unload")
void ext_on_unload(void) {}

} // extern "C"
