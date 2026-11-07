#include "ssm/linear_equation.hpp"
#include <algorithm>

namespace ssm {
namespace {
using Poly = std::vector<Rational>;
struct OutsideScope : MathError {
  using MathError::MathError;
};
struct EmptyDomain : MathError {
  Expr condition;
  explicit EmptyDomain(const Expr &e)
      : MathError("Required condition " + print(e) +
                  " != 0 is identically zero"),
        condition(e) {}
};
struct Fraction {
  Poly numerator, denominator;
};
bool zero(const Poly &p) { return p.size() == 1 && p[0].zero(); }
void trim(Poly &p) {
  while (p.size() > 1 && p.back().zero())
    p.pop_back();
}
class Builder {
  std::string variable;
  unsigned nodes = 0, work = 0;
  void operation() {
    if (++work > 4096)
      throw OutsideScope("Coefficient work budget exceeded");
  }
  void digits(const Rational &v, std::size_t maximum = 512) {
    if (v.numerator().str().size() > maximum ||
        v.denominator().str().size() > maximum)
      throw OutsideScope("Coefficient digit budget exceeded");
  }
  Poly add(Poly a, const Poly &b, bool subtract = false) {
    a.resize(std::max(a.size(), b.size()));
    for (std::size_t i = 0; i < b.size(); ++i) {
      operation();
      a[i] = subtract ? a[i] - b[i] : a[i] + b[i];
      digits(a[i]);
    }
    trim(a);
    return a;
  }
  Poly multiply(const Poly &a, const Poly &b) {
    if (zero(a) || zero(b))
      return {Rational(0)};
    if (a.size() + b.size() - 2 > 4)
      throw OutsideScope("Intermediate polynomial degree exceeds 4");
    Poly p(a.size() + b.size() - 1);
    for (std::size_t i = 0; i < a.size(); ++i)
      for (std::size_t j = 0; j < b.size(); ++j) {
        operation();
        p[i + j] = p[i + j] + a[i] * b[j];
        digits(p[i + j]);
      }
    trim(p);
    return p;
  }
  Poly power(Poly a, unsigned n) {
    Poly out{Rational(1)};
    while (n--)
      out = multiply(out, a);
    return out;
  }
  void condition(const Expr &e, const Poly &numerator) {
    if (zero(numerator))
      throw EmptyDomain(e);
    if (conditions.size() >= 64)
      throw OutsideScope("Original-domain condition budget exceeded");
    if (std::none_of(conditions.begin(), conditions.end(),
                     [&](const Expr &x) { return structurally_equal(x, e); }))
      conditions.push_back(e);
  }

public:
  std::vector<Expr> conditions;
  explicit Builder(std::string_view name) {
    if (name.empty() || name.size() > 32)
      throw OutsideScope("Variable must be 1..32 ASCII identifier characters");
    auto symbol = Expr::symbol(std::string(name));
    (void)symbol;
    for (unsigned char c : name)
      if (c > 127)
        throw OutsideScope("Use an ASCII variable name");
    variable = std::string(name);
  }
  Fraction build(const Expr &e, unsigned depth = 0) {
    if (++nodes > 256 || depth > 32)
      throw OutsideScope("Equation tree budget exceeded");
    using K = Kind;
    if (e.kind() == K::Constant) {
      digits(e.value(), 128);
      return {{e.value()}, {Rational(1)}};
    }
    if (e.kind() == K::Symbol) {
      if (e.name() != variable)
        throw OutsideScope(
            "Parameters and additional variables are unsupported");
      return {{Rational(0), Rational(1)}, {Rational(1)}};
    }
    auto a = build(e.left(), depth + 1);
    if (e.kind() == K::Negate) {
      for (auto &v : a.numerator)
        v = -v;
      return a;
    }
    if (e.kind() == K::Power) {
      const auto exponent = e.right();
      if (++nodes > 256 || exponent.kind() != K::Constant ||
          !exponent.value().integral() ||
          exponent.value().numerator() < Integer(-4) ||
          exponent.value().numerator() > Integer(4))
        throw OutsideScope("Use direct integer powers from -4 to 4");
      const int n = std::stoi(exponent.value().numerator().str());
      if (n <= 0)
        condition(e.left(), a.numerator);
      if (n == 0)
        return {{Rational(1)}, {Rational(1)}};
      if (n < 0)
        std::swap(a.numerator, a.denominator);
      const auto magnitude = static_cast<unsigned>(n < 0 ? -n : n);
      return {power(a.numerator, magnitude), power(a.denominator, magnitude)};
    }
    auto b = build(e.right(), depth + 1);
    if (e.kind() == K::Multiply)
      return {multiply(a.numerator, b.numerator),
              multiply(a.denominator, b.denominator)};
    if (e.kind() == K::Divide) {
      condition(e.right(), b.numerator);
      return {multiply(a.numerator, b.denominator),
              multiply(a.denominator, b.numerator)};
    }
    return {add(multiply(a.numerator, b.denominator),
                multiply(b.numerator, a.denominator), e.kind() == K::Subtract),
            multiply(a.denominator, b.denominator)};
  }
  Poly clear(const Fraction &a, const Fraction &b) {
    return add(multiply(a.numerator, b.denominator),
               multiply(b.numerator, a.denominator), true);
  }
};
} // namespace
LinearEquationResult solve_linear_equation(const Expr &left, const Expr &right,
                                           std::string_view variable) {
  LinearEquationResult result;
  try {
    Builder builder(variable);
    // Evaluation order is explicit: retain left-side conditions before right.
    const auto a = builder.build(left);
    const auto b = builder.build(right);
    result.nonzero_conditions = builder.conditions;
    for (const auto &e : result.nonzero_conditions)
      result.steps.push_back({EquationStepKind::DomainRestriction,
                              print(e) + " != 0 in the original expression"});
    result.cleared_coefficients = builder.clear(a, b);
    result.steps.push_back({EquationStepKind::ConditionalDenominatorClearing,
                            "Cross-multiplied numerator polynomial = 0 only "
                            "where all original nonzero conditions hold"});
    if (result.cleared_coefficients.size() > 2) {
      result.explanation = "Cleared equation is nonlinear; no factor "
                           "cancellation or nonlinear solving is attempted";
      return result;
    }
    if (result.cleared_coefficients.size() == 1) {
      result.status = result.cleared_coefficients[0].zero()
                          ? EquationStatus::Identity
                          : EquationStatus::NoSolution;
      result.explanation =
          result.status == EquationStatus::Identity
              ? "Identity on the original admissible real domain"
              : "Nonzero constant cannot equal zero";
      result.steps.push_back(
          {EquationStepKind::ExactLinearClassification, result.explanation});
      return result;
    }
    const auto candidate =
        -result.cleared_coefficients[0] / result.cleared_coefficients[1];
    if (candidate.numerator().str().size() > 512 ||
        candidate.denominator().str().size() > 512)
      throw OutsideScope("Candidate digit budget exceeded");
    const std::map<std::string, Rational> binding{
        {std::string(variable), candidate}};
    result.steps.push_back({EquationStepKind::ExactLinearClassification,
                            "Unique affine candidate: " + candidate.str()});
    bool admissible = true;
    try {
      for (const auto &e : result.nonzero_conditions)
        if (evaluate(e, binding).zero())
          admissible = false;
      if (admissible && !(evaluate(left, binding) == evaluate(right, binding)))
        throw OutsideScope(
            "Exact candidate substitution failed; no solution certified");
    } catch (const OutsideScope &) {
      throw;
    } catch (const MathError &) {
      admissible = false;
    }
    result.status =
        admissible ? EquationStatus::Unique : EquationStatus::NoSolution;
    if (admissible)
      result.solution = candidate;
    result.explanation =
        admissible
            ? "Candidate satisfies both original sides and all domain "
              "conditions exactly"
            : "The sole affine candidate is excluded by the original domain";
    result.steps.push_back(
        {EquationStepKind::ExactSubstitution, result.explanation});
  } catch (const EmptyDomain &e) {
    result.status = EquationStatus::NoSolution;
    result.explanation = e.what();
    result.nonzero_conditions.push_back(e.condition);
    result.steps.push_back({EquationStepKind::DomainRestriction,
                            "Original admissible domain is empty"});
  } catch (const MathError &e) {
    result.status = EquationStatus::Unsupported;
    result.solution.reset();
    result.explanation = e.what();
  }
  return result;
}
} // namespace ssm
