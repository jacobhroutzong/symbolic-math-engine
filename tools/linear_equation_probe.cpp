#include "ssm/linear_equation.hpp"
#include <iostream>
#include <sstream>
int main() {
  std::string line;
  while (std::getline(std::cin, line)) {
    try {
      std::istringstream in(line);
      std::string lhs, rhs, name;
      if (!std::getline(in, lhs, '\t') || !std::getline(in, rhs, '\t') ||
          !std::getline(in, name) || name.find('\t') != std::string::npos)
        return 2;
      auto result =
          ssm::solve_linear_equation(ssm::parse(lhs), ssm::parse(rhs), name);
      const char *status =
          result.status == ssm::EquationStatus::Unique       ? "unique"
          : result.status == ssm::EquationStatus::Identity   ? "identity"
          : result.status == ssm::EquationStatus::NoSolution ? "none"
                                                             : "unsupported";
      std::cout << status << '\t'
                << (result.solution ? result.solution->str() : "-") << '\t';
      for (std::size_t i = 0; i < result.nonzero_conditions.size(); ++i) {
        if (i)
          std::cout << ';';
        std::cout << ssm::print(result.nonzero_conditions[i]);
      }
      std::cout << '\t';
      for (std::size_t i = 0; i < result.cleared_coefficients.size(); ++i) {
        if (i)
          std::cout << ';';
        std::cout << result.cleared_coefficients[i].str();
      }
      std::cout << '\t' << result.explanation << '\n';
    } catch (const std::exception &e) {
      std::cout << "parse_error\t-\t\t\t" << e.what() << '\n';
    }
  }
}
