#include "dprintf.hxx"
#include "literal.hxx"
#include "clause.hxx"
#include "literal_storage.hxx"
#include <cstdint>
#include <fstream>
#include <cstring>
#include <vector>

class DPLLSolver
{
public:

    enum class ERROR : uint8_t
    {
        OK, FILE_NOT_FOUND, CORRUPTED_DATA, UNSUPPORTED_FORMAT, MULTIPLE_HEADERS
    };

    enum class STATUS : uint8_t
    {
        UNKNOWN, EARLY_SAT, EARLY_UNSAT, SAT, UNSAT
    };

    enum class STEP_RESULT : uint8_t
    {
        OK, CLAUSE_EXHAUSTED, FORMULA_EXHAUSTED
    };

    DPLLSolver();
    ERROR loadDIMACS(char filename[]);
    STATUS solve();
    ~DPLLSolver();


private:    
    char* _cnf_text = nullptr;

    int _variables_count = 0;
    LiteralStorage _literal_storage;

    int _clauses_count = 0;
    Clause** _clauses = nullptr;

    std::list<Clause*> _formula;
    std::list<Clause*> _singular_clauses;

    STEP_RESULT _removeTrivialClauses();
    STEP_RESULT _propagateUnit();
    void _applyLiteralsSummary();
    STEP_RESULT _removePureClauses();

    static long _readFile(char filename[], char** destination);
};

long DPLLSolver::_readFile(char filename[], char** destination)
{
    FILE* file = fopen(filename, "r");
    fseek(file, 0, SEEK_END);
	long text_length = ftell(file);
	rewind(file);
	*destination = new char[text_length + 1]{0};
    text_length = fread(*destination, sizeof(char), text_length, file);
    fclose(file);

    return text_length;
}

DPLLSolver::DPLLSolver() { }

DPLLSolver::ERROR DPLLSolver::loadDIMACS(char filename[])
{
    long text_length = _readFile(filename, &_cnf_text);
    
    uintptr_t current_line = 0;
    bool header_presented = false;
    uintptr_t clause_counter = 0;

    for (uintptr_t current_sym = 0; current_sym < text_length; current_sym++)
    {
        // Parsing line when it's end reached
        if (_cnf_text[current_sym] == '\n' or _cnf_text[current_sym] == '\0')
        {
            _cnf_text[current_sym] = '\0';

            if (current_sym <= current_line); // Skipping empty lines
            else if (_cnf_text[current_line] == 'c'); // Skipping commented lines
            else if (_cnf_text[current_line] == 'p') // Parsing cnf info (p cnf <variables> <clauses>)
            {
                if (header_presented)
                {
                    dprintf("Multiple headers presented in file!\n", nullptr);
                    return DPLLSolver::ERROR::MULTIPLE_HEADERS;
                }
                header_presented = true;
                sscanf(_cnf_text + current_line, "p cnf %d %d", &_variables_count, &_clauses_count);
                dprintf("Readed header: %d vars and %d clauses\n", _variables_count, _clauses_count);
                _clauses = new Clause*[_clauses_count]{0};
                _literal_storage.Allocate(_variables_count);
            }
            else if (clause_counter < _clauses_count)
            {
                if (!header_presented)
                {
                    dprintf("No header presented in DIMACS!\n", nullptr);
                    return DPLLSolver::ERROR::UNSUPPORTED_FORMAT;
                }

                auto clause = new Clause(_cnf_text + current_line);
                _clauses[clause_counter++] = clause;
                _formula.push_back(clause);
                if (clause->Singular())
                    _singular_clauses.push_back(clause);
            }

            current_line = current_sym + 1;
            while (_cnf_text[current_line] == ' ' or
                   _cnf_text[current_line] == '\n' or
                   _cnf_text[current_line] == '\t')
                current_line++;
        }
    }

    debug
    {
        int i = 1;
        dprintf("┍━Current formula\n", nullptr);
        for (auto clause : _formula)
            dprintf("│     Clause %d: [%s] (size: %d)\n", i++, clause->ToString().c_str(), clause->Size());

        dprintf("┕━Formula contains %d singular clauses of %d clauses\n", _singular_clauses.size(), _formula.size());
    }

    return DPLLSolver::ERROR::OK;
}

DPLLSolver::STEP_RESULT DPLLSolver::_removeTrivialClauses()
{
    dprintf("┍━Removing trivial clauses from formula of %d clauses\n", _formula.size());
    _formula.remove_if([](Clause* clause) { return clause->Trivial(); });
    dprintf("┕━After removing formula consists of %d clauses\n", _formula.size());

    return _formula.size() == 0 ? STEP_RESULT::FORMULA_EXHAUSTED : STEP_RESULT::OK;
}

