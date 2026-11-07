#pragma once
#include "ssm/expression.hpp"
#include <optional>

namespace ssm {
enum class EquationStatus { Unique, NoSolution, Identity, Unsupported };
enum class EquationStepKind {
  DomainRestriction,
  ConditionalDenominatorClearing,
  ExactLinearClassification,
  ExactSubstitution
};
struct EquationStep {
  EquationStepKind kind;
  std::string explanation;
};
struct LinearEquationResult {
  EquationStatus status = EquationStatus::Unsupported;
  std::optional<Rational> solution;
  std::vector<Expr> nonzero_conditions;
  std::vector<Rational> cleared_coefficients; // ascending powers, zero = {0}
  std::vector<EquationStep> steps;
  std::string explanation;
};
// Bounded rational-expression equation; cleared polynomial must be affine.
// Scope and resource failures return Unsupported; malformed Expr construction
// and allocation failures retain their existing exception behavior.
LinearEquationResult solve_linear_equation(const Expr &left, const Expr &right,
                                           std::string_view variable);
} // namespace ssm
