#define DEBUG
#include "dpll.hxx"
#include "dprintf.hxx"
#include <cstdio>

int main(int argc, char** argv)
{
    if (argc == 1)
        dprintf("Filename isn't provided!\nUsage: %s [filename, ..]\n", argv[0]);

    int err_count = 0;

    for (int f_no = 1; f_no < argc; f_no++)
    {
        auto solver = DPLLSolver();
        if (solver.loadDIMACS(argv[f_no]) != DPLLSolver::ERROR::OK)
        {
            dprintf("Cannot load CNF from file %s\n", argv[f_no]);
            err_count++;
            continue;
        }
        
        auto result = solver.solve();
        
        switch (result)
        {
            case DPLLSolver::STATUS::UNKNOWN:
                printf("UNKOWN\n"); break;
            case DPLLSolver::STATUS::SAT:
                printf("SAT\n"); break;
            case DPLLSolver::STATUS::UNSAT:
                printf("UNSAT\n"); break;
            default:
                printf("Unreachable");
        }
    }

    return err_count != 0;
}
