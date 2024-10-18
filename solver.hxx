#include "dprintf.hxx"
#include "cnf.hxx"
#include "rules.hxx"
#include <cmath>

class Solver
{
public:

    enum class Status : uint8_t
    {
        UNKNOWN, EARLY_SAT, EARLY_UNSAT, SAT, UNSAT
    };

    Solver(Rule rules);

    Status DPLLRecursive(CNF cnf, Literal propagate);
    Status DPLLLinear(CNF cnf);

    uint64_t Complexity();

private:

    Rule _rules = Rule::NONE;
    bool _removeTrivial();
    bool _removeSingular();
    bool _removePure();

    uint64_t _complexity = 0;
};

Solver::Solver(Rule rules = Rule::NONE) : _rules(rules) { }

Solver::Status Solver::DPLLRecursive(CNF cnf, Literal propagate = EmptyLiteral)
{
    _complexity++;
    dprintf("Solving CNF of %d clauses, propagating = %d\n", cnf.ClausesCount(), propagate);

    CNF::ActionResult res = CNF::ActionResult::OK;
    if ((propagate != EmptyLiteral and (res = cnf.PropagateUnit(propagate)) != CNF::ActionResult::OK) or 
        (_removeTrivial() and (res = cnf.RemoveTrivialClauses()) != CNF::ActionResult::OK) or
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

    if ((not cnf.IsUnsatPropagation(to_propagate) and DPLLRecursive(cnf, to_propagate) == Status::SAT) or 
        (not cnf.IsUnsatPropagation(-to_propagate) and DPLLRecursive(cnf, -to_propagate) == Status::SAT)) 
        return Status::SAT; // If one of sub-branches is SAT, current branch is SAT too
    
    return Status::UNSAT; // If all sub-branches are UNSAT, current branch is UNSAT too
}

uint64_t Solver::Complexity()
{
    return _complexity;
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
