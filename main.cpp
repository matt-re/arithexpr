#include <charconv>
#include <iostream>
#include <limits>
#include <locale>
#include <optional>
#include <span>
#include <stack>
#include <string>
#include <string_view>
#include <vector>

// Replace with chk_add and co. in C++ 26  https://en.cppreference.com/cpp/header/stdckdint.h
// Current implementation uses
// https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/integers-int/int32-c

bool checked_add(int& result, int a, int b)
{
	if (b > 0 && (a > std::numeric_limits<int>::max() - b)) {
		return true;
	}
	if (b < 0 && (a < std::numeric_limits<int>::min() - b)) {
		return true;
	}
	result = a + b;
	return false;
}

bool checked_sub(int& result, int a, int b)
{
	if (b > 0 && (a < std::numeric_limits<int>::min() + b)) {
		return true;
	}
	if (b < 0 && (a > std::numeric_limits<int>::max() + b)) {
		return true;
	}
	result = a - b;
	return false;
}

bool checked_mul(int& result, int a, int b)
{
	if (a > 0) {
		if (b > 0) {
			if (a > std::numeric_limits<int>::max() / b) {
				return true;
			}
		} else {
			if (b < std::numeric_limits<int>::min() / a) {
				return true;
			}
		}
	} else {
		if (b > 0) {
			if (a < std::numeric_limits<int>::min() / b) {
				return true;
			}
		} else {
			if (a != 0 && b < std::numeric_limits<int>::max() / a) {
				return true;
			}
		}
	}
	result = a * b;
	return false;
}

bool checked_div(int& result, int a, int b)
{
	if (b == 0) {
		return true;
	}
	if (a == std::numeric_limits<int>::min() && b == -1) {
		return true;
	}
	result = a / b;
	return false;
}

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

			int result;
			bool overflow = true;

			switch (token[0]) {
			case '+':
				overflow = checked_add(result, a, b);
				break;
			case '-':
				overflow = checked_sub(result, a, b);
				break;
			case '*':
				overflow = checked_mul(result, a, b);
				break;
			case '/':
				overflow = checked_div(result, a, b);
				break;
			}

			if (overflow) {
				return std::nullopt;
			}
			stack.push(result);
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

std::optional<std::vector<std::string_view>> postfix_from_infix(std::span<const std::string_view> tokens)
{
	// Use the Shunting Yard algorithm to convert an infix expression to postfix expression
	std::vector<std::string_view> result;
	std::stack<std::string_view> stack;
	for (std::string_view token : tokens) {
		if (token == "(") {
			stack.push(token);
		} else if (token == ")") {
			// On closing parenthesis copy all operators to result
			while (!stack.empty() && stack.top() != "(") {
				result.push_back(stack.top());
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
				result.push_back(stack.top());
				stack.pop();
			}
			stack.push(token);
		} else {
			// Copy number straight to result
			result.push_back(token);
		}
	}
	// Any remaining operators copy to result
	while (!stack.empty()) {
		if (stack.top() == "(") {
			return std::nullopt;
		}
		result.push_back(stack.top());
		stack.pop();
	}
	return result;
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
	std::optional<std::vector<std::string_view>> postfix_tokens = postfix_from_infix(*tokens);
	if (!postfix_tokens) {
		return false;
	}
	std::optional<int> value = evaluate(*postfix_tokens);
	if (!value) {
		return false;
	}
	result = *value;
	return true;
}

