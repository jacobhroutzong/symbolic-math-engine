#include "ssm/expression.hpp"
#include <algorithm>
#include <cctype>
#include <functional>
#include <set>
#include <utility>
namespace ssm {
struct Expr::Node {
  Kind kind;
  Rational value;
  std::string name;
  std::shared_ptr<const Node> left, right;
  int height = 1;
};
Expr::Expr(std::shared_ptr<const Node> n) : node_(std::move(n)) {}
Expr::Expr(Rational v)
    : node_(std::make_shared<const Node>(
          Node{Kind::Constant, std::move(v), {}, nullptr, nullptr})) {}
Expr Expr::symbol(std::string n) {
  if (n.empty() ||
      !(std::isalpha(static_cast<unsigned char>(n[0])) || n[0] == '_') ||
      !std::all_of(n.begin(), n.end(),
                   [](unsigned char c) { return std::isalnum(c) || c == '_'; }))
    throw MathError("Invalid symbol");
  return Expr(std::make_shared<const Node>(
      Node{Kind::Symbol, {}, std::move(n), nullptr, nullptr}));
}
Expr Expr::unary(Expr a) {
  if (a.kind() == Kind::Constant)
    return Expr(-a.value());
  int h = a.node_->height + 1;
  if (h > 128)
    throw MathError("AST height limit is 128");
  return Expr(std::make_shared<const Node>(
      Node{Kind::Negate, {}, {}, a.node_, nullptr, h}));
}
Expr Expr::binary(Kind k, Expr a, Expr b) {
  if (k < Kind::Add || k > Kind::Power)
    throw MathError("Invalid binary operator");
  if (k == Kind::Divide && a.kind() == Kind::Constant &&
      b.kind() == Kind::Constant)
    return Expr(a.value() / b.value());
  int h = 1 + std::max(a.node_->height, b.node_->height);
  if (h > 128)
    throw MathError("AST height limit is 128");
  return Expr(
      std::make_shared<const Node>(Node{k, {}, {}, a.node_, b.node_, h}));
}
Kind Expr::kind() const { return node_->kind; }
const Rational &Expr::value() const {
  if (kind() != Kind::Constant)
    throw MathError("Expression is not constant");
  return node_->value;
}
const std::string &Expr::name() const {
  if (kind() != Kind::Symbol)
    throw MathError("Expression is not a symbol");
  return node_->name;
}
Expr Expr::left() const {
  if (!node_->left)
    throw MathError("No child");
  return Expr(node_->left);
}
Expr Expr::right() const {
  if (!node_->right)
    throw MathError("No right child");
  return Expr(node_->right);
}
ParseError::ParseError(std::string m, std::size_t p)
    : std::invalid_argument(std::move(m) + " at byte " + std::to_string(p)),
      position_(p) {}
namespace {
using K = Kind;
bool constant(const Expr &e, int v) {
  return e.kind() == K::Constant && e.value() == Rational(v);
}
int exponent(const Expr &e) {
  auto s = simplify(e);
  if (s.kind() != K::Constant || !s.value().integral())
    throw MathError("Only constant integer powers are supported here");
  auto n = s.value().numerator();
  if (n < Integer(-1024) || n > Integer(1024))
    throw MathError("Exponent limit is 1024");
  return std::stoi(n.str());
}
class Parser {
  std::string_view input;
  std::size_t pos = 0, depth = 0;
  void space() {
    while (pos < input.size() &&
           std::isspace(static_cast<unsigned char>(input[pos])))
      ++pos;
  }
  bool take(char c) {
    space();
    if (pos < input.size() && input[pos] == c) {
      ++pos;
      return true;
    }
    return false;
  }
  Expr sum() {
    auto e = product();
    for (;;) {
      if (take('+'))
        e = Expr::binary(K::Add, e, product());
      else if (take('-'))
        e = Expr::binary(K::Subtract, e, product());
      else
        return e;
    }
  }
  Expr product() {
    auto e = unary();
    for (;;) {
      if (take('*'))
        e = Expr::binary(K::Multiply, e, unary());
      else if (take('/'))
        e = Expr::binary(K::Divide, e, unary());
      else
        return e;
    }
  }
  Expr unary() {
    if (++depth > 128)
      throw ParseError("Expression nesting limit", pos);
    Expr e;
    if (take('+'))
      e = unary();
    else if (take('-'))
      e = Expr::unary(unary());
    else {
      e = primary();
      if (take('^'))
        e = Expr::binary(K::Power, e, unary());
    }
    --depth;
    return e;
  }
  Expr primary() {
    space();
    auto start = pos;
    if (take('(')) {
      auto e = sum();
      if (!take(')'))
        throw ParseError("Expected closing parenthesis", pos);
      return e;
    }
    if (pos < input.size() &&
        std::isdigit(static_cast<unsigned char>(input[pos]))) {
      while (pos < input.size() &&
             std::isdigit(static_cast<unsigned char>(input[pos])))
        ++pos;
      return Expr(Rational(Integer(input.substr(start, pos - start))));
    }
    if (pos < input.size() &&
        (std::isalpha(static_cast<unsigned char>(input[pos])) ||
         input[pos] == '_')) {
      while (pos < input.size() &&
             (std::isalnum(static_cast<unsigned char>(input[pos])) ||
              input[pos] == '_'))
        ++pos;
      return Expr::symbol(std::string(input.substr(start, pos - start)));
    }
    throw ParseError("Expected integer, symbol or parenthesis", pos);
  }

public:
  explicit Parser(std::string_view s) : input(s) {
    if (s.size() > 4096)
      throw ParseError("Input length limit is 4096 bytes", 4096);
  }
  Expr run() {
    auto e = sum();
    space();
    if (pos != input.size())
      throw ParseError(
          "Unexpected token (implicit multiplication is unsupported)", pos);
    return e;
  }
};
bool total(const Expr &e) {
  switch (e.kind()) {
  case K::Constant:
  case K::Symbol:
    return true;
  case K::Negate:
    return total(e.left());
  case K::Add:
  case K::Subtract:
  case K::Multiply:
    return total(e.left()) && total(e.right());
  case K::Divide:
    return e.right().kind() == K::Constant && !e.right().value().zero() &&
           total(e.left());
  case K::Power:
    try {
      return exponent(e.right()) > 0 && total(e.left());
    } catch (const MathError &) {
      return false;
    }
  }
  return false;
}
Expr simplify_impl(const Expr &e, int depth) {
  if (depth > 128)
    throw MathError("Simplification depth limit");
  if (e.kind() == K::Constant || e.kind() == K::Symbol)
    return e;
  auto a = simplify_impl(e.left(), depth + 1);
  if (e.kind() == K::Negate) {
    if (a.kind() == K::Negate)
      return a.left();
    return Expr::unary(a);
  }
  auto b = simplify_impl(e.right(), depth + 1);
  auto k = e.kind();
  if (a.kind() == K::Constant && b.kind() == K::Constant) {
    const auto &x = a.value();
    const auto &y = b.value();
    switch (k) {
    case K::Add:
      return Expr(x + y);
    case K::Subtract:
      return Expr(x - y);
    case K::Multiply:
      return Expr(x * y);
    case K::Divide:
      return Expr(x / y);
    case K::Power:
      return Expr(power(x, exponent(b)));
    default:
      break;
    }
  }
  if (k == K::Divide && constant(b, 0))
    throw MathError("Zero denominator");
  if (k == K::Add) {
    if (constant(a, 0))
      return b;
    if (constant(b, 0))
      return a;
  }
  if (k == K::Subtract) {
    if (constant(b, 0))
      return a;
    if (structurally_equal(a, b) && total(a))
      return Expr();
  }
  if (k == K::Multiply) {
    if (constant(a, 1))
      return b;
    if (constant(b, 1))
      return a;
    if ((constant(a, 0) && total(b)) || (constant(b, 0) && total(a)))
      return Expr();
  }
  if (k == K::Divide && constant(b, 1))
    return a;
  if (k == K::Power && constant(b, 1))
    return a;
  return Expr::binary(k, a, b);
}
Rational evaluate_impl(const Expr &e, const std::map<std::string, Rational> &m,
                       int depth) {
  if (depth > 128)
    throw MathError("Evaluation depth limit");
  if (e.kind() == K::Constant)
    return e.value();
  if (e.kind() == K::Symbol) {
    auto i = m.find(e.name());
    if (i == m.end())
      throw MathError("Unbound symbol: " + e.name());
    return i->second;
  }
  auto a = evaluate_impl(e.left(), m, depth + 1);
  if (e.kind() == K::Negate)
    return -a;
  auto b = evaluate_impl(e.right(), m, depth + 1);
  switch (e.kind()) {
  case K::Add:
    return a + b;
  case K::Subtract:
    return a - b;
  case K::Multiply:
    return a * b;
  case K::Divide:
    return a / b;
  case K::Power:
    return power(a, exponent(Expr(b)));
  default:
    throw MathError("Invalid expression");
  }
}
std::string print_impl(const Expr &e, bool tex, int depth) {
  if (depth > 128)
    throw MathError("Printing depth limit");
  if (e.kind() == K::Constant) {
    const auto &v = e.value();
    if (v.integral())
      return v.numerator() < Integer(0) ? "(" + v.str() + ")" : v.str();
    return tex ? "\\frac{" + v.numerator().str() + "}{" +
                     v.denominator().str() + "}"
               : "(" + v.str() + ")";
  }
  if (e.kind() == K::Symbol) {
    if (!tex)
      return e.name();
    std::string escaped;
    for (char c : e.name()) {
      if (c == '_')
        escaped += "\\";
      escaped += c;
    }
    return "\\mathrm{" + escaped + "}";
  }
  auto a = print_impl(e.left(), tex, depth + 1);
  if (e.kind() == K::Negate)
    return "(-" + a + ")";
  auto b = print_impl(e.right(), tex, depth + 1);
  if (tex && e.kind() == K::Divide)
    return "\\frac{" + a + "}{" + b + "}";
  if (tex && e.kind() == K::Power)
    return "{" + a + "}^{" + b + "}";
  auto op = e.kind() == K::Add        ? "+"
            : e.kind() == K::Subtract ? "-"
            : e.kind() == K::Multiply ? (tex ? "\\cdot " : "*")
            : e.kind() == K::Divide   ? "/"
                                      : "^";
  return "(" + a + op + b + ")";
}
bool equal_impl(const Expr &a, const Expr &b, int depth) {
  if (depth > 128)
    throw MathError("Equality depth limit");
  if (a.kind() != b.kind())
    return false;
  if (a.kind() == K::Constant)
    return a.value() == b.value();
  if (a.kind() == K::Symbol)
    return a.name() == b.name();
  return equal_impl(a.left(), b.left(), depth + 1) &&
         (a.kind() == K::Negate || equal_impl(a.right(), b.right(), depth + 1));
}
} // namespace
Expr parse(std::string_view s) { return Parser(s).run(); }
std::string print(const Expr &e) { return print_impl(e, false, 0); }
std::string latex(const Expr &e) { return print_impl(e, true, 0); }
bool structurally_equal(const Expr &a, const Expr &b) {
  return equal_impl(a, b, 0);
}
Expr simplify(const Expr &e) { return simplify_impl(e, 0); }
Rational evaluate(const Expr &e, const std::map<std::string, Rational> &m) {
  return evaluate_impl(e, m, 0);
}
Derivative differentiate(const Expr &input, std::string_view variable) {
  Expr::symbol(std::string(variable)); // Validate differentiation variable.
  std::vector<Expr> conditions;
  std::set<std::string> seen;
  int budget = 8192;
  auto require_nonzero = [&](const Expr &e) {
    auto s = simplify(e);
    if (s.kind() == K::Constant) {
      if (s.value().zero())
        throw MathError("Undefined original domain");
      return;
    }
    if (seen.insert(print(s)).second)
      conditions.push_back(s);
  };
  std::function<void(const Expr &, int)> domain = [&](const Expr &e,
                                                      int depth) {
    if (depth > 128)
      throw MathError("Differentiation depth limit");
    if (e.kind() == K::Constant || e.kind() == K::Symbol)
      return;
    domain(e.left(), depth + 1);
    if (e.kind() == K::Negate)
      return;
    domain(e.right(), depth + 1);
    if (e.kind() == K::Divide)
      require_nonzero(e.right());
    if (e.kind() == K::Power && exponent(e.right()) <= 0)
      require_nonzero(e.left());
  };
  domain(input, 0);
  auto op = [](K k, Expr a, Expr b) { return Expr::binary(k, a, b); };
  std::function<Expr(const Expr &, int)> diff = [&](const Expr &e,
                                                    int depth) -> Expr {
    if (--budget < 0 || depth > 128)
      throw MathError("Differentiation resource limit");
    if (e.kind() == K::Constant)
      return Expr();
    if (e.kind() == K::Symbol)
      return Expr(Rational(e.name() == variable ? 1 : 0));
    auto a = e.left(), da = diff(a, depth + 1);
    if (e.kind() == K::Negate)
      return Expr::unary(da);
    auto b = e.right();
    if (e.kind() == K::Power) {
      int n = exponent(b);
      if (n == 0)
        return Expr();
      if (n == 1)
        return da; // Do not introduce x^0 and an artificial hole at x=0.
      if (n == -1024)
        throw MathError("Derivative exponent would exceed the supported limit");
      return op(K::Multiply,
                op(K::Multiply, Expr(Rational(n)),
                   op(K::Power, a, Expr(Rational(n - 1)))),
                da);
    }
    auto db = diff(b, depth + 1);
    switch (e.kind()) {
    case K::Add:
    case K::Subtract:
      return op(e.kind(), da, db);
    case K::Multiply:
      return op(K::Add, op(K::Multiply, da, b), op(K::Multiply, a, db));
    case K::Divide:
      return op(K::Divide,
                op(K::Subtract, op(K::Multiply, da, b), op(K::Multiply, a, db)),
                op(K::Power, b, Expr(Rational(2))));
    default:
      throw MathError("Unsupported derivative");
    }
  };
  return {simplify(diff(input, 0)), std::move(conditions)};
}
namespace {
using Monomial = std::vector<std::string>;
using Polynomial = std::map<Monomial, Rational>;
void add_term(Polynomial &p, const Monomial &m, const Rational &v) {
  auto sum = (p.contains(m) ? p.at(m) : Rational()) + v;
  if (sum.zero())
    p.erase(m);
  else
    p[m] = sum;
  if (p.size() > 1024)
    throw MathError("Polynomial term limit");
}
Polynomial multiply(const Polynomial &a, const Polynomial &b, int &budget) {
  Polynomial r;
  for (const auto &[ma, va] : a)
    for (const auto &[mb, vb] : b) {
      if (--budget < 0)
        throw MathError("Polynomial expansion limit");
      auto m = ma;
      m.insert(m.end(), mb.begin(), mb.end());
      if (m.size() > 128)
        throw MathError("Polynomial degree limit");
      std::sort(m.begin(), m.end());
      add_term(r, m, va * vb);
    }
  return r;
}
Polynomial polynomial(const Expr &e, int depth, int &budget) {
  if (depth > 128 || --budget < 0)
    throw MathError("Polynomial resource limit");
  if (e.kind() == K::Constant) {
    Polynomial r;
    add_term(r, {}, e.value());
    return r;
  }
  if (e.kind() == K::Symbol)
    return {{{e.name()}, Rational(1)}};
  auto a = polynomial(e.left(), depth + 1, budget);
  if (e.kind() == K::Negate) {
    for (auto &[m, v] : a)
      v = -v;
    return a;
  }
  if (e.kind() == K::Power) {
    int n = exponent(e.right());
    if (n < 1 || n > 16)
      throw MathError("Outside polynomial power scope");
    Polynomial r{{{}, Rational(1)}};
    for (int i = 0; i < n; ++i)
      r = multiply(r, a, budget);
    return r;
  }
  auto b = polynomial(e.right(), depth + 1, budget);
  if (e.kind() == K::Multiply)
    return multiply(a, b, budget);
  if (e.kind() == K::Divide) {
    auto d = simplify(e.right());
    if (d.kind() != K::Constant || d.value().zero())
      throw MathError("Partial rational function");
    for (auto &[m, v] : a)
      v = v / d.value();
    return a;
  }
  for (const auto &[m, v] : b)
    add_term(a, m, e.kind() == K::Subtract ? -v : v);
  return a;
}
} // namespace
Equivalence polynomial_equivalence(const Expr &a, const Expr &b) {
  try {
    int budget = 32768;
    auto pa = polynomial(simplify(a), 0, budget),
         pb = polynomial(simplify(b), 0, budget);
    return pa == pb ? Equivalence::Equal : Equivalence::Different;
  } catch (const MathError &) {
    return Equivalence::Unknown;
  }
}
Equivalence polynomial_equivalence_raw(const Expr &a, const Expr &b) {
  try {
    int budget = 32768;
    auto pa = polynomial(a, 0, budget), pb = polynomial(b, 0, budget);
    return pa == pb ? Equivalence::Equal : Equivalence::Different;
  } catch (const MathError &) {
    return Equivalence::Unknown;
  }
}
} // namespace ssm
