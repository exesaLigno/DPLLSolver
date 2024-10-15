all: dpll

dpll: dpll.hxx literal.hxx literal_storage.hxx dprintf.hxx clause.hxx main.cxx
	g++ -std=c++17 -Wpedantic -Werror -O2 main.cxx -o dpll

dpll-debug: dpll.hxx literal.hxx literal_storage.hxx dprintf.hxx clause.hxx main.cxx
	g++ -std=c++17 -g3 -Wpedantic -Werror -fsanitize=address -lasan main.cxx -o dpll