std::string tokens_to_string(const std::vector<std::string_view>& value)
{
	std::string s;
	for (const auto& v : value) {
		if (!s.empty()) {
			s += ' ';
		}
		s += v;
	}
	return s;
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
		{ "2147483646 + 1",      std::numeric_limits<int>::max() },
		{ "-2147483647 + -1",    std::numeric_limits<int>::min() },
		{ "0 * 0",               0 },
		{ "46340 * 46341",       2147441940 }, // sqrt(INT_MAX) = 46340.9
		{ "-46340 * -46341",     2147441940 },
		{ "-1073741824 * 2",     std::numeric_limits<int>::min() },
		{ "1073741824 * -2",     std::numeric_limits<int>::min() },
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
	std::pair<std::vector<std::string_view>, std::vector<std::string_view>> inputs[] = {
		{ { "1", "+", "3", "*", "4" },                                { "1", "3", "+", "4", "*" } },
		{ { "1", "+", "3" },                                          { "1", "3", "+" } },
		{ { "(", "1", "+", "3", ")", "*", "2" },                      { "1", "3", "+", "2", "*" } },
		{ { "(", "4", "/", "2", ")", "+", "6" },                      { "4", "2", "/", "6", "+" } },
		{ { "4", "+", "(", "12", "/", "(", "1", "*", "2", ")", ")" }, { "4", "12", "1", "2", "*", "/", "+" } },
	};
	for (const auto& [tokens, expected_result] : inputs) {
		const auto result = postfix_from_infix(tokens);
		if (!result) {
			std::cerr << "Failed Infix to Postfix " << tokens_to_string(tokens) << " Invalid Expression\n";
			success = false;
		} else if (*result == expected_result) {
			std::cerr << "Passed Infix to Postfix " << tokens_to_string(tokens) << "\n";
		} else {
			std::cerr << "Failed Infix to Postfix " << tokens_to_string(tokens) << " Actual Result: " << tokens_to_string(*result) << "\n";
			success = false;
		}
	}

	std::vector<std::string_view> invalid_tokens[] = {
		{ "(", "1", "+", "(", "12", "*", "2", ")" },
		{ "1", "+", "(", "12", "*", "2", ")", ")" },
	};
	for (const auto& tokens : invalid_tokens) {
		const auto result = postfix_from_infix(tokens);
		if (!result) {
			std::cerr << "Passed Infix to Postfix Bad Expression " << tokens_to_string(tokens) << "\n";
		} else {
			std::cerr << "Failed Infix to Postfix Bad Expression " << tokens_to_string(tokens) << "\n";
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
			std::cerr << "Failed Tokenize " << expr << "\n";
			success = false;
		} else if (*result == expected_result) {
			std::cerr << "Passed Tokenize " << expr << "\n";
		} else {
			std::cerr << "Failed Tokenize " << expr << " Actual Result: " << tokens_to_string(*result) << "\n";
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

bool run_overflow_tests()
{
	bool success = true;
	const char* inputs[] = {
		" 2147483648",		// max constant, from_chars fails
		"-2147483649", 		// min constant, from_chars fails
		" 2147483647 +  1",	// add overflow
		"-2147483648 + -1",	// add underflow
		" 2147483647 - -1",	// sub overflow
		"-2147483648 -  1",	// sub underflow
		" 2147483647 *  2",	// mul +ve * +ve > INT_MAX
		"-2147483648 *  2",	// mul -ve * +ve < INT_MIN
		" 2147483647 * -2",	// mul +ve * -ve < INT_MIN
		"-2147483648 * -1",	// mul -ve * -ve > INT_MAX
		"-2147483648 / -1",	// div overflow
	};
	for (const auto& expr : inputs) {
		int result;
		if (!evaluate(expr, result)) {
			std::cerr << "Passed Evaluate Overflow Expression " << expr << "\n";
		} else {
			std::cerr << "Failed Evaluate Overflow Expression " << expr << "\n";
			success = false;
		}
	}
	return success;
}

bool run_checked_tests()
{
	bool success = true;
	int unused;

	if (checked_add(unused, 2147483647, 1)) {
		std::cerr << "Passed Add Overflow\n";
	} else {
		std::cerr << "Failed Add Overflow\n";
		success = false;
	}

	if (checked_add(unused, -2147483648, -1)) {
		std::cerr << "Passed Add Underflow\n";
	} else {
		std::cerr << "Failed Add Underflow\n";
		success = false;
	}

	if (checked_sub(unused, 2147483647, -1)) {
		std::cerr << "Passed Sub Overflow\n";
	} else {
		std::cerr << "Failed Sub Overflow\n";
		success = false;
	}

	if (checked_sub(unused, -2147483648, 1)) {
		std::cerr << "Passed Sub Underflow\n";
	} else {
		std::cerr << "Failed Sub Underflow\n";
		success = false;
	}

	if (checked_mul(unused, 2147483647, 2)) {
		std::cerr << "Passed Mul +ve and +ve Overflow\n";
	} else {
		std::cerr << "Failed Mul +ve and +ve Overflow\n";
		success = false;
	}

	if (checked_mul(unused, -2147483648, 2)) {
		std::cerr << "Passed Mul -ve and +ve Underflow\n";
	} else {
		std::cerr << "Failed Mul -ve and +ve Underflow\n";
		success = false;
	}

	if (checked_mul(unused, 2147483647, -2)) {
		std::cerr << "Passed Mul +ve and -ve Underflow\n";
	} else {
		std::cerr << "Failed Mul +ve and -ve Underflow\n";
		success = false;
	}

	if (checked_mul(unused, -2147483648, -1)) {
		std::cerr << "Passed Mul -ve and -ve Overflow\n";
	} else {
		std::cerr << "Failed Mul -ve and -ve Overflow\n";
		success = false;
	}

	if (checked_div(unused, -2147483648, -1)) {
		std::cerr << "Passed Div Overflow\n";
	} else {
		std::cerr << "Failed Div Overflow\n";
		success = false;
	}

	return success;
}

bool run_tests()
{
	bool success = true;
	success &= run_tokenize_tests();
	success &= run_postfix_from_infix_tests();
	success &= run_evaluate_tests();
	success &= run_overflow_tests();
	success &= run_checked_tests();
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

