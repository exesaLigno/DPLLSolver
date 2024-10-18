SOURCES=main.cxx solver.hxx rules.hxx cnf.hxx literal.hxx dimacs.hxx dprintf.hxx

all: dpll

dpll: $(SOURCES)
	g++ -std=c++17 -Wpedantic -Werror -O2 main.cxx -o dpll

dpll-debug: $(SOURCES)
	g++ -std=c++17 -g3 -Wpedantic -Werror -fsanitize=address -lasan -D DEBUG main.cxx -o dpll

dpll-perf: $(SOURCES)
	g++ -std=c++17 -g3 -Wpedantic -Werror -O2 main.cxx -o dpll
