#define DEBUG
#include "dpll.hxx"
#include <cstdio>

int main(int argc, char** argv)
{
    if (argc == 1)
        dprintf("Filename isn't provided!\nUsage: %s [filename, ..]\n", argv[0]);

    for (int f_no = 1; f_no < argc; f_no++)
    {
        auto solver = DPLLSolver();
        auto err = solver.loadDIMACS(argv[f_no]);
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

    return 0;
}