DPLLSolver::STEP_RESULT DPLLSolver::_propagateUnit()
{
    while (_singular_clauses.size() > 0)
    {
        Literal propagating_literal = _singular_clauses.back()->SingleLiteral();
        _singular_clauses.pop_back();

        // Removing all clauses that contains 
        dprintf("┍━Propagating literal '%d' in formula of %d clauses\n", propagating_literal, _formula.size());
        _formula.remove_if(
            [propagating_literal](Clause* clause) 
            { 
                return clause->Contains(propagating_literal); 
            }
        );
        dprintf("│     After removing clauses formula contains %d clauses\n", _formula.size())

        // Removing contra-literal from all formulas that contains it
        for (auto clause : _formula)
        {
            if (clause->Contains(-propagating_literal))
            {
                dprintf("│     Removing literal '%d' from clause [%s]\n", -propagating_literal, clause->ToString().c_str())
                clause->RemoveLiteral(-propagating_literal);

                if (clause->Singular())
                {
                    dprintf("│     Added clause [%s] to singular\n", clause->ToString().c_str());
                    _singular_clauses.push_back(clause);
                }

                else if (clause->Empty())
                {
                    dprintf("│     Empty clause created while removing literal '%d'\n", propagating_literal);
                    dprintf("┕━Early exit (empty clause)\n", nullptr);
                    return STEP_RESULT::CLAUSE_EXHAUSTED;
                }
            }
        }

        dprintf("┕━Everything done for literal '%d'\n", propagating_literal);
    }

    return _formula.size() == 0 ? STEP_RESULT::FORMULA_EXHAUSTED : STEP_RESULT::OK;
}

void DPLLSolver::_applyLiteralsSummary()
{
    _literal_storage.Reset();

    for (auto clause : _formula)
    {
        for (auto literal : clause->literals)
            _literal_storage.SetUsage(literal);
    }
}

DPLLSolver::STEP_RESULT DPLLSolver::_removePureClauses()
{
    dprintf("┍━Removing clauses with pure literals\n", nullptr);
    _applyLiteralsSummary();

    while (_literal_storage.PureCount() > 0)
    {
        dprintf("│     Found %d pure literals in formula of %d clauses\n", _literal_storage.PureCount(), _formula.size());
        for (Literal literal = 1; literal <= _variables_count; literal++)
        {
            if (_literal_storage.Pure(literal))
                _formula.remove_if(
                    [literal](Clause* clause) 
                    { 
                        return clause->Contains(literal) or clause->Contains(-literal); 
                    }
                );
        }
        dprintf("│     After removing formula contains of %d clauses\n", _formula.size());

        _applyLiteralsSummary();
    }
    dprintf("┕━All pure literals removed. Formula contains %d clauses\n", _formula.size());

    return _formula.size() == 0 ? STEP_RESULT::FORMULA_EXHAUSTED : STEP_RESULT::OK;
}

DPLLSolver::STATUS DPLLSolver::solve()
{
    STEP_RESULT res = STEP_RESULT::OK;
    if (STEP_RESULT::OK != (res = _removeTrivialClauses())) goto exit;
    if (STEP_RESULT::OK != (res = _propagateUnit())) goto exit;
    if (STEP_RESULT::OK != (res = _removePureClauses())) goto exit;

exit:
    debug
    {
        int i = 1;
        dprintf("┍━Final formula (res = %s)\n", res == STEP_RESULT::OK ? "OK" : 
                                                res == STEP_RESULT::FORMULA_EXHAUSTED ? "FORMULA_EXHAUSTED" :
                                                res == STEP_RESULT::CLAUSE_EXHAUSTED ? "CLAUSE_EXHAUSTED" :
                                                "Unknown");
        for (auto clause : _formula)
            dprintf("│     Clause %d: [%s] (size: %d)\n", i++, clause->ToString().c_str(), clause->Size());

        dprintf("┕━Formula contains %d singular clauses of %d clauses\n", _singular_clauses.size(), _formula.size());
    }

    switch (res)
    {
        case STEP_RESULT::FORMULA_EXHAUSTED:
            return STATUS::SAT;
        case STEP_RESULT::CLAUSE_EXHAUSTED:
            return STATUS::UNSAT;
        default:
            return _formula.size() == 0 ? STATUS::SAT : STATUS::UNSAT;
    }

    return DPLLSolver::STATUS::UNKNOWN;
}

DPLLSolver::~DPLLSolver()
{
    if (_clauses)
    {
        for (int i = 0; i < _clauses_count; i++)
        {
            if (_clauses[i])
            {
                delete _clauses[i];
                _clauses[i] = nullptr;
            }
        }
        delete[] _clauses;
        _clauses = nullptr;
        _clauses_count = 0;
    }

    if (_cnf_text)
    {
        delete[] _cnf_text;
        _cnf_text = nullptr;
    }
}
