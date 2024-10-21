#pragma once
#include <cstring>
#include <list>
#include "literal.hxx"

class CNF
{
public:

    enum class ActionResult : uint8_t
    {
        OK, EMPTY_CLAUSE_CREATED, CNF_DEVASTED
    };

    struct PureResult
    {
        bool pure;
        bool negative;
    };

    CNF();
    CNF(CNF& other); /// Copy constructor (necessary to save intermediate states)
    CNF& operator=(CNF& other);
    ~CNF();

    bool IsUnsatPropagation(Literal literal); /// Checking if propagating of literal will cause UNSAT for this branch

    PureResult IsPure(Literal literal); /// Checking if literal pure or not
    Literal FindSingularClause(); /// Finding first single clause in CNF and returning literal from this clause
    Literal FindPureLiteral(); /// Finding first pure literal in CNF
    Literal FirstLiteral(); /// Returning first literal in CNF

    ActionResult PropagateUnit(Literal literal); /// Removing all clauses with literal and all contra-literal occurancies from remaining clauses

    ActionResult RemoveTrivialClauses(); /// Rule 0: Removing trivial clauses from CNF
    ActionResult RemoveSingularClauses(); /// Rule 1: Propagating units from singular clauses
    ActionResult RemovePureLiterals(); /// Rule 2: Removing clauses with pure literals

    uint32_t ClausesCount();
    uint32_t VariablesCount();
    std::string ToRawString();
    std::string ToString();

    friend class DIMACS;

private:
    Literal _single_literal = EmptyLiteral;

    uintptr_t _cnf_data_size = 0; /// Current size of _cnf_data
    Literal* _cnf_data = nullptr; /// Clauses separated by EmptyLiteral (also after last element)

    uint32_t _variables_count = 0; /// Count of variables used in cnf
    uint32_t _clauses_count = 0; /// Count of clauses in cnf
};

CNF::CNF() { }

CNF::CNF(CNF& other)
{
    _cnf_data_size = other._cnf_data_size;
    _cnf_data = new Literal[_cnf_data_size] { 0 };
    memcpy(_cnf_data, other._cnf_data, _cnf_data_size * sizeof(Literal));

    _variables_count = other._variables_count;
    _clauses_count = other._clauses_count;

    _single_literal = EmptyLiteral;
}

CNF& CNF::operator=(CNF& other)
{
    if (this == &other)
        return *this;

    if (_cnf_data)
    {
        delete[] _cnf_data;
        _cnf_data = nullptr;
        _cnf_data_size = 0;
    }

    _cnf_data_size = other._cnf_data_size;
    _cnf_data = new Literal[_cnf_data_size] { 0 };
    memcpy(_cnf_data, other._cnf_data, _cnf_data_size * sizeof(Literal));

    _variables_count = other._variables_count;
    _clauses_count = other._clauses_count;

    _single_literal = EmptyLiteral;

    return *this;
}

CNF::~CNF()
{
    if (_cnf_data)
    {
        delete[] _cnf_data;
        _cnf_data = nullptr;
        _cnf_data_size = 0;
    }
}

bool CNF::IsUnsatPropagation(Literal literal)
{
    return false;
}

CNF::PureResult CNF::IsPure(Literal literal)
{
    bool found = false;
    bool found_negation = false;

    for (int i = 0; i < _cnf_data_size; i++)
    {
        if (_cnf_data[i] == literal)
            found = true;
        else if (_cnf_data[i] == -literal)
            found_negation = true;
    }

    return { found != found_negation, found_negation };
}

Literal CNF::FindSingularClause()
{
    // Propagate (single way to edit CNF) updating list of single literals, 
    // so it is necessary to search in cycle only if CNF not edited
    if (_single_literal == EmptyLiteral)
    {        
        bool first_literal_in_clause = true;

        for (int i = 0; i < _cnf_data_size; i++)
        {
            if (first_literal_in_clause and _cnf_data[i + 1] == EmptyLiteral)
                return _cnf_data[i];

            if (first_literal_in_clause)
                first_literal_in_clause = false;

            if (_cnf_data[i] == EmptyLiteral)
                first_literal_in_clause = true;
        }
    }

    Literal found = _single_literal;
    _single_literal = EmptyLiteral;

    return found;
}

Literal CNF::FindPureLiteral()
{
    for (Literal literal = 1; literal <= _variables_count; literal++)
    {
        auto res = IsPure(literal);
        if (res.pure)
            return res.negative ? -literal : literal;
    }

    return EmptyLiteral;
}

Literal CNF::FirstLiteral()
{
    if (_cnf_data_size > 0 and _cnf_data[0] != EmptyLiteral)
        return _cnf_data[0];
    
    return EmptyLiteral;
}

