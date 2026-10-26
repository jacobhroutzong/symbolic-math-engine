#pragma once
#include "ssm/integer.hpp"
#include <stdexcept>
namespace ssm {
class MathError : public std::domain_error {
public:
  using std::domain_error::domain_error;
};
class Rational {
  Integer numerator_, denominator_;

public:
  Rational(Integer numerator = 0, Integer denominator = 1);
  const Integer &numerator() const { return numerator_; }
  const Integer &denominator() const { return denominator_; }
  bool zero() const { return numerator_.zero(); }
  bool integral() const { return denominator_ == Integer(1); }
  std::string str() const;
  Rational operator-() const { return Rational(-numerator_, denominator_); }
  friend bool operator==(const Rational &, const Rational &) = default;
  friend Rational operator+(const Rational &, const Rational &);
  friend Rational operator-(const Rational &, const Rational &);
  friend Rational operator*(const Rational &, const Rational &);
  friend Rational operator/(const Rational &, const Rational &);
};
Rational power(Rational value, int exponent);
} // namespace ssm
