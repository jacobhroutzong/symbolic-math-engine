#include "ssm/linear_equation.hpp"
#include <gtest/gtest.h>
#include <tuple>
using namespace ssm;
TEST(LinearEquation, ExactAffineAndRationalRoots) {
  for (const auto &[lhs, rhs, root] :
       std::vector<std::tuple<std::string, std::string, std::string>>{
           {"2*(x+1)", "7", "5/2"},
           {"(2*x+1)/(x-3)", "0", "-1/2"},
           {"x^-1", "2", "1/2"},
           {"x/3+1/2", "2", "9/2"}}) {
    auto a = parse(lhs), b = parse(rhs);
    auto r = solve_linear_equation(a, b, "x");
    ASSERT_EQ(r.status, EquationStatus::Unique) << r.explanation;
    ASSERT_TRUE(r.solution);
    EXPECT_EQ(r.solution->str(), root);
    EXPECT_EQ(evaluate(a, {{"x", *r.solution}}),
              evaluate(b, {{"x", *r.solution}}));
    EXPECT_EQ(r.steps.back().kind, EquationStepKind::ExactSubstitution);
  }
}
TEST(LinearEquation, RetainsDomainHolesAndConditionalTraces) {
  for (auto text : {"x/x", "x^0", "0*(1/x)+1"}) {
    auto r = solve_linear_equation(parse(text), parse("1"), "x");
    EXPECT_EQ(r.status, EquationStatus::Identity);
    ASSERT_FALSE(r.nonzero_conditions.empty());
    EXPECT_EQ(r.steps.front().kind, EquationStepKind::DomainRestriction);
    EXPECT_EQ(print(r.nonzero_conditions[0]), "x");
  }
  for (auto text : {"x/x", "(x-1)/(x-1)"}) {
    auto r = solve_linear_equation(parse(text), parse("0"), "x");
    EXPECT_EQ(r.status, EquationStatus::NoSolution);
    EXPECT_FALSE(r.solution);
    EXPECT_FALSE(r.nonzero_conditions.empty());
  }
  auto empty = solve_linear_equation(parse("1/(x-x)"), parse("0"), "x");
  EXPECT_EQ(empty.status, EquationStatus::NoSolution);
  EXPECT_NE(empty.explanation.find("identically zero"), std::string::npos);
}
TEST(LinearEquation, DegenerateAndUnsupportedAreDistinct) {
  EXPECT_EQ(solve_linear_equation(parse("x+1"), parse("x+2"), "x").status,
            EquationStatus::NoSolution);
  EXPECT_EQ(solve_linear_equation(parse("x-x"), parse("0"), "x").status,
            EquationStatus::Identity);
  for (auto text : {"x*x", "x^5", "x^x", "y*x", "x^2/x"}) {
    auto r = solve_linear_equation(parse(text), parse("1"), "x");
    EXPECT_EQ(r.status, EquationStatus::Unsupported);
    EXPECT_FALSE(r.solution);
    EXPECT_FALSE(r.explanation.empty());
  }
  EXPECT_EQ(solve_linear_equation(parse("x"), parse("1"), "bad-name").status,
            EquationStatus::Unsupported);
}
TEST(LinearEquation, ResourceBudgetsRejectWithoutClaimingNoSolutions) {
  EXPECT_EQ(solve_linear_equation(parse(std::string(129, '9') + "*x"),
                                  parse("1"), "x")
                .status,
            EquationStatus::Unsupported);
  auto e = Expr::symbol("x");
  for (int i = 0; i < 40; ++i)
    e = Expr::binary(Kind::Add, e, Expr(Rational(1)));
  EXPECT_EQ(solve_linear_equation(e, Expr(Rational(0)), "x").status,
            EquationStatus::Unsupported);
}
