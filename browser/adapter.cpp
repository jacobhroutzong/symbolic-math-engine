#include "ssm/expression.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <emscripten/bind.h>

namespace {
std::string quote(const std::string &s) {
  std::string r = "\"";
  for (unsigned char c : s) {
    if (c == '"' || c == '\\')
      r += '\\';
    if (c >= 32)
      r += static_cast<char>(c);
  }
  return r + '"';
}
int inspect(const ssm::Expr &e, int &nodes, int depth = 0) {
  using K = ssm::Kind;
  if (++nodes > 64 || depth > 32)
    throw ssm::MathError("Browser limit: 64 expression nodes and depth 32");
  if (e.kind() == K::Symbol)
    return 1;
  if (e.kind() == K::Constant)
    return static_cast<int>(e.value().numerator().str().size() +
                            e.value().denominator().str().size());
  int a = inspect(e.left(), nodes, depth + 1);
  if (e.kind() == K::Negate)
    return a;
  int b = inspect(e.right(), nodes, depth + 1), cost = a + b + 1;
  if (e.kind() == K::Power) {
    auto p = e.right();
    if (p.kind() != K::Constant || !p.value().integral() ||
        p.value().numerator() < ssm::Integer(-16) ||
        p.value().numerator() > ssm::Integer(16))
      throw ssm::MathError(
          "Browser powers must be direct integer constants from -16 to 16");
    cost = a * std::max(1, std::abs(std::stoi(p.value().numerator().str())));
  }
  if (cost > 512)
    throw ssm::MathError(
        "Expression exceeds the browser arithmetic-growth budget");
  return cost;
}
std::string differentiate(const std::string &expression,
                          const std::string &variable) {
  try {
    if (expression.size() > 256 || variable.empty() || variable.size() > 32)
      throw ssm::MathError(
          "Browser limits: expression 256 bytes, variable 1–32 bytes");
    int digits = 0;
    for (unsigned char c : expression) {
      if (c > 127)
        throw ssm::MathError(
            "Use ASCII mathematical operators and symbol names");
      digits = std::isdigit(c) ? digits + 1 : 0;
      if (digits > 64)
        throw ssm::MathError(
            "Integer literals are limited to 64 digits in the browser");
    }
    auto e = ssm::parse(expression);
    int nodes = 0;
    inspect(e, nodes);
    auto d = ssm::differentiate(e, variable);
    auto canonical = ssm::print(e), derivative = ssm::print(d.expression),
         latex = ssm::latex(d.expression);
    if (canonical.size() + derivative.size() + latex.size() > 65536)
      throw ssm::MathError("Result exceeds the browser output budget");
    std::string out = "{\"ok\":true,\"canonical\":" + quote(canonical) +
                      ",\"derivative\":" + quote(derivative) +
                      ",\"latex\":" + quote(latex) + ",\"conditions\":[";
    for (std::size_t i = 0; i < d.nonzero_conditions.size(); ++i) {
      if (i)
        out += ',';
      out += quote(ssm::print(d.nonzero_conditions[i]));
    }
    return out + "]}";
  } catch (const std::exception &e) {
    return "{\"ok\":false,\"error\":" + quote(e.what()) + "}";
  }
}
} // namespace
EMSCRIPTEN_BINDINGS(symbolic_engine) {
  emscripten::function("differentiate", &differentiate);
}
