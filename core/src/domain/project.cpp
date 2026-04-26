#include "project.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <utility>

#include "util/error.h"
#include "vx/version.h"

namespace vx {

namespace {

struct JsonValue {
    enum class Type {
        Object,
        String,
        Number
    };

    Type type = Type::String;
    std::map<std::string, JsonValue> object;
    std::string string;
    int64_t number = 0;
};

class JsonParser {
public:
    explicit JsonParser(std::string input) : input_(std::move(input)) {}

    JsonValue parse() {
        JsonValue value = parseValue();
        skipWhitespace();
        if (pos_ != input_.size()) {
            throw Error(VX_ERR_INVALID_ARG, "unexpected trailing JSON data");
        }
        return value;
    }

private:
    [[nodiscard]] char peek() const {
        if (pos_ >= input_.size()) {
            return '\0';
        }
        return input_[pos_];
    }

    char consume() {
        if (pos_ >= input_.size()) {
            throw Error(VX_ERR_INVALID_ARG, "unexpected end of JSON");
        }
        return input_[pos_++];
    }

    void expect(char expected) {
        const char actual = consume();
        if (actual != expected) {
            throw Error(VX_ERR_INVALID_ARG, "unexpected JSON character");
        }
    }

    void skipWhitespace() {
        while (pos_ < input_.size() &&
               std::isspace(static_cast<unsigned char>(input_[pos_])) != 0) {
            ++pos_;
        }
    }

    JsonValue parseValue() {
        skipWhitespace();
        const char c = peek();
        if (c == '{') {
            return parseObject();
        }
        if (c == '"') {
            JsonValue value;
            value.type = JsonValue::Type::String;
            value.string = parseString();
            return value;
        }
        if (c == '-' || (c >= '0' && c <= '9')) {
            JsonValue value;
            value.type = JsonValue::Type::Number;
            value.number = parseNumber();
            return value;
        }
        throw Error(VX_ERR_INVALID_ARG, "unsupported JSON value");
    }

    JsonValue parseObject() {
        JsonValue value;
        value.type = JsonValue::Type::Object;
        expect('{');
        skipWhitespace();
        if (peek() == '}') {
            consume();
            return value;
        }

        while (true) {
            skipWhitespace();
            const std::string key = parseString();
            skipWhitespace();
            expect(':');
            value.object.emplace(key, parseValue());
            skipWhitespace();
            const char next = consume();
            if (next == '}') {
                return value;
            }
            if (next != ',') {
                throw Error(VX_ERR_INVALID_ARG, "expected JSON object separator");
            }
        }
    }

    std::string parseString() {
        expect('"');
        std::string out;
        while (true) {
            const char c = consume();
            if (c == '"') {
                return out;
            }
            if (c != '\\') {
                out.push_back(c);
                continue;
            }

            const char escaped = consume();
            switch (escaped) {
                case '"':
                case '\\':
                case '/':
                    out.push_back(escaped);
                    break;
                case 'b':
                    out.push_back('\b');
                    break;
                case 'f':
                    out.push_back('\f');
                    break;
                case 'n':
                    out.push_back('\n');
                    break;
                case 'r':
                    out.push_back('\r');
                    break;
                case 't':
                    out.push_back('\t');
                    break;
                default:
                    throw Error(VX_ERR_UNSUPPORTED, "unsupported JSON string escape");
            }
        }
    }

    int64_t parseNumber() {
        const size_t start = pos_;
        if (peek() == '-') {
            consume();
        }
        if (peek() < '0' || peek() > '9') {
            throw Error(VX_ERR_INVALID_ARG, "invalid JSON number");
        }
        while (peek() >= '0' && peek() <= '9') {
            consume();
        }

        const std::string token = input_.substr(start, pos_ - start);
        char* end = nullptr;
        const long long parsed = std::strtoll(token.c_str(), &end, 10);
        if (end == nullptr || *end != '\0') {
            throw Error(VX_ERR_INVALID_ARG, "invalid JSON integer");
        }
        return static_cast<int64_t>(parsed);
    }