CNF::ActionResult CNF::PropagateUnit(Literal literal)
{
    int old_clause_size = 0;
    int new_clause_size = 0;

    bool clause_removed = false;

    int new_idx = 0;

    for (int old_idx = 0; old_idx < _cnf_data_size; old_idx++)
    {
        old_clause_size++;
        Literal current = _cnf_data[old_idx];

        bool clause_end = current == EmptyLiteral;

        if (current == -literal) // Just skipping contra-literals
        {
            _cnf_data[old_idx] = EmptyLiteral;

            dprintf("Literal %d removed from clause\n", -literal);
        }

        else if (current == literal) // Deleting copied literals of this clause and skipping uncopied ending
        {
            while (new_idx > 0 and _cnf_data[new_idx - 1] != EmptyLiteral)
                _cnf_data[--new_idx] = EmptyLiteral;
        
            while (old_idx < _cnf_data_size and _cnf_data[old_idx++] != EmptyLiteral)
                _cnf_data[old_idx - 1] = EmptyLiteral;
            
            old_idx--;
            clause_removed = true;
            _clauses_count--;
            clause_end = true;

            dprintf("Whole clause with literal %d removed\n", literal);
        }

        else
        {
            _cnf_data[new_idx++] = current;
            new_clause_size++;
        }

        if (clause_end)
        {
            if (not clause_removed)
            {
                new_clause_size--;
                old_clause_size--;

                switch (new_clause_size)
                {
                    case 0:
                    {
                        dprintf("Empty clause created while removing literal %d\n", literal);
                        return ActionResult::EMPTY_CLAUSE_CREATED;
                    }
                    
                    case 1:
                    {
                        if (old_clause_size == 2)
                        {
                            _single_literal = _cnf_data[new_idx - 2];
                            dprintf("Singular clause of literal %d created while removing literal %d\n", _cnf_data[new_idx - 2], literal);
                        }
                    }
                }
            }
            else
                clause_removed = false;

            new_clause_size = 0;
            old_clause_size = 0;
        }
    }

    _cnf_data_size = new_idx;

    return _cnf_data_size == 0 ? ActionResult::CNF_DEVASTED : ActionResult::OK;
}

CNF::ActionResult CNF::RemoveTrivialClauses()
{
    dprintf("Removing trivial clauses\n", nullptr);
    return ActionResult::OK;
}

CNF::ActionResult CNF::RemoveSingularClauses()
{
    dprintf("Removing singular clauses\n", nullptr);
    Literal propagating = FindSingularClause();
    while (propagating != EmptyLiteral)
    {
        dprintf("Removing literal %d\n", propagating);

        ActionResult res = PropagateUnit(propagating);

        dprintf("Literal %d removed\n", propagating);

        if (res != ActionResult::OK) return res;

        propagating = FindSingularClause();
    }

    dprintf("Singular clauses removed\n", nullptr);
    return ActionResult::OK;
}

CNF::ActionResult CNF::RemovePureLiterals()
{
    dprintf("Removing clauses with pure literals\n", nullptr);

    Literal propagating = FindPureLiteral();
    while (propagating != EmptyLiteral)
    {
        dprintf("Removing pure literal %d\n", propagating);

        ActionResult res = PropagateUnit(propagating);
        if (res != ActionResult::OK) return res;

        propagating = FindPureLiteral();
    }

    return ActionResult::OK;
}

uint32_t CNF::ClausesCount()
{
    return _clauses_count;
}

uint32_t CNF::VariablesCount()
{
    return _variables_count;
}

std::string CNF::ToRawString()
{
    char buffer[50] = { 0 };
    std::string result = "[";
    for (uintptr_t idx = 0; idx < _cnf_data_size; idx++)
    {
        sprintf(buffer, "%d, ", _cnf_data[idx]);
        result = result.append(buffer);
    }
    if (_cnf_data_size > 0)
    {
        result.pop_back();
        result.pop_back();
    }
    result = result.append("]");
    return result;
}

std::string CNF::ToString()
{
    char buffer[50] = { 0 };
    std::string result = "0: [";
    int clause_no = 1;

    for (uintptr_t idx = 0; idx < _cnf_data_size; idx++)
    {
        if (_cnf_data[idx] == EmptyLiteral)
        {
            if (clause_no >= _clauses_count)
                break;
            result.pop_back();
            sprintf(buffer, "]\n%d: [", clause_no++);
            result = result.append(buffer);
        }
        else
        {
            sprintf(buffer, "%d ", _cnf_data[idx]);
            result = result.append(buffer);
        }
    }

    result.pop_back();
    result = result.append("]");

    return result;
}
