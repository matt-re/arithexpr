#include <charconv>
#include <locale>
#include <iostream>
#include <optional>
#include <span>
#include <stack>
#include <string>
#include <string_view>
#include <vector>

std::optional<int> evaluate(std::span<const std::string_view> tokens)
{
	std::stack<int> stack;
	for (std::string_view token : tokens) {
		if (token == "+" || token == "-" || token == "*" || token == "/") {
			if (stack.size() < 2) {
				return std::nullopt;
			}

			int b = stack.top();
			stack.pop();
			int a = stack.top();
			stack.pop();

			switch (token[0]) {
			case '+':
				stack.push(a + b);
				break;
			case '-':
				stack.push(a - b);
				break;
			case '*':
				stack.push(a * b);
				break;
			case '/':
				if (b == 0) {
					return std::nullopt;
				}
				stack.push(a / b);
				break;
			}
		} else {
			int value;
			const char* end = token.data() + token.size();
			auto [ptr, ec] = std::from_chars(token.data(), end, value);
			if (ec != std::errc() || ptr != end) {
				return std::nullopt;
			}
			stack.push(value);
		}
	}

	if (stack.size() != 1) {
		return std::nullopt;
	}
	return stack.top();
}

std::optional<std::vector<std::string_view>> postfix_expr_from_infix(std::span<const std::string_view> infix_expr)
{
	// Use the Shunting Yard algorithm to convert an infix expression to postfix expression
	std::vector<std::string_view> postfix_expr;
	std::stack<std::string_view> stack;
	for (std::string_view token : infix_expr) {
		if (token == "(") {
			stack.push(token);
		} else if (token == ")") {
			// On closing parenthesis copy all operators to result
			while (!stack.empty() && stack.top() != "(") {
				postfix_expr.push_back(stack.top());
				stack.pop();
			}
			// Remove the opening parenthesis
			if (stack.empty()) {
				return std::nullopt;
			}
			stack.pop();
		} else if (token == "+" || token == "-" || token == "*" || token == "/") {
			// For this app all operators have the same precedence, as shown in the given spec document,
			// therefore copy all operators pushed so far to result
			// To support operators with different precedence then only pop operators with higher, or
			// same precedence and left associative
			while (!stack.empty() && stack.top() != "(") {
				postfix_expr.push_back(stack.top());
				stack.pop();
			}
			stack.push(token);
		} else {
			// Copy number straight to result
			postfix_expr.push_back(token);
		}
	}
	// Any remaining operators copy to result
	while (!stack.empty()) {
		if (stack.top() == "(") {
			return std::nullopt;
		}
		postfix_expr.push_back(stack.top());
		stack.pop();
	}
	return postfix_expr;
}

std::optional<std::vector<std::string_view>> tokenize(std::string_view expr)
{
	std::vector<std::string_view> tokens;
	const std::locale& loc = std::locale::classic();
	std::size_t i = 0;
	while (i < expr.size()) {
		// Find the start and end of each word by ignoring whitespace
		while (i < expr.size() && std::isspace(expr[i], loc)) {
			i++;
		}
		const std::size_t start = i;
		while (i < expr.size() && !std::isspace(expr[i], loc)) {
			i++;
		}
		const std::size_t end = i;

		// The word between start and end may contain a combination of numbers, operators and parentheses
		// that need to be split
		std::size_t cur = start;
		while (cur < end) {
			const std::size_t beg = cur;
			cur++;

			const bool prev_is_operator = tokens.empty()
				|| tokens.back() == "+" || tokens.back() == "-"
				|| tokens.back() == "*" || tokens.back() == "/"
				|| tokens.back() == "(";

			const bool is_number =
				std::isdigit(expr[beg], loc)
				|| (expr[beg] == '-' && (beg + 1 < end) && std::isdigit(expr[beg + 1], loc) && prev_is_operator);

			if (is_number) {
				while (cur < end && std::isdigit(expr[cur], loc)) {
					cur++;
				}
			}

			tokens.push_back(expr.substr(beg, cur - beg));
		}
	}

	if (tokens.empty()) {
		return std::nullopt;
	}
	return tokens;
}

bool evaluate(const char* expression, int& result)
{
	if (!expression) {
		return false;
	}

	std::optional<std::vector<std::string_view>> tokens = tokenize(expression);
	if (!tokens) {
		return false;
	}
	// Transform the infix expression to a postfix expression to make the evaluation easier
	// by removing the parentheses and having explicit operator ordering
	std::optional<std::vector<std::string_view>> expr = postfix_expr_from_infix(*tokens);
	if (!expr) {
		return false;
	}
	std::optional<int> value = evaluate(*expr);
	if (!value) {
		return false;
	}
	result = *value;
	return true;
}

bool run_evaluate_tests()
{
	bool success = true;

	std::pair<const char*, int> inputs[] = {
		{ "1 + 3 * 4",          16 },
		{ "1 + 3",               4 },
		{ "(1 + 3) * 2",         8 },
		{ "(4 / 2) + 6",         8 },
		{ "4 + (12 / (1 * 2))", 10 },
		{ "4+(12/(1*2))",       10 },
		{ "-1",                 -1 },
		{ "-1 * 2",             -2 },
		{ "-1+2",                1 },
		{ "1--2",                3 },
	};
	for (const auto& [expr, expected_result] : inputs) {
		int result;
		bool evaluated = evaluate(expr, result);
		if (evaluated && result == expected_result) {
			std::cerr << "Passed Evaluate " << expr << "\n";
		} else if (evaluated) {
			std::cerr << "Failed Evaluate " << expr << " Actual Result: " << result << "\n";
			success = false;
		} else {
			std::cerr << "Failed Evaluate " << expr << " invalid expression\n";
			success = false;
		}
	}

	const char* bad_exprs[] = {
		"(1 + (12 * 2)",
		"1 / 0",
		"1 + + 2",
		"+ 1",
		"1)",
		"1(",
		nullptr,
	};
	for (const auto& expr : bad_exprs) {
		int result;
		if (!evaluate(expr, result)) {
			if (expr) {
				std::cerr << "Passed Evaluate Bad Expression " << expr << "\n";
			} else {
				std::cerr << "Passed Evaluate Bad Expression NULL\n";
			}
		} else {
			std::cerr << "Failed Evaluate Bad Expression " << expr << "\n";
			success = false;
		}
	}

	return success;
}