    std::string input_;
    size_t pos_ = 0;
};

[[nodiscard]] const JsonValue& requireKey(const JsonValue& object, const char* key) {
    if (object.type != JsonValue::Type::Object) {
        throw Error(VX_ERR_INVALID_ARG, "expected JSON object");
    }
    const auto it = object.object.find(std::string{key});
    if (it == object.object.end()) {
        throw Error(VX_ERR_INVALID_ARG, std::string{"missing project JSON key: "} + key);
    }
    return it->second;
}

[[nodiscard]] int64_t requireNumber(const JsonValue& object, const char* key) {
    const JsonValue& value = requireKey(object, key);
    if (value.type != JsonValue::Type::Number) {
        throw Error(VX_ERR_INVALID_ARG, std::string{"expected JSON number: "} + key);
    }
    return value.number;
}

[[nodiscard]] std::string requireString(const JsonValue& object, const char* key) {
    const JsonValue& value = requireKey(object, key);
    if (value.type != JsonValue::Type::String) {
        throw Error(VX_ERR_INVALID_ARG, std::string{"expected JSON string: "} + key);
    }
    return value.string;
}

[[nodiscard]] int checkedInt(int64_t value, const std::string& field) {
    if (value < static_cast<int64_t>(std::numeric_limits<int>::min()) ||
        value > static_cast<int64_t>(std::numeric_limits<int>::max())) {
        throw Error(VX_ERR_INVALID_ARG, "integer field out of range: " + field);
    }
    return static_cast<int>(value);
}

[[nodiscard]] std::string escapeJsonString(const std::string& value) {
    std::ostringstream out;
    for (const char c : value) {
        switch (c) {
            case '"':
                out << "\\\"";
                break;
            case '\\':
                out << "\\\\";
                break;
            case '\b':
                out << "\\b";
                break;
            case '\f':
                out << "\\f";
                break;
            case '\n':
                out << "\\n";
                break;
            case '\r':
                out << "\\r";
                break;
            case '\t':
                out << "\\t";
                break;
            default:
                out << c;
                break;
        }
    }
    return out.str();
}

[[nodiscard]] int64_t toUnixMillis(std::chrono::system_clock::time_point time) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch()).count();
}

[[nodiscard]] std::chrono::system_clock::time_point fromUnixMillis(int64_t millis) {
    return std::chrono::system_clock::time_point{std::chrono::milliseconds{millis}};
}

[[nodiscard]] const char* colorSpaceToString(ColorSpace color_space) {
    switch (color_space) {
        case ColorSpace::Rec709:
            return "rec709";
        case ColorSpace::Rec2020_PQ:
            return "rec2020_pq";
        case ColorSpace::Rec2020_HLG:
            return "rec2020_hlg";
        case ColorSpace::DisplayP3:
            return "display_p3";
        case ColorSpace::SRGB:
            return "srgb";
    }
    return "rec709";
}

[[nodiscard]] ColorSpace colorSpaceFromString(const std::string& value) {
    if (value == "rec709") {
        return ColorSpace::Rec709;
    }
    if (value == "rec2020_pq") {
        return ColorSpace::Rec2020_PQ;
    }
    if (value == "rec2020_hlg") {
        return ColorSpace::Rec2020_HLG;
    }
    if (value == "display_p3") {
        return ColorSpace::DisplayP3;
    }
    if (value == "srgb") {
        return ColorSpace::SRGB;
    }
    throw Error(VX_ERR_INVALID_ARG, "unknown color space: " + value);
}

}  // namespace

std::string projectToJson(const Project& p) {
    std::ostringstream out;
    out << R"({"schema_version":)" << p.schema_version
        << R"(,"id":")" << p.id.toString()
        << R"(","name":")" << escapeJsonString(p.name)
        << R"(","created_at_ms":)" << toUnixMillis(p.created_at)
        << R"(,"modified_at_ms":)" << toUnixMillis(p.modified_at)
        << R"(,"canvas":{"width":)" << p.canvas.width
        << R"(,"height":)" << p.canvas.height
        << R"(},"framerate":{"num":)" << p.framerate.num
        << R"(,"den":)" << p.framerate.den
        << R"(},"color_space":")" << colorSpaceToString(p.color_space)
        << R"("})";
    return out.str();
}

Project projectFromJson(const std::string& json) {
    const JsonValue root = JsonParser{json}.parse();

    Project project;
    project.schema_version = checkedInt(requireNumber(root, "schema_version"), "schema_version");
    if (project.schema_version > VX_SCHEMA_VERSION_CURRENT) {
        throw Error(VX_ERR_UNSUPPORTED, "project schema version is newer than this engine");
    }
    if (project.schema_version < 1) {
        throw Error(VX_ERR_INVALID_ARG, "project schema version is invalid");
    }

    project.id = Uuid::fromString(requireString(root, "id"));
    project.name = requireString(root, "name");
    project.created_at = fromUnixMillis(requireNumber(root, "created_at_ms"));
    project.modified_at = fromUnixMillis(requireNumber(root, "modified_at_ms"));

    const JsonValue& canvas = requireKey(root, "canvas");
    project.canvas.width = checkedInt(requireNumber(canvas, "width"), "canvas.width");
    project.canvas.height = checkedInt(requireNumber(canvas, "height"), "canvas.height");
    if (project.canvas.width <= 0 || project.canvas.height <= 0) {
        throw Error(VX_ERR_INVALID_ARG, "canvas dimensions must be positive");
    }

    const JsonValue& framerate = requireKey(root, "framerate");
    project.framerate.num = requireNumber(framerate, "num");
    project.framerate.den = requireNumber(framerate, "den");
    if (project.framerate.num <= 0 || project.framerate.den <= 0) {
        throw Error(VX_ERR_INVALID_ARG, "framerate must be positive");
    }
    project.framerate = project.framerate.reduced();
    project.color_space = colorSpaceFromString(requireString(root, "color_space"));
    return project;
}

}  // namespace vx
