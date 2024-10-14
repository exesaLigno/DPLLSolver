#include "dprintf.hxx"
#include <cstdint>
#include <fstream>

class DPLLSolver
{
public:

    enum class ERROR : uint8_t
    {
        OK, FILE_NOT_FOUND, CORRUPTED_DATA, UNSUPPORTED_FORMAT, MULTIPLE_HEADERS
    };

    enum class STATUS : uint8_t
    {
        UNKNOWN, SAT, UNSAT
    };

    // DPLLSolver();
    ERROR loadDIMACS(char filename[]);
    STATUS solve();
    ~DPLLSolver();


private:

    int _variables_count = 0;
    int _clauses_count = 0;
    char* _cnf_text = nullptr;
    char** _clauses = nullptr;

};

DPLLSolver::ERROR DPLLSolver::loadDIMACS(char filename[])
{
    FILE* file = fopen(filename, "r");
    fseek(file, 0, SEEK_END);
	long text_length = ftell(file);
	rewind(file);
	char* _cnf_text = new char[text_length + 1]{0};
    text_length = fread(_cnf_text, sizeof(char), text_length, file);
    fclose(file);
    
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
            }
            else if (clause_counter < _clauses_count)
            {
                if (!header_presented)
                {
                    dprintf("This code is unreachable!\n", nullptr);
                    return DPLLSolver::ERROR::UNSUPPORTED_FORMAT;
                }

                _clauses[clause_counter++] = _cnf_text + current_line;
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
            dprintf("Clause %d: \"%s\"\n", i + 1, _clauses[i]);
    }

    return DPLLSolver::ERROR::OK;
}

DPLLSolver::STATUS DPLLSolver::solve()
{
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
}
