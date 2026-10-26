#include "ssm/integer.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>
namespace ssm {
void Integer::normalize() {
  while (!limbs_.empty() && limbs_.back() == 0)
    limbs_.pop_back();
  if (zero())
    negative_ = false;
}
Integer::Integer(std::int64_t v) {
  negative_ = v < 0;
  auto u = v < 0 ? std::uint64_t(-(v + 1)) + 1 : std::uint64_t(v);
  while (u) {
    limbs_.push_back(static_cast<std::uint32_t>(u % base));
    u /= base;
  }
}
Integer::Integer(std::string_view s) {
  if (s.empty())
    throw std::invalid_argument("Empty integer");
  std::size_t first = 0;
  if (s[0] == '-' || s[0] == '+') {
    negative_ = s[0] == '-';
    first = 1;
  }
  if (first == s.size())
    throw std::invalid_argument("Integer needs digits");
  for (std::size_t i = first; i < s.size(); ++i)
    if (s[i] < '0' || s[i] > '9')
      throw std::invalid_argument("Invalid integer digit");
  for (std::size_t end = s.size(); end > first;) {
    auto start = end - first > 9 ? end - 9 : first;
    std::uint32_t v = 0;
    for (auto i = start; i < end; ++i)
      v = v * 10 + static_cast<unsigned>(s[i] - '0');
    limbs_.push_back(v);
    end = start;
  }
  normalize();
}
std::string Integer::str() const {
  if (zero())
    return "0";
  std::ostringstream out;
  if (negative_)
    out << '-';
  out << limbs_.back();
  for (std::size_t i = limbs_.size() - 1; i-- > 0;)
    out << std::setw(9) << std::setfill('0') << limbs_[i];
  return out.str();
}
Integer Integer::abs() const {
  Integer r = *this;
  r.negative_ = false;
  return r;
}
Integer Integer::operator-() const {
  Integer r = *this;
  if (!zero())
    r.negative_ = !negative_;
  return r;
}
int Integer::magnitude_compare(const Integer &a, const Integer &b) {
  if (a.limbs_.size() != b.limbs_.size())
    return a.limbs_.size() < b.limbs_.size() ? -1 : 1;
  for (auto i = a.limbs_.size(); i-- > 0;)
    if (a.limbs_[i] != b.limbs_[i])
      return a.limbs_[i] < b.limbs_[i] ? -1 : 1;
  return 0;
}
std::strong_ordering operator<=>(const Integer &a, const Integer &b) {
  if (a.negative_ != b.negative_)
    return a.negative_ ? std::strong_ordering::less
                       : std::strong_ordering::greater;
  int c = Integer::magnitude_compare(a, b);
  if (a.negative_)
    c = -c;
  return c < 0   ? std::strong_ordering::less
         : c > 0 ? std::strong_ordering::greater
                 : std::strong_ordering::equal;
}
Integer Integer::magnitude_add(const Integer &a, const Integer &b) {
  Integer r;
  std::uint64_t carry = 0;
  for (std::size_t i = 0;
       i < std::max(a.limbs_.size(), b.limbs_.size()) || carry; ++i) {
    auto v = carry + (i < a.limbs_.size() ? a.limbs_[i] : 0) +
             (i < b.limbs_.size() ? b.limbs_[i] : 0);
    r.limbs_.push_back(static_cast<std::uint32_t>(v % base));
    carry = v / base;
  }
  return r;
}
Integer Integer::magnitude_subtract(const Integer &a, const Integer &b) {
  Integer r;
  std::int64_t borrow = 0;
  for (std::size_t i = 0; i < a.limbs_.size(); ++i) {
    auto v = std::int64_t(a.limbs_[i]) -
             (i < b.limbs_.size() ? b.limbs_[i] : 0) - borrow;
    borrow = v < 0;
    if (borrow)
      v += base;
    r.limbs_.push_back(static_cast<std::uint32_t>(v));
  }
  r.normalize();
  return r;
}
Integer operator+(const Integer &a, const Integer &b) {
  Integer r;
  if (a.negative_ == b.negative_) {
    r = Integer::magnitude_add(a, b);
    r.negative_ = a.negative_;
  } else {
    int c = Integer::magnitude_compare(a, b);
    r = c >= 0 ? Integer::magnitude_subtract(a, b)
               : Integer::magnitude_subtract(b, a);
    r.negative_ = c >= 0 ? a.negative_ : b.negative_;
  }
  r.normalize();
  return r;
}
Integer operator-(const Integer &a, const Integer &b) { return a + (-b); }
Integer operator*(const Integer &a, const Integer &b) {
  Integer r;
  r.limbs_.resize(a.limbs_.size() + b.limbs_.size(), 0);
  for (std::size_t i = 0; i < a.limbs_.size(); ++i) {
    std::uint64_t carry = 0;
    for (std::size_t j = 0; j < b.limbs_.size() || carry; ++j) {
      auto v =
          std::uint64_t(r.limbs_[i + j]) + carry +
          (j < b.limbs_.size() ? std::uint64_t(a.limbs_[i]) * b.limbs_[j] : 0);
      r.limbs_[i + j] = static_cast<std::uint32_t>(v % Integer::base);
      carry = v / Integer::base;
    }
  }
  r.negative_ = a.negative_ != b.negative_;
  r.normalize();
  return r;
}
std::pair<Integer, Integer> Integer::divide(const Integer &a,
                                            const Integer &b) {
  if (b.zero())
    throw std::domain_error("Integer division by zero");
  Integer dividend = a.abs(), divisor = b.abs(), q, rem;
  q.limbs_.resize(dividend.limbs_.size());
  // Long division; binary search each base-10^9 quotient digit. No
  // machine-sized cap.
  for (auto i = dividend.limbs_.size(); i-- > 0;) {
    rem.limbs_.insert(rem.limbs_.begin(), dividend.limbs_[i]);
    rem.normalize();
    std::uint32_t lo = 0, hi = base - 1, best = 0;
    while (lo <= hi) {
      auto mid = lo + (hi - lo) / 2;
      Integer product = divisor * Integer(mid);
      if (magnitude_compare(product, rem) <= 0) {
        best = mid;
        lo = mid + 1;
      } else {
        if (mid == 0)
          break;
        hi = mid - 1;
      }
    }
    q.limbs_[i] = best;
    rem = magnitude_subtract(rem, divisor * Integer(best));
  }
  q.negative_ = a.negative_ != b.negative_;
  rem.negative_ = a.negative_;
  q.normalize();
  rem.normalize();
  return {q, rem};
}
Integer operator/(const Integer &a, const Integer &b) {
  return Integer::divide(a, b).first;
}
Integer operator%(const Integer &a, const Integer &b) {
  return Integer::divide(a, b).second;
}
Integer gcd(Integer a, Integer b) {
  a = a.abs();
  b = b.abs();
  while (!b.zero()) {
    auto r = a % b;
    a = b;
    b = r;
  }
  return a;
}
} // namespace ssm
