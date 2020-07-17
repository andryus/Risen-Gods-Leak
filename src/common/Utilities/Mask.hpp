#pragma once

#include <type_traits>

namespace util
{

template<typename Flag>
class Mask
{
    using Base = typename std::underlying_type<Flag>::type;

public:
    constexpr Mask() : mask_{} {}
    constexpr Mask(const Flag& flag) : mask_{static_cast<Base>(flag)} {}
    constexpr Mask(const Mask& mask) = default;
    constexpr explicit Mask(const Base& base) : mask_{base} {}

    constexpr Mask operator|(const Mask& other) const { return Mask{static_cast<Base>(this->mask_ | other.mask_)}; }
    constexpr Mask operator+(const Mask& other) const { return Mask{static_cast<Base>(this->mask_ | other.mask_)}; }
    constexpr Mask operator&(const Mask& other) const { return Mask{static_cast<Base>(this->mask_ & other.mask_)}; }
    constexpr Mask operator-(const Mask& other) const { return Mask{static_cast<Base>(this->mask_ & ~other.mask_)}; }
    constexpr Mask operator^(const Mask& other) const { return Mask{static_cast<Base>(this->mask_ ^ other.mask_)}; }
    constexpr Mask operator~() const { return Mask{static_cast<Base>(~this->mask_)}; }

    Mask& operator|=(const Mask& other) { this->mask_ |= other.mask_; return *this; }
    Mask& operator+=(const Mask& other) { this->mask_ |= other.mask_; return *this; }
    Mask& operator&=(const Mask& other) { this->mask_ &= other.mask_; return *this; }
    Mask& operator-=(const Mask& other) { this->mask_ &= ~other.mask_; return *this; }
    Mask& operator^=(const Mask& other) { this->mask_ ^= other.mask_; return *this; }

    constexpr explicit operator bool() const { return mask_ != 0; }
    constexpr operator Base() const { return mask_; }

private:
    Base mask_;
};

}

#define ADD_FLAG_OPERATORS(FLAG) \
    constexpr ::util::Mask<FLAG> operator|(const FLAG& a, const FLAG& b) { return ::util::Mask<FLAG>{a} | ::util::Mask<FLAG>{b}; } \
    constexpr ::util::Mask<FLAG> operator+(const FLAG& a, const FLAG& b) { return ::util::Mask<FLAG>{a} + ::util::Mask<FLAG>{b}; }
