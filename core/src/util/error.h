#ifndef VX_UTIL_ERROR_H
#define VX_UTIL_ERROR_H

#include <stdexcept>
#include <string>

#include "vx/public_api.h"  // for vx_status_t

namespace vx {

/// Engine-internal exception. Carries a status code for FFI translation.
class Error : public std::runtime_error {
public:
    Error(vx_status_t status, std::string message)
        : std::runtime_error(std::move(message)), status_(status) {}

    [[nodiscard]] vx_status_t status() const noexcept { return status_; }

private:
    vx_status_t status_;
};

}  // namespace vx

#endif  // VX_UTIL_ERROR_H
