#include "uuid.h"

#include <random>
#include <stdexcept>

#include "error.h"

namespace vx {

Uuid Uuid::generate() {
    static thread_local std::mt19937_64 rng{std::random_device{}()};
    Uuid id;
    auto* p = id.bytes.data();
    auto a = rng();
    auto b = rng();
    for (int i = 0; i < 8; ++i) p[i]     = static_cast<uint8_t>((a >> (i * 8)) & 0xFF);
    for (int i = 0; i < 8; ++i) p[8 + i] = static_cast<uint8_t>((b >> (i * 8)) & 0xFF);
    // Per RFC 4122 v4: set version and variant bits.
    p[6] = static_cast<uint8_t>((p[6] & 0x0F) | 0x40);
    p[8] = static_cast<uint8_t>((p[8] & 0x3F) | 0x80);
    return id;
}

namespace {
constexpr char hex[] = "0123456789abcdef";
}

std::string Uuid::toString() const {
    std::string s(36, '-');
    auto write_byte = [&](size_t out, uint8_t b) {
        s[out]     = hex[(b >> 4) & 0x0F];
        s[out + 1] = hex[b & 0x0F];
    };
    size_t out = 0;
    for (size_t i = 0; i < 16; ++i) {
        if (i == 4 || i == 6 || i == 8 || i == 10) ++out;
        write_byte(out, bytes[i]);
        out += 2;
    }
    return s;
}

Uuid Uuid::fromString(const std::string& s) {
    if (s.size() != 36) throw Error(VX_ERR_INVALID_ARG, "UUID must be 36 chars");
    auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    Uuid id;
    size_t in = 0;
    for (size_t i = 0; i < 16; ++i) {
        if (in == 8 || in == 13 || in == 18 || in == 23) {
            if (s[in] != '-') throw Error(VX_ERR_INVALID_ARG, "UUID format");
            ++in;
        }
        int hi = nibble(s[in]);
        int lo = nibble(s[in + 1]);
        if (hi < 0 || lo < 0) throw Error(VX_ERR_INVALID_ARG, "UUID hex");
        id.bytes[i] = static_cast<uint8_t>((hi << 4) | lo);
        in += 2;
    }
    return id;
}

}  // namespace vx
