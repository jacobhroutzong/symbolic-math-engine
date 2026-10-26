#pragma once
#include "ssm/rational.hpp"
#include <map>
#include <memory>
#include <vector>
namespace ssm {
enum class Kind {
  Constant,
  Symbol,
  Negate,
  Add,
  Subtract,
  Multiply,
  Divide,
  Power
};
class Expr {
  struct Node;
  std::shared_ptr<const Node> node_;
  explicit Expr(std::shared_ptr<const Node> node);

public:
  Expr(Rational value = Rational());
  static Expr symbol(std::string name);
  static Expr unary(Expr child);
  static Expr binary(Kind kind, Expr left, Expr right);
  Kind kind() const;
  const Rational &value() const;
  const std::string &name() const;
  Expr left() const;
  Expr right() const;
};
class ParseError : public std::invalid_argument {
  std::size_t position_;

public:
  ParseError(std::string message, std::size_t position);
  std::size_t position() const { return position_; }
};
Expr parse(std::string_view input);
std::string print(const Expr &);
std::string latex(const Expr &);
bool structurally_equal(const Expr &, const Expr &);
Expr simplify(const Expr &);
Rational evaluate(const Expr &,
                  const std::map<std::string, Rational> &bindings);
struct Derivative {
  Expr expression;
  std::vector<Expr> nonzero_conditions;
};
// Integer powers only, |exponent|<=1024; carries original-domain conditions.
Derivative differentiate(const Expr &, std::string_view variable);
enum class Equivalence { Equal, Different, Unknown };
// Exact polynomial identity, <=1024 expanded terms and powers <=16.
// Partial rational functions outside this domain return Unknown.
Equivalence polynomial_equivalence(const Expr &, const Expr &);
} // namespace ssm
