#include <charconv>
#include <iostream>
#include <span>
#include <stack>
#include <string>
#include <string_view>
#include <vector>

std::vector<std::string_view> postfix_expr_from_infix(std::span<const std::string_view> infix_expr)
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
			if (!stack.empty()) {
				stack.pop();
			}
		} else if (token == "+" || token == "-" || token == "*" || token == "/") {
			// For this app all operators have the same precedence, as shown in the given spec document
			// Copy all operators pushed so far to result
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
		postfix_expr.push_back(stack.top());
		stack.pop();
	}
	return postfix_expr;
}

std::vector<std::string_view> tokenize(std::string_view expr)
{
	std::vector<std::string_view> tokens;
	const std::locale& loc = std::locale::classic();
	std::size_t i = 0;
	while (i < expr.size()) {
		// Find the start and end of each word by ignoring whitespace
		while (i < expr.size() && std::isspace(expr[i], loc)) {
			i++;
		}
		if (i >= expr.size()) {
			break;
		}
		const std::size_t start = i;
		while (i < expr.size() && !std::isspace(expr[i], loc)) {
			i++;
		}
		//const std::size_t end = i - start;

		// The word between start and end may contain a combination of numbers, operators and parentheses
		// that need to be split
		std::size_t cur = start;
		while (cur < i) {
			if (std::isdigit(expr[cur], loc)) {
				const std::size_t beg = cur;
				cur++;
				while (cur < i && std::isdigit(expr[cur], loc)) {
					cur++;
				}
				tokens.push_back(expr.substr(beg, cur - beg));
			} else if (expr[cur] == '-' && (cur+1 < i) && std::isdigit(expr[cur+1], loc)
				   && (tokens.empty() || tokens.back() == "(" || tokens.back() == "+" || tokens.back() == "-"
				       || tokens.back() == "*" || tokens.back() == "/")) {
				// Support negative numbers
				// TODO Support unary postivie numbers e.g. +1 ?
				const std::size_t beg = cur;
				cur++;
				while (cur < i && std::isdigit(expr[cur], loc)) {
					cur++;
				}
				tokens.push_back(expr.substr(beg, cur - beg));
			} else {
				tokens.push_back(expr.substr(cur, 1));
				cur++;
			}
		}
	}
	return tokens;
}

bool evaluate(const char* expression, int& result)
{
	// Transform the infix expression to a postfix expression to make the evaluation easier
	// by removing the parentheses and explicit operator ordering
	std::vector<std::string_view> tokens = postfix_expr_from_infix(tokenize(expression));
	std::stack<int> stack;
	for (std::string_view token : tokens) {
		if (token == "+" || token == "-" || token == "*" || token == "/") {
			if (stack.size() < 2) {
				return false;
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
					return false;
				}
				stack.push(a / b);
				break;
			}
		} else {
			// The tokenize and postfix_expr_from_infix functions do not validate input and
			// badly formed expression can be evaluated. At this point a nubmer is expected
			// and from_chars will return an error with badly formed input.
			int value;
			const char* end = token.data() + token.size();
			auto [ptr, ec] = std::from_chars(token.data(), end, value);
			if (ec != std::errc() || ptr != end) {
				return false;
			}
			stack.push(value);
		}
	}

	if (stack.size() != 1) {
		return false;
	}

	result = stack.top();
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
		{ "-1",                 -1 },
		{ "-1 * 2",             -2 },
		{ "-1+2",                1 },
		{ "1--2",                3 },
	};
	for (auto [expr, expected_result] : inputs) {
		int result;
		bool b = evaluate(expr, result);
		if (b && result == expected_result) {
			std::cerr << "Passed Evaluate " << expr << "\n";
		} else {
			if (!b) {
				std::cerr << "Failed Evaluate " << expr << " invalid expression\n";
			} else {
				std::cerr << "Failed Evaluate " << expr << " Actual Result: " << result << "\n";
			}
			success = false;
		}
	}

	const char* bad_exprs[] = {
		"(1 + (12 * 2)",
		"1 / 0",
		"1 + + 2",
		"+ 1",
	};
	for (const char* expr : bad_exprs) {
		int result;
		if (!evaluate(expr, result)) {
			std::cerr << "Passed Evaluate Bad Expression " << expr << "\n";
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
		// Bad input causes hanging parenthesis, conversion lets them through so keep in test
		{ "(1 + (12 * 2)",      { "1", "12", "2", "*", "+", "(" } },
	};
	for (auto [expr, expected_result] : inputs) {
		std::vector<std::string_view> result = postfix_expr_from_infix(tokenize(expr));
		if (result == expected_result) {
			std::cerr << "Passed Infix to Postfix " << expr << "\n";
		} else {
			std::cerr << "Failed Infix to Postfix " << expr << " Actual Result: ";
			for (auto x : result) {
				std::cerr << x << ", ";
			}
			std::cerr << "\n";
			success = false;
		}
	}
	return success;
}

bool run_tokenize_tests(void)
{
	bool success = true;
	std::pair<std::string, std::vector<std::string_view>> inputs[] = {
		{ "1 + 3 * 4",          { "1", "+", "3", "*", "4" } },
		{ "1 + 3",              { "1", "+", "3" } },
		{ "(1 + 3) * 2",        { "(", "1", "+", "3", ")", "*", "2" } },
		{ "(4 / 2) + 6",        { "(", "4", "/", "2", ")", "+", "6" } },
		{ "4 + (12 / (1 * 2))", { "4", "+", "(", "12", "/", "(", "1", "*", "2", ")", ")" } },
		{ "(1 + (12 * 2)",      { "(", "1", "+", "(", "12", "*", "2", ")" } },
	};
	for (auto [expr, expected_result] : inputs) {
		std::vector<std::string_view> result = tokenize(expr);
		if (result == expected_result) {
			std::cerr << "Passed Tokenize " << expr << "\n";
		} else {
			std::cerr << "Failed Tokenize " << expr << " Actual Result: ";
			for (auto x : result) {
				std::cerr << x << ", ";
			}
			std::cerr << "\n";
			success = false;
		}
	}
	return success;
}

bool run_tests(void)
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

