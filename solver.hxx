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

Solver::Status Solver::_DPLLLinear_test(CNF initial_cnf)
{
    #define propagate steps[steps_idx].s_propagate
    #define cnf steps[steps_idx].s_cnf
    #define checked_both steps[steps_idx].s_checked_both

    Status global_result = Status::UNSAT;

    struct Step
    {
        Literal s_propagate = EmptyLiteral;
        bool s_checked_both = false;
        CNF s_cnf;
    };

    int steps_idx = 0;
    Step* steps = new Step[initial_cnf.VariablesCount()];
    steps[steps_idx].s_cnf = initial_cnf;

    while (true)
    {
        _complexity++;
        dprintf("Solving CNF of %d clauses, propagating = %d\n", cnf.ClausesCount(), propagate);

        CNF::ActionResult res = CNF::ActionResult::OK;
        if ((propagate != EmptyLiteral and (res = cnf.PropagateUnit(propagate)) != CNF::ActionResult::OK) or 
            (_removeSingular() and (res = cnf.RemoveSingularClauses()) != CNF::ActionResult::OK) or
            (_removePure() and (res = cnf.RemovePureLiterals()) != CNF::ActionResult::OK)) 
            goto exit;

    exit:
        if (res == CNF::ActionResult::CNF_DEVASTED)
        {
            global_result = Status::SAT;
            break;
        }

        else if (res == CNF::ActionResult::EMPTY_CLAUSE_CREATED)
        {
            while (not steps_idx > 0 and checked_both)
                steps_idx--;

            if (steps_idx == 0)
                break;
            
            propagate = -propagate;
            cnf = steps[steps_idx - 1].s_cnf;
            checked_both = true;
            continue;
        }

        else
        {
            Literal to_propagate = cnf.FirstLiteral();
            if (to_propagate < 0) to_propagate = -to_propagate;

            // steps.push_back( { to_propagate, false, cnf } );
            steps_idx++;
            propagate = to_propagate;
            cnf = steps[steps_idx - 1].s_cnf;
        }
    }

    delete[] steps;

    return global_result;

    #undef propagate
    #undef cnf
    #undef checked_both
}

Solver::Status Solver::_DPLLLinear(CNF initial_cnf)
{
    Status result = Status::UNSAT;

    CNF cnf = initial_cnf;

    int propagating_idx = 0;
    int propagating_size = cnf.VariablesCount() + 1;
    Literal* propagating = new Literal[propagating_size] { 0 };
    CNF* cnfs = new CNF[propagating_size];

    while (true)
    {
        _complexity++;
        dprintf("Solving CNF of %d clauses, propagating = %d\n%s\n", cnf.ClausesCount(), propagating[propagating_idx], cnf.ToString().c_str());

        CNF::ActionResult res = CNF::ActionResult::OK;
        if ((propagating[propagating_idx] != EmptyLiteral and (res = cnf.PropagateUnit(propagating[propagating_idx])) != CNF::ActionResult::OK) or 
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
            while (propagating_idx > 0 and propagating[propagating_idx] < 0)
            {
                propagating[propagating_idx] = EmptyLiteral;
                propagating_idx--;
            }

            if (propagating_idx == 0)
            {
                result = Status::UNSAT;
                break;
            }

            propagating[propagating_idx] = -propagating[propagating_idx];
            cnf = cnfs[propagating_idx - 1];

            dprintf("Empty clause created, trying another branch (idx = %d, literal = %d)\n", propagating_idx, propagating[propagating_idx]);

            continue;
        }

        cnfs[propagating_idx] = cnf;
        propagating_idx++;

        if (propagating[propagating_idx] == EmptyLiteral)
        {
            Literal first_literal = cnf.FirstLiteral();
            if (first_literal < 0) first_literal = -first_literal;

            propagating[propagating_idx] = first_literal;
        }
    }

    delete[] propagating;

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
