#pragma once

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-type-check.hpp"
#include "beatsaber-hook/shared/utils/utils-functions.h"
#include <string_view>
#include <string>

struct StringW {
    // Dynamically allocated string
    StringW(Il2CppString *str) noexcept : inst1(str) { inst2 = to_utf8(csstrtostr(inst1)); }
    StringW(const std::string &str) noexcept : inst2(str) { inst1 = il2cpp_utils::newcsstr(inst2); }
    StringW(const char *str) noexcept : inst2(str) { inst1 = il2cpp_utils::newcsstr(inst2); }

    constexpr operator bool() const noexcept {
        return inst1 != nullptr;
    }

    constexpr bool operator ==(std::nullptr_t rhs) const noexcept {
        return inst1 == rhs;
    }

    template<typename T>
    requires (std::is_constructible_v<std::u16string_view, T> || std::is_constructible_v<std::string_view, T> || std::is_same_v<T, StringW>)
    StringW operator +(T const& rhs) const noexcept {
        if constexpr (std::is_same_v<T, StringW>) return inst2 + rhs.inst2;
        else return inst2 + std::string(rhs);
    }

    bool operator ==(StringW const& rhs) const noexcept {
        return inst2 == rhs.inst2;
    }

    bool operator ==(std::string const& rhs) const noexcept {
        return inst2 == rhs;
    }

    bool operator ==(Il2CppString* const& rhs) const noexcept {
        return inst1->Equals(rhs);
    }

    operator Il2CppString*() const { return inst1; }
    operator std::string() const { return inst2; }

    private:
    Il2CppString* inst1;
    std::string inst2;
};

template<typename T>
requires (!std::is_constructible_v<T, StringW> && (std::is_constructible_v<std::u16string_view, T> || std::is_constructible_v<std::string_view, T>))
StringW operator +(T const lhs, StringW const& rhs) noexcept {
    return lhs + std::string(rhs);
}

typedef StringW ConstString;
