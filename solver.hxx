#include "dprintf.hxx"
#include "cnf.hxx"
#include "rules.hxx"
#include <cmath>

class Solver
{
public:

    enum class Status : uint8_t
    {
        UNKNOWN, SAT, UNSAT
    };

    Solver(Rule rules);

    Status Solve(CNF cnf);

    uint64_t Complexity();

private:

    Status _DPLLRecursive(CNF cnf, Literal propagate);
    Status _DPLLLinear_test(CNF cnf);
    Status _DPLLLinear(CNF cnf);
    Rule _rules = Rule::NONE;
    bool _recursiveSolving();
    bool _removeTrivial();
    bool _removeSingular();
    bool _removePure();

    uint64_t _complexity = 0;
};

Solver::Solver(Rule rules = Rule::NONE) : _rules(rules) { }

Solver::Status Solver::_DPLLRecursive(CNF cnf, Literal propagate = EmptyLiteral)
{
    _complexity++;
    dprintf("Solving CNF of %d clauses, propagating = %d\n", cnf.ClausesCount(), propagate);

    CNF::ActionResult res = CNF::ActionResult::OK;
    if ((propagate != EmptyLiteral and (res = cnf.PropagateUnit(propagate)) != CNF::ActionResult::OK) or 
        (_removeSingular() and (res = cnf.RemoveSingularClauses()) != CNF::ActionResult::OK) or
        (_removePure() and (res = cnf.RemovePureLiterals()) != CNF::ActionResult::OK)) 
        goto exit;

exit:
    switch (res)
    {
        case CNF::ActionResult::CNF_DEVASTED: return Status::SAT; // If cnf was devasted, this branch is SAT
        case CNF::ActionResult::EMPTY_CLAUSE_CREATED: return Status::UNSAT; // If empty clause was created, this branch is UNSAT
    }

    Literal to_propagate = cnf.FirstLiteral();
    if (to_propagate < 0) to_propagate = -to_propagate;

    if ((not cnf.IsUnsatPropagation(to_propagate) and _DPLLRecursive(cnf, to_propagate) == Status::SAT) or 
        (not cnf.IsUnsatPropagation(-to_propagate) and _DPLLRecursive(cnf, -to_propagate) == Status::SAT)) 
        return Status::SAT; // If one of sub-branches is SAT, current branch is SAT too
    
    return Status::UNSAT; // If all sub-branches are UNSAT, current branch is UNSAT too
}

Solver::Status Solver::_DPLLLinear(CNF initial_cnf)
{
    Status result = Status::UNSAT;

    int propagating_idx = 0;
    int propagating_size = initial_cnf.VariablesCount() + 1;
    Literal* propagating = new Literal[propagating_size] { 0 };
    CNF* cnfs = new CNF[propagating_size];
    cnfs[0] = initial_cnf;

    #define cnf cnfs[propagating_idx]
    #define prev_cnf cnfs[propagating_idx - 1]
    #define propagate propagating[propagating_idx]

    while (true)
    {
        _complexity++;
        dprintf("Solving CNF of %d clauses, propagating = %d\n%s\n", cnf.ClausesCount(), propagating[propagating_idx], cnf.ToString().c_str());

        CNF::ActionResult res = CNF::ActionResult::OK;
        if ((propagate != EmptyLiteral and (res = cnf.PropagateUnit(propagate)) != CNF::ActionResult::OK) or 
            (_removeSingular() and (res = cnf.RemoveSingularClauses()) != CNF::ActionResult::OK) or
            (_removePure() and (res = cnf.RemovePureLiterals()) != CNF::ActionResult::OK)) 
            goto exit;

    exit:
        if (res == CNF::ActionResult::CNF_DEVASTED)
        {
            result = Status::SAT;
            break;
        }

        else if (res == CNF::ActionResult::EMPTY_CLAUSE_CREATED)
        {
            while (propagating_idx > 0 and propagate < 0)
            {
                propagate = EmptyLiteral;
                propagating_idx--;
            }

            if (propagating_idx == 0)
            {
                result = Status::UNSAT;
                break;
            }

            propagate = -propagate;
            cnf = prev_cnf;

            dprintf("Empty clause created, trying another branch (idx = %d, literal = %d)\n", propagating_idx, propagating[propagating_idx]);

            continue;
        }

        propagating_idx++;
        cnf = prev_cnf;

        if (propagate == EmptyLiteral)
        {
            Literal first_literal = cnf.FirstLiteral();
            if (first_literal < 0) first_literal = -first_literal;

            propagate = first_literal;
        }
    }

    delete[] propagating;
    delete[] cnfs;

    #undef cnf
    #undef prev_cnf
    #undef propagate

    return result;
}

Solver::Status Solver::Solve(CNF cnf)
{
    CNF::ActionResult res = CNF::ActionResult::OK;
    if (_removeTrivial())
        res = cnf.RemoveTrivialClauses();

    switch (res)
    {
        case CNF::ActionResult::CNF_DEVASTED: return Status::SAT; // If cnf was devasted, this branch is SAT
        case CNF::ActionResult::EMPTY_CLAUSE_CREATED: return Status::UNSAT; // If empty clause was created, this branch is UNSAT
    }

    if (_recursiveSolving())
        return _DPLLRecursive(cnf);
    else
        return _DPLLLinear(cnf);
}

uint64_t Solver::Complexity()
{
    return _complexity;
}

bool Solver::_recursiveSolving()
{
    return (_rules & Rule::RECURSIVE_SOLVING) == Rule::RECURSIVE_SOLVING;
}

bool Solver::_removeTrivial()
{
    return (_rules & Rule::REMOVE_TRIVIAL) == Rule::REMOVE_TRIVIAL;
}

bool Solver::_removeSingular()
{
    return (_rules & Rule::REMOVE_SINGULAR) == Rule::REMOVE_SINGULAR;
}

bool Solver::_removePure()
{
    return (_rules & Rule::REMOVE_PURE) == Rule::REMOVE_PURE;
}
