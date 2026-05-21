#include <iostream>
#include <span>
#include <stack>
#include <string>
#include <string_view>
#include <vector>

std::vector<std::string_view> postfix_expr_from_infix(std::span<const std::string_view> infix_expr)
{
	// Use the Shunting Yarn algorithm to convert an infix expression to postfix expression
	std::vector<std::string_view> postfix_expr;
	std::stack<std::string_view> stack;
	for (std::string_view token : infix_expr) {
		if (token == "(") {
			stack.push(token);
		} else if (token == ")") {
			// On closing parenthese copy all operators to result
			while (!stack.empty() && stack.top() != "(") {
				postfix_expr.push_back(stack.top());
				stack.pop();
			}
			// Remove the opening parenthese
			if (!stack.empty()) {
				stack.pop();
			}
		} else if (token == "+" || token == "-" || token == "*" || token == "/") {
			// For this app all operators have the same precendence, as shown in the given spec document
			// Copy all operators push so far to result
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
	// Any remaing operators copy to result
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

		// The word beween start and end may contain a combination of numbers, operators and parentheses
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
			} else {
				tokens.push_back(expr.substr(cur, 1));
				cur++;
			}
		}
	}
	return tokens;
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
		// Bad input cause hanging parenthese, conversion lets them through so keep in test
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
	return success;
}

int main(void)
{
	return run_tests() ? 0 : 1;
}

