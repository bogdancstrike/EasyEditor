#include "vx/public_api.h"

#include "vx/version.h"

extern "C" {

const char* vx_version(void) {
    return VX_VERSION_STRING;
}

}  // extern "C"
