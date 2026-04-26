#include "vx/public_api.h"

#include <atomic>

namespace {
std::atomic_bool g_initialized{false};
}

extern "C" {

const char* vx_status_message(vx_status_t status) {
    switch (status) {
        case VX_OK:
            return "ok";
        case VX_ERR_INVALID_ARG:
            return "invalid argument";
        case VX_ERR_IO:
            return "io error";
        case VX_ERR_DECODE:
            return "decode error";
        case VX_ERR_GPU:
            return "gpu error";
        case VX_ERR_OOM:
            return "out of memory";
        case VX_ERR_UNSUPPORTED:
            return "unsupported";
        case VX_ERR_NOT_FOUND:
            return "not found";
        case VX_ERR_INTERNAL:
            return "internal error";
    }
    return "unknown status";
}

vx_status_t vx_init(void) {
    g_initialized.store(true);
    return VX_OK;
}

void vx_shutdown(void) {
    g_initialized.store(false);
}

}  // extern "C"
