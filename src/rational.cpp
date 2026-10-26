#include "ssm/rational.hpp"
namespace ssm {
Rational::Rational(Integer n, Integer d) : numerator_(n), denominator_(d) {
  if (d.zero())
    throw MathError("Zero rational denominator");
  if (d < Integer(0)) {
    numerator_ = -n;
    denominator_ = -d;
  }
  auto g = gcd(numerator_, denominator_);
  numerator_ = numerator_ / g;
  denominator_ = denominator_ / g;
}
std::string Rational::str() const {
  return integral() ? numerator_.str()
                    : numerator_.str() + "/" + denominator_.str();
}
Rational operator+(const Rational &a, const Rational &b) {
  return Rational(a.numerator_ * b.denominator_ + b.numerator_ * a.denominator_,
                  a.denominator_ * b.denominator_);
}
Rational operator-(const Rational &a, const Rational &b) { return a + (-b); }
Rational operator*(const Rational &a, const Rational &b) {
  auto g = gcd(a.numerator_, b.denominator_),
       h = gcd(b.numerator_, a.denominator_);
  return Rational((a.numerator_ / g) * (b.numerator_ / h),
                  (a.denominator_ / h) * (b.denominator_ / g));
}
Rational operator/(const Rational &a, const Rational &b) {
  if (b.zero())
    throw MathError("Division by zero");
  return a * Rational(b.denominator_, b.numerator_);
}
Rational power(Rational v, int e) {
  if (e < -1024 || e > 1024)
    throw MathError("Exponent limit is 1024");
  if (v.zero() && e <= 0)
    throw MathError("Zero to nonpositive power is undefined");
  if (e < 0) {
    v = Rational(1) / v;
    e = -e;
  }
  Rational r(1);
  while (e) {
    if (e & 1)
      r = r * v;
    e /= 2;
    if (e)
      v = v * v;
  }
  return r;
}
} // namespace ssm
