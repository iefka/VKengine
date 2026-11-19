#pragma once
#include<array>
#include<cstddef>
#include<cstdint>
#include<type_traits>
#include<utility>

namespace de::details {
    template<typename T>
    concept Uint32EnumClass = std::is_scoped_enum_v<T>
        && std::is_same_v<std::underlying_type_t<T>, std::uint32_t>;
}
namespace de{
    template<details::Uint32EnumClass Slot, std::size_t N>
    consteval bool areConsecutiveFromZero(const std::array<Slot, N>& slots) { 
        for (uint32_t i = 0; i < slots.size(); ++i)
            if (std::to_underlying(slots[i]) != i) return false;
        return true;
    }
}