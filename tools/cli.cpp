#include "ssm/expression.hpp"
#include <iostream>
#include <sstream>
using namespace ssm;
// A local batch adapter. No code generation, subprocesses or network access.
int main() {
  std::string line;
  while (std::getline(std::cin, line)) {
    try {
      std::vector<std::string> fields;
      std::istringstream in(line);
      std::string field;
      while (std::getline(in, field, '\t'))
        fields.push_back(field);
      if (fields.size() != 3)
        throw MathError("Expected operation TAB expression TAB argument");
      const auto &command = fields[0];
      std::string output;
      if (command == "integer") {
        Integer a(fields[1]), b(fields[2]);
        output = (a + b).str() + "\t" + (a - b).str() + "\t" + (a * b).str() +
                 "\t" + (a / b).str() + "\t" + (a % b).str() + "\t" +
                 gcd(a, b).str();
      } else if (command == "rational") {
        auto a = evaluate(parse(fields[1]), {}),
             b = evaluate(parse(fields[2]), {});
        output = (a + b).str() + "\t" + (a - b).str() + "\t" + (a * b).str() +
                 "\t" + (a / b).str();
      } else if (command == "derive") {
        auto e = parse(fields[1]);
        auto d = differentiate(e, fields[2]);
        output = print(simplify(e)) + "\t" + print(d.expression) + "\t";
        for (std::size_t i = 0; i < d.nonzero_conditions.size(); ++i) {
          if (i)
            output += ';';
          output += print(d.nonzero_conditions[i]);
        }
        output += '\t';
        output += latex(e);
      } else if (command == "eval")
        output =
            evaluate(parse(fields[1]), {{"x", evaluate(parse(fields[2]), {})}})
                .str();
      else if (command == "equiv") {
        auto result =
            polynomial_equivalence(parse(fields[1]), parse(fields[2]));
        output = result == Equivalence::Equal       ? "equal"
                 : result == Equivalence::Different ? "different"
                                                    : "unknown";
      } else
        throw MathError("Unknown operation");
      std::cout << "ok\t" << output << '\n';
    } catch (const std::exception &e) {
      std::cout << "error\t" << e.what() << '\n';
    }
  }
}
