#include "solver.hxx"
#include "dprintf.hxx"
#include "dimacs.hxx"
#include "cnf.hxx"
#include <cstdio>

int main(int argc, char** argv)
{
    if (argc == 1)
        dprintf("Filename isn't provided!\nUsage: %s [filename, ..]\n", argv[0]);

    int err_count = 0;
    auto solver = Solver(Rule::REMOVE_SINGULAR /*| Rule::REMOVE_PURE*/);

    for (int f_no = 1; f_no < argc; f_no++)
    {
        CNF cnf = DIMACS::ReadFromFile(argv[f_no]);

        dprintf("Loaded %s\nCNF consist of %d clauses\n%s\n", argv[f_no], cnf.ClausesCount(), cnf.ToString().c_str());
        
        auto result = solver.DPLLRecursive(cnf);
        
        switch (result)
        {
            case Solver::Status::UNKNOWN:
                printf("UNKOWN\n"); break;
            case Solver::Status::SAT:
                printf("SAT\n"); break;
            case Solver::Status::UNSAT:
                printf("UNSAT\n"); break;
            default:
                printf("Unreachable");
        }
    }

    return err_count != 0;
}
