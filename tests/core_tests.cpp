#include "ssm/expression.hpp"
#include <gtest/gtest.h>
#include <limits>
#include <random>
using namespace ssm;
TEST(Integer, LargeSignedArithmeticAndDivision) {
  Integer a("123456789012345678901234567890"), b("98765432109876543210");
  EXPECT_EQ((a + b).str(), "123456789111111111011111111100");
  EXPECT_EQ((a * b).str(),
            "12193263113702179522496570642237463801111263526900");
  EXPECT_EQ((a / b) * b + a % b, a);
  EXPECT_EQ(((-a) / b) * b + (-a) % b, -a);
  EXPECT_EQ(((-Integer(7)) / Integer(3)).str(), "-2");
  EXPECT_EQ(((-Integer(7)) % Integer(3)).str(), "-1");
  EXPECT_EQ(Integer(std::numeric_limits<std::int64_t>::min()).str(),
            "-9223372036854775808");
  EXPECT_EQ(Integer("-0000"), Integer());
  EXPECT_THROW(Integer("12x"), std::invalid_argument);
  EXPECT_THROW(a / Integer(), std::domain_error);
}
TEST(Integer, SeededAlgebraicIdentities) {
  std::mt19937 generator(20261026);
  for (int i = 0; i < 150; ++i) {
    Integer a(static_cast<int>(generator() % 1000000) - 500000),
        b(static_cast<int>(generator() % 1000000) + 1),
        c(static_cast<int>(generator() % 1000));
    EXPECT_EQ((a + b) - b, a);
    EXPECT_EQ(a * (b + c), a * b + a * c);
    EXPECT_EQ((a / b) * b + a % b, a);
    EXPECT_LT((a % b).abs(), b);
  }
}
TEST(Rational, NormalizationAndExactIdentities) {
  EXPECT_EQ(Rational(Integer(6), Integer(-8)).str(), "-3/4");
  EXPECT_EQ(Rational(0, Integer(-123)).str(), "0");
  EXPECT_EQ((Rational(1, 3) + Rational(1, 6)).str(), "1/2");
  Rational x(Integer("9999999999999999999999999999999999999999"), Integer(7));
  EXPECT_EQ((x / Rational(3, 5)) * Rational(3, 5), x);
  EXPECT_EQ(power(Rational(2, 3), -3), Rational(27, 8));
  EXPECT_THROW(Rational(1, 0), MathError);
  EXPECT_THROW(power(Rational(0), 0), MathError);
  EXPECT_THROW(power(Rational(0), -1), MathError);
}
TEST(Parser, PrecedenceAssociativityAndRoundTrip) {
  EXPECT_EQ(evaluate(parse("-2^2"), {}), Rational(-4));
  EXPECT_EQ(evaluate(parse("2^3^2"), {}), Rational(512));
  EXPECT_EQ(evaluate(parse("8/4/2"), {}), Rational(1));
  EXPECT_EQ(evaluate(parse("2^-3"), {}), Rational(1, 8));
  for (auto s : {"x + 2*y^3", "-(x-y)/(x+1)", "-7/11", "x^-2", "a_b+12", "--x",
                 "x^(2^3)", "(-2)^2", "(-2)^x", "-5"}) {
    auto e = parse(s);
    EXPECT_TRUE(structurally_equal(e, parse(print(e)))) << s;
  }
}
TEST(Parser, InvalidExpressionsAndResourceLimits) {
  for (auto s :
       {"", "2x", "x+", "(x+1", "x**2", "x.5", "sin(x)", "x=2", "2^^3", "x,2"})
    EXPECT_THROW(parse(s), ParseError) << s;
  EXPECT_THROW(parse(std::string(4097, '1')), ParseError);
  EXPECT_THROW(parse(std::string(150, '(') + "x" + std::string(150, ')')),
               ParseError);
  EXPECT_THROW(parse("1/0"), MathError);
  EXPECT_THROW(evaluate(parse("missing"), {}), MathError);
}
TEST(Simplification, PreservesPartialDomains) {
  EXPECT_EQ(print(simplify(parse("x+0"))), "x");
  EXPECT_EQ(print(simplify(parse("2/3+1/6"))), "(5/6)");
  EXPECT_EQ(print(simplify(parse("0*x"))), "0");
  EXPECT_EQ(print(simplify(parse("x-x"))), "0");
  EXPECT_EQ(print(simplify(parse("x/x"))), "(x/x)");
  auto e = simplify(parse("0*(1/x)"));
  EXPECT_THROW(evaluate(e, {{"x", Rational(0)}}), MathError);
  auto power_zero = simplify(parse("x^0"));
  EXPECT_THROW(evaluate(power_zero, {{"x", Rational(0)}}), MathError);
  for (auto s : {"x+0", "1*(x+2)", "0*(1/x)", "(x+2)*(x-3)", "-(-x)"}) {
    auto a = simplify(parse(s));
    EXPECT_TRUE(structurally_equal(a, simplify(a)));
    for (int x = 1; x <= 5; ++x)
      EXPECT_EQ(evaluate(parse(s), {{"x", Rational(x)}}),
                evaluate(a, {{"x", Rational(x)}}));
  }
}
TEST(Differentiation, ExactRulesAndConditions) {
  EXPECT_EQ(evaluate(differentiate(parse("x^1"), "x").expression,
                     {{"x", Rational(0)}}), Rational(1));
  EXPECT_THROW(differentiate(parse("x^-1024"), "x"), MathError);
  auto d = differentiate(parse("(x+1)^3 + 2*x"), "x");
  EXPECT_EQ(evaluate(d.expression, {{"x", Rational(2)}}), Rational(29));
  EXPECT_TRUE(d.nonzero_conditions.empty());
  auto q = differentiate(parse("1/x"), "x");
  EXPECT_EQ(q.nonzero_conditions.size(), 1);
  EXPECT_EQ(evaluate(q.expression, {{"x", Rational(2)}}), Rational(-1, 4));
  auto z = differentiate(parse("x^0"), "x");
  EXPECT_EQ(print(z.expression), "0");
  ASSERT_EQ(z.nonzero_conditions.size(), 1);
  EXPECT_EQ(print(z.nonzero_conditions[0]), "x");
  EXPECT_THROW(differentiate(parse("x^x"), "x"), MathError);
  EXPECT_THROW(differentiate(parse("x^(1/2)"), "x"), MathError);
  auto multi = differentiate(parse("x*y+y^2"), "y");
  EXPECT_EQ(
      evaluate(multi.expression, {{"x", Rational(3)}, {"y", Rational(4)}}),
      Rational(11));
}
TEST(Equivalence, StructuralVersusPolynomialVersusAgreement) {
  auto a = parse("(x+1)^2"), b = parse("x^2+2*x+1");
  EXPECT_FALSE(structurally_equal(a, b));
  EXPECT_EQ(polynomial_equivalence(a, b), Equivalence::Equal);
  EXPECT_EQ(polynomial_equivalence(parse("x^2"), parse("x")),
            Equivalence::Different);
  EXPECT_EQ(evaluate(parse("x^2"), {{"x", Rational(1)}}),
            evaluate(parse("x"), {{"x", Rational(1)}}));
  EXPECT_EQ(polynomial_equivalence(parse("x/x"), parse("1")),
            Equivalence::Unknown);
  EXPECT_EQ(polynomial_equivalence(parse("x^17"), parse("x*x^16")),
            Equivalence::Unknown);
}
TEST(Serialization, LatexAndSymbolValidation) {
  EXPECT_EQ(latex(parse("1/2")), "\\frac{1}{2}");
  EXPECT_EQ(latex(parse("a_b")), "\\mathrm{a\\_b}");
  EXPECT_NE(latex(parse("x^2/(y+1)")).find("\\frac{"), std::string::npos);
  EXPECT_THROW(Expr::symbol("bad{symbol}"), MathError);
}
