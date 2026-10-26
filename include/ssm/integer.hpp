#pragma once
#include <compare>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
namespace ssm {
// Independently implemented signed, dynamically sized base-10^9 integer.
class Integer {
  static constexpr std::uint32_t base = 1000000000;
  std::vector<std::uint32_t> limbs_;
  bool negative_ = false;
  void normalize();
  static int magnitude_compare(const Integer &, const Integer &);
  static Integer magnitude_add(const Integer &, const Integer &);
  static Integer magnitude_subtract(const Integer &, const Integer &);
  static std::pair<Integer, Integer> divide(const Integer &, const Integer &);

public:
  Integer(std::int64_t value = 0);
  explicit Integer(std::string_view decimal);
  std::string str() const;
  bool zero() const { return limbs_.empty(); }
  Integer abs() const;
  Integer operator-() const;
  friend bool operator==(const Integer &, const Integer &) = default;
  friend std::strong_ordering operator<=>(const Integer &, const Integer &);
  friend Integer operator+(const Integer &, const Integer &);
  friend Integer operator-(const Integer &, const Integer &);
  friend Integer operator*(const Integer &, const Integer &);
  friend Integer operator/(const Integer &, const Integer &);
  friend Integer operator%(const Integer &, const Integer &);
};
Integer gcd(Integer a, Integer b);
} // namespace ssm
