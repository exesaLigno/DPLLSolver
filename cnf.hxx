#pragma once
#include <cstring>
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
        bool pure : 4;
        bool negative : 4;
    };

    CNF();
    CNF(CNF& other); /// Copy constructor (necessary to save intermediate states)
    ~CNF();

    PureResult IsPure(Literal literal); /// Checking if literal pure or not
    Literal FindSingularClause(); /// Finding first single clause in CNF and returning literal from this clause
    Literal FirstLiteral(); /// Returning first literal in CNF

    ActionResult PropagateUnit(Literal literal); /// Removing all clauses with literal and all contra-literal occurancies from remaining clauses

    ActionResult RemoveTrivialClauses(); /// Rule 0: Removing trivial clauses from CNF
    ActionResult RemoveSingularClauses(); /// Rule 1: Propagating units from singular clauses
    ActionResult RemovePureLiterals(); /// Rule 2: Removing clauses with pure literals

    uint32_t ClausesCount();
    std::string ToRawString();
    std::string ToString();

    friend class DIMACS;

private:
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
    bool skip_clause = false;

    for (int i = 0; i < _cnf_data_size and _cnf_data[i] != 0; i++)
    {
        if (_cnf_data[i] == literal)
        {
            skip_clause = true;
            break;
        }
    }

    int new_idx = 0;

    for (int old_idx = 0; old_idx < _cnf_data_size; old_idx++)
    {
        bool clause_end = _cnf_data[old_idx] == EmptyLiteral;

        if (!skip_clause && _cnf_data[old_idx] != -literal)
        {
            Literal tmp = _cnf_data[old_idx];
            _cnf_data[old_idx] = EmptyLiteral;
            _cnf_data[new_idx] = tmp;

            if (_cnf_data[new_idx] == EmptyLiteral and (new_idx == 0 or _cnf_data[new_idx - 1] == EmptyLiteral))
            {
                dprintf("Empty clause created while removing literal %d\n", literal);
                return ActionResult::EMPTY_CLAUSE_CREATED;
            }

            new_idx++;
        }

        else
            _cnf_data[old_idx] = EmptyLiteral;

        if (clause_end)
        {
            skip_clause = false;

            for (int i = old_idx + 1; i < _cnf_data_size and _cnf_data[i] != 0; i++)
            {
                if (_cnf_data[i] == literal)
                {
                    skip_clause = true;
                    break;
                }
            }
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
    while (true)
    {
        ActionResult res = PropagateUnit(propagating);
        if (res != ActionResult::OK) return res;

        if ((propagating = FindSingularClause()) == EmptyLiteral)
            break;
    }

    return ActionResult::OK;
}

CNF::ActionResult CNF::RemovePureLiterals()
{
    dprintf("Removing clauses with pure literals\n", nullptr);
    return ActionResult::OK;
}

uint32_t CNF::ClausesCount()
{
    return _clauses_count;
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
    result.pop_back();
    result.pop_back();
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
