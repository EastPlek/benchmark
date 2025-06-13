
#include <vector>
#include <any>
#include <iostream>
#include <unordered_map>
#include <string>

#define function(X) struct X : public function_base
#define MAIN int main(int argc, char* argv[]) {\
				Main main(argc,argv);			 \
				}
#define LINKS std::vector<function_base*> link
struct function_base {
	function_base() = default;
	virtual ~function_base() = default;

	template<typename T>
	T& var(const std::string& name) {
		return std::any_cast<T&>(variables.at(name));
	}
	template <typename T>
	void declare(const std::string& name, T val) {
		variables[name] = std::any(val);
	}

	std::unordered_map<std::string,std::any> variables;
};

function(Sum) {
	enum class Links {
		Main
	};
	Sum(int x, int y, LINKS) {
		link[static_cast<int>(Links::Main)]->var<int>("z") = x + y;
	}
};

function(Main) {
	Main(int argc, char* argv[]) {
		declare("z", 0);
		Sum sum(6, 10, { this });
		std::cout << var<int>("z") << '\n';
	}
};

MAIN