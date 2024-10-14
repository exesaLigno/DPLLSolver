#include "dprintf.hxx"
#include <cstdint>
#include <fstream>
#include <cstring>
#include <vector>

class DPLLSolver
{
public:

    enum class RULE : uint8_t
    {
        EMPTY = 0,
        TAUTOLOGIES_REMOVAL = 1,
        UNIT_PROPAGATE = 2,
        PURE_EXCLUSION = 4,
        MASK = 7
    };

    enum class ERROR : uint8_t
    {
        OK, FILE_NOT_FOUND, CORRUPTED_DATA, UNSUPPORTED_FORMAT, MULTIPLE_HEADERS
    };

    enum class STATUS : uint8_t
    {
        UNKNOWN, SAT, UNSAT
    };

    DPLLSolver();
    DPLLSolver(RULE rules);
    ERROR loadDIMACS(char filename[]);
    STATUS solve();
    ~DPLLSolver();


private:

    RULE _rules = RULE::EMPTY;
    
    char* _cnf_text = nullptr;

    int _variables_count = 0;
    int _clauses_count = 0;
    char** _clauses = nullptr;

    int _singular_clauses_count = 0;
    char** _singular_clauses = nullptr;

    void _removeTrivialClauses();

    bool _removeSingular(char* literal);
    bool _propagateUnit();

    static long _readFile(char filename[], char** destination);
    static int _clauseSize(const char* clause);
    static void _removeContraLiteralFromClause(char* clause, char* literal);
    static bool _clauseContainsLiteral(char* clause, char* literal);
    static bool _clauseContainsContraLiteral(char* clause, char* literal);
    static bool _isTrivialClause(char* clause);
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

bool DPLLSolver::_clauseContainsLiteral(char* clause, char* literal)
{
    int literal_len = strchr(literal, ' ') - literal;
    char* clause_literal_start = clause;
    while (clause_literal_start)
    {
        char* end = strchr(clause_literal_start, ' ');
        if (!end) break;

        int clause_literal_len = end - clause_literal_start;
        if (clause_literal_len == literal_len and !strncmp(clause_literal_start, literal, literal_len))
            return true;

        while (*end == ' ') end++;
        clause_literal_start = end;
    }

    return false;
}

bool DPLLSolver::_clauseContainsContraLiteral(char* clause, char* literal)
{
    if (*literal == '-')
        return _clauseContainsLiteral(clause, literal + 1);

    else
    {
        // Dirty hack: in our case literal always have previous sym allocated in ram, so...
        char tmp = literal[-1];
        literal[-1] = '-';
        bool result = _clauseContainsLiteral(clause, literal - 1);
        literal[-1] = tmp;
        return result;
    }
}

void DPLLSolver::_removeContraLiteralFromClause(char* clause, char* literal)
{
    int literal_len = strchr(literal, ' ') - literal;
    bool is_negative = *literal == '-';
    char tmp_end = literal[literal_len];
    literal[literal_len] = '\0';

    char* removing_part = strstr(clause, is_negative ? literal + 1 : literal);
    while (removing_part != nullptr)
    {
        if (removing_part[is_negative ? literal_len - 1 : literal_len] == ' ' and
            ((is_negative and (removing_part == clause or removing_part[-1] == ' ')) or 
            (not is_negative and removing_part[-1] == '-')))
        {
            char* old_p = removing_part + (is_negative ? literal_len : literal_len + 1);
            char* new_p = is_negative ? removing_part : removing_part - 1;
            while (*old_p != '\0')
            {
                char tmp = *old_p;
                *(old_p++) = '\0';
                *(new_p++) = tmp;
            }
            while (*new_p != '\0')
                *(new_p++) = '\0';
            break;
        }

        removing_part = strstr(removing_part + 1, is_negative ? literal + 1 : literal);
    }

    literal[literal_len] = tmp_end;
}

int DPLLSolver::_clauseSize(const char* clause)
{
    int size = 0;
    bool in_variable = false;
    for (int i = 0; clause[i] != '\0'; i++)
    {
        if (not in_variable and clause[i] == '0') break;
        else if (not in_variable and ((clause[i] >= '0' and clause[i] <= '9') or clause[i] == '-'))
        {
            size++;
            in_variable = true;
        }
        else if (in_variable and (clause[i] == ' ' or clause[i] == '\t'))
            in_variable = false;
    }

    return size;
}

bool DPLLSolver::_isTrivialClause(char* clause)
{
    std::vector<int> mentioned_literals;

    bool in_variable = false;
    char* literal_start = nullptr;
    for (int i = 0; clause[i] != '\0'; i++)
    {
        if (not in_variable and clause[i] == '0') break;

        // One of contraliterals must be with leading '-'
        else if (not in_variable and ((clause[i] >= '0' and clause[i] <= '9') or clause[i] == '-'))
        {
            literal_start = clause + i;
            in_variable = true;
        }
        else if (in_variable and (clause[i] == ' ' or clause[i] == '\t'))
        {
            char tmp = clause[i];
            clause[i] = '\0';
            int current = atoi(literal_start);
            clause[i] = tmp;

            for (int mentioned : mentioned_literals)
            {
                if (mentioned + current == 0)
                    return true;
            }

            mentioned_literals.push_back(current);
            in_variable = false;
        }
    }

    return false;
}

DPLLSolver::DPLLSolver() : _rules(DPLLSolver::RULE::EMPTY) {}

DPLLSolver::DPLLSolver(RULE rules) : _rules(rules) {}

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
                _clauses = new char*[_clauses_count]{0};
                _singular_clauses = new char*[_clauses_count]{0};
            }
            else if (clause_counter < _clauses_count)
            {
                if (!header_presented)
                {
                    dprintf("No header presented in DIMACS!\n", nullptr);
                    return DPLLSolver::ERROR::UNSUPPORTED_FORMAT;
                }

                _clauses[clause_counter++] = _cnf_text + current_line;
                if (_clauseSize(_cnf_text + current_line) == 1)
                    _singular_clauses[_singular_clauses_count++] = _cnf_text + current_line;
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
        for (int i = 0; i < _clauses_count; i++)
            dprintf("Clause %d: '%s' (size: %d)\n", i + 1, _clauses[i], _clauseSize(_clauses[i]));

        dprintf("Formula contains %d singular clauses of %d clauses\n", _singular_clauses_count, _clauses_count);
    }

    return DPLLSolver::ERROR::OK;
}

