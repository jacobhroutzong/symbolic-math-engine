#include "ssm/trace.hpp"
#include <gtest/gtest.h>
using namespace ssm;
TEST(Trace, ChecksAggregateStepAndRejectsFalseRewrite) {
  auto trace = simplify_with_trace(parse("(x+0)*1"));
  ASSERT_EQ(trace.verification, Equivalence::Equal);
  ASSERT_EQ(trace.steps.size(), 1u);
  EXPECT_EQ(print(trace.result), "x");
  EXPECT_EQ(check_polynomial_step(trace.steps[0].before, trace.steps[0].after),
            Equivalence::Equal);
  EXPECT_EQ(check_polynomial_step(parse("x+1"), parse("x+2")),
            Equivalence::Different);
  EXPECT_TRUE(simplify_with_trace(parse("x")).steps.empty());
}
TEST(Trace, RefusesPartialDomainAndResourceClaims) {
  for (auto text : {"x/x", "0*(1/x)", "x^0", "x^-1", "x^17"}) {
    auto original = parse(text);
    auto trace = simplify_with_trace(original);
    EXPECT_EQ(trace.verification, Equivalence::Unknown);
    EXPECT_TRUE(trace.steps.empty());
    EXPECT_TRUE(structurally_equal(trace.result, original));
  }
  EXPECT_EQ(check_polynomial_step(parse("x/x"), parse("1")),
            Equivalence::Unknown);
  EXPECT_EQ(check_polynomial_step(parse("(x+1)^2"), parse("x*x+2*x+1")),
            Equivalence::Equal);
  auto growing = Expr::symbol("x");
  for (int i = 0; i < 4; ++i)
    growing = Expr::binary(Kind::Power, growing, Expr(Rational(16)));
  EXPECT_EQ(simplify_with_trace(growing).verification, Equivalence::Unknown);
}
