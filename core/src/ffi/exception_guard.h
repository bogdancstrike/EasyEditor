#ifndef VX_FFI_EXCEPTION_GUARD_H
#define VX_FFI_EXCEPTION_GUARD_H

#include <new>

#include "util/error.h"

#define VX_FFI_TRY try

#define VX_FFI_CATCH              \
    catch (const ::vx::Error& e) { \
        return e.status();         \
    }                              \
    catch (const std::bad_alloc&) {\
        return VX_ERR_OOM;         \
    }                              \
    catch (...) {                  \
        return VX_ERR_INTERNAL;    \
    }

#endif  // VX_FFI_EXCEPTION_GUARD_H
