#include "ssm/trace.hpp"
namespace ssm {
namespace {
unsigned total_polynomial(const Expr &e, unsigned &nodes, unsigned depth = 0) {
  if (++nodes > 256 || depth > 64)
    return false;
  using K = Kind;
  if (e.kind() == K::Symbol)
    return 1;
  if (e.kind() == K::Constant) {
    const auto digits = e.value().numerator().str().size() +
                        e.value().denominator().str().size();
    return digits <= 512 ? static_cast<unsigned>(digits) : 0;
  }
  if (e.kind() == K::Negate)
    return total_polynomial(e.left(), nodes, depth + 1);
  if (e.kind() == K::Divide)
    return false;
  if (e.kind() == K::Power) {
    const auto p = e.right();
    if (p.kind() != K::Constant || !p.value().integral() ||
        p.value().numerator() < Integer(1) ||
        p.value().numerator() > Integer(16))
      return false;
    const auto cost = total_polynomial(e.left(), nodes, depth + 1);
    const auto growth =
        cost * static_cast<unsigned>(std::stoi(p.value().numerator().str()));
    return cost && growth <= 512 ? growth : 0;
  }
  const auto a = total_polynomial(e.left(), nodes, depth + 1),
             b = total_polynomial(e.right(), nodes, depth + 1);
  return a && b && a + b + 1 <= 512 ? a + b + 1 : 0;
}
} // namespace
Equivalence check_polynomial_step(const Expr &before, const Expr &after) {
  unsigned a = 0, b = 0;
  if (!total_polynomial(before, a) || !total_polynomial(after, b))
    return Equivalence::Unknown;
  return polynomial_equivalence_raw(before, after);
}
SimplificationTrace simplify_with_trace(const Expr &expression) {
  unsigned nodes = 0;
  if (!total_polynomial(expression, nodes))
    return {expression, {}, Equivalence::Unknown};
  auto result = simplify(expression);
  const auto verification = check_polynomial_step(expression, result);
  if (verification != Equivalence::Equal)
    return {expression, {}, verification};
  std::vector<TransformationStep> steps;
  if (!structurally_equal(expression, result))
    steps.push_back(
        {expression, result,
         "conservative simplification; exact polynomial identity checked",
         verification});
  return {result, std::move(steps), verification};
}
} // namespace ssm
