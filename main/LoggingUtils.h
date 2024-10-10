#ifndef LPRD_MCU_LOGGINGUTILS_H
#define LPRD_MCU_LOGGINGUTILS_H

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <utility>
#include <Arduino.h>
#include <IPAddress.h>

template<>
struct fmt::formatter<String> {
    constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const String &s, FormatContext &ctx) {
        return fmt::format_to(ctx.out(), "{}", s.c_str());
    }
};

template<>
struct fmt::formatter<IPAddress> {
    constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const IPAddress &ip, FormatContext &ctx) {
        return fmt::format_to(
                ctx.out(),
                "{}.{}.{}.{}",
                ip[0], // First octet
                ip[1], // Second octet
                ip[2], // Third octet
                ip[3]  // Fourth octet
        );
    }
};


#endif //LPRD_MCU_LOGGINGUTILS_H