void DPLLSolver::_removeTrivialClauses()
{
    int new_clauses_count = 0;

    for (int old_idx = 0, new_idx = 0; old_idx < _clauses_count; old_idx++)
    {
        if (_isTrivialClause(_clauses[old_idx]))
        {
            dprintf("Removed trivial clause %s\n", _clauses[old_idx]);
            continue; // Just skipping such clauses (they will be deleted)
        }

        new_clauses_count++;
        char* tmp = _clauses[old_idx];
        _clauses[old_idx] = nullptr;
        _clauses[new_idx++] = tmp;
    }

    _clauses_count = new_clauses_count;
}

bool DPLLSolver::_removeSingular(char* literal)
{
    int new_clauses_count = 0;

    for (int old_idx = 0, new_idx = 0; old_idx < _clauses_count; old_idx++)
    {
        if (_clauseContainsLiteral(_clauses[old_idx], literal)) 
            continue; // Just skipping such clauses (they will be deleted)

        else if (_clauseContainsContraLiteral(_clauses[old_idx], literal))
        {
            dprintf("%s contains contraliteral for %s\n", _clauses[old_idx], literal);
            _removeContraLiteralFromClause(_clauses[old_idx], literal);
            if (_clauseSize(_clauses[old_idx]) == 1)
                _singular_clauses[_singular_clauses_count++] = _clauses[old_idx];
            dprintf("New clause: %s\n", _clauses[old_idx]);

            if (_clauseSize(_clauses[old_idx]) == 0)
                return true;
        }

        new_clauses_count++;
        char* tmp = _clauses[old_idx];
        _clauses[old_idx] = nullptr;
        _clauses[new_idx++] = tmp;
    }

    _clauses_count = new_clauses_count;

    return false;
}

bool DPLLSolver::_propagateUnit()
{
    while (_singular_clauses_count > 0)
    {
        _singular_clauses_count--;
        char* clause_to_propagate = _singular_clauses[_singular_clauses_count];
        _singular_clauses[_singular_clauses_count] = nullptr;

        dprintf("Removing clause '%s'\n", clause_to_propagate);
        bool empty_clause_presented = _removeSingular(clause_to_propagate);
        if (empty_clause_presented)
        {
            dprintf("Empty clause appeared while removing %s\n", clause_to_propagate);
            return empty_clause_presented;
        }
        dprintf("After removing formula contains %d singular clauses of %d clauses\n", _singular_clauses_count, _clauses_count);

        debug
        {
            for (int i = 0; i < _clauses_count; i++)
                dprintf("Clause %d: '%s' (size: %d)\n", i + 1, _clauses[i], _clauseSize(_clauses[i]));

            dprintf("Formula contains %d singular clauses of %d clauses\n", _singular_clauses_count, _clauses_count);
        }
    }

    return false;
}

DPLLSolver::STATUS DPLLSolver::solve()
{
    _removeTrivialClauses();
    bool empty_clause_presented = _propagateUnit();

    debug
    {
        for (int i = 0; i < _clauses_count; i++)
            dprintf("Clause %d: '%s' (size: %d)\n", i + 1, _clauses[i], _clauseSize(_clauses[i]));

        dprintf("Formula contains %d singular clauses of %d clauses\n", _singular_clauses_count, _clauses_count);
    }

    if (_clauses_count == 0 and not empty_clause_presented)
        return DPLLSolver::STATUS::SAT;
    else
        return DPLLSolver::STATUS::UNSAT;
    return DPLLSolver::STATUS::UNKNOWN;
}

DPLLSolver::~DPLLSolver()
{
    if (_clauses)
    {
        delete[] _clauses;
        _clauses = nullptr;
    }

    if (_cnf_text)
    {
        delete[] _cnf_text;
        _cnf_text = nullptr;
    }

    if (_singular_clauses)
    {
        delete[] _singular_clauses;
        _singular_clauses = nullptr;
    }
}
