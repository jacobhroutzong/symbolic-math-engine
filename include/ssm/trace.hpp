#pragma once
#include "ssm/expression.hpp"
namespace ssm {
struct TransformationStep {
  Expr before, after;
  std::string rule;
  Equivalence verification;
};
struct SimplificationTrace {
  Expr result;
  std::vector<TransformationStep> steps;
  Equivalence verification;
};
// Checks an authored rewrite with the bounded exact polynomial checker.
// Partial-domain expressions (including x^0) return Unknown conservatively.
Equivalence check_polynomial_step(const Expr &before, const Expr &after);
// One aggregate simplification step, independently checked via normalization.
// Unsupported domains/budgets return the original expression, no certified
// step.
SimplificationTrace simplify_with_trace(const Expr &expression);
} // namespace ssm