bool run_postfix_from_infix_tests()
{
	bool success = true;
	std::pair<std::string, std::vector<std::string_view>> inputs[] = {
		{ "1 + 3 * 4",          { "1", "3", "+", "4", "*" } },
		{ "1 + 3",              { "1", "3", "+" } },
		{ "(1 + 3) * 2",        { "1", "3", "+", "2", "*" } },
		{ "(4 / 2) + 6",        { "4", "2", "/", "6", "+" } },
		{ "4 + (12 / (1 * 2))", { "4", "12", "1", "2", "*", "/", "+" } },
		{ "4+(12/(1*2))",       { "4", "12", "1", "2", "*", "/", "+" } },
	};
	for (const auto& [expr, expected_result] : inputs) {
		const auto tokens = tokenize(expr);
		if (!tokens) {
			std::cerr << "Failed Infix to Postfix " << expr << " Cannot Tokenize\n";
			success = false;
			continue;
		}

		const auto result = postfix_expr_from_infix(*tokens);
		if (!result) {
			std::cerr << "Failed Infix to Postfix " << expr << " Invalid Expression\n";
			success = false;
		} else if (*result == expected_result) {
			std::cerr << "Passed Infix to Postfix " << expr << "\n";
		} else {
			std::cerr << "Failed Infix to Postfix " << expr << " Actual Result: ";
			for (const auto& x : *result) {
				std::cerr << x << ", ";
			}
			std::cerr << "\n";
			success = false;
		}
	}

	std::string bad_exprs[] = {
		"(1 + (12 * 2)",
		"1 + (12 * 2))",
	};
	for (const auto& expr : bad_exprs) {
		const auto tokens = tokenize(expr);
		if (!tokens) {
			std::cerr << "Failed Infix to Postfix Bad Expression " << expr << " Cannot Tokenize\n";
			success = false;
			continue;
		}

		const auto result = postfix_expr_from_infix(*tokens);
		if (!result) {
			std::cerr << "Passed Infix to Postfix Bad Expression " << expr << "\n";
		} else {
			std::cerr << "Failed Infix to Postfix Bad Expression " << expr << "\n";
			success = false;
		}
	}

	return success;
}

bool run_tokenize_tests()
{
	bool success = true;
	std::pair<std::string, std::vector<std::string_view>> inputs[] = {
		{ "1 + 3 * 4",          { "1", "+", "3", "*", "4" } },
		{ "1 + 3",              { "1", "+", "3" } },
		{ "(1 + 3) * 2",        { "(", "1", "+", "3", ")", "*", "2" } },
		{ "(4 / 2) + 6",        { "(", "4", "/", "2", ")", "+", "6" } },
		{ "4 + (12 / (1 * 2))", { "4", "+", "(", "12", "/", "(", "1", "*", "2", ")", ")" } },
		{ "4+(12/(1*2))",       { "4", "+", "(", "12", "/", "(", "1", "*", "2", ")", ")" } },
		{ "(1 + (12 * 2)",      { "(", "1", "+", "(", "12", "*", "2", ")" } },
	};
	for (const auto& [expr, expected_result] : inputs) {
		const auto result = tokenize(expr);
		if (!result) {
			std::cerr << "Failed Tokenize \"" << expr << "\"\n";
			success = false;
		} else if (*result == expected_result) {
			std::cerr << "Passed Tokenize " << expr << "\n";
		} else {
			std::cerr << "Failed Tokenize " << expr << " Actual Result: ";
			for (const auto& x : *result) {
				std::cerr << x << ", ";
			}
			std::cerr << "\n";
			success = false;
		}
	}

	std::pair<std::string, std::string> empty_strings[] = {
		{ "",   "Empty" },
		{ " ",  "Space" },
		{ "\t", "Tab" },
	};
	for (const auto& [expr, desc] : empty_strings) {
		const auto result = tokenize(expr);
		if (!result) {
			std::cerr << "Passed Tokenize Empty String (" << desc << ")\n";
		} else {
			std::cerr << "Failed Tokenize Empty String (" << desc << ")\n";
			success = false;
		}
	}

	return success;
}

bool run_tests()
{
	bool success = true;
	success &= run_tokenize_tests();
	success &= run_postfix_from_infix_tests();
	success &= run_evaluate_tests();
	return success;
}

int main(int argc, char** argv)
{
	if (argc < 2) {
		return run_tests() ? 0 : 1;
	}

	std::string expr;
	for (const char* arg : std::span(argv + 1, static_cast<size_t>(argc) - 1)) {
		if (!expr.empty()) {
			expr += ' ';
		}
		expr += arg;
	}

	int result;
	if (!evaluate(expr.c_str(), result)) {
		std::cout << "error: invalid expression\n";
		return 1;
	}
	std::cout << result << "\n";
	return 0;
}

