#pragma once
#include "cnf.hxx"
#include "literal.hxx"
#include "dprintf.hxx"
#include <list>
#include <fstream>

class DIMACS
{
public:
    static CNF ReadFromFile(const char name[]);

private:
    static long _readFile(const char filename[], char** destination);
    static void _allocateCNF(CNF& cnf, const char* clauses_text, const uint32_t clauses_count);
    static int _appendClause(Literal* destionation, char* clause_string);
};

CNF DIMACS::ReadFromFile(const char filename[])
{
    CNF cnf;
    
    char* cnf_text = nullptr;
    long text_length = _readFile(filename, &cnf_text);
    
    uintptr_t current_line = 0;
    bool header_presented = false;
    uintptr_t clause_counter = 0;

    for (uintptr_t current_sym = 0; current_sym < text_length; current_sym++)
    {
        // Parsing line when it's end reached
        if (cnf_text[current_sym] == '\n' or cnf_text[current_sym] == '\0')
        {
            cnf_text[current_sym] = '\0';

            if (current_sym <= current_line); // Skipping empty lines
            else if (cnf_text[current_line] == 'c'); // Skipping commented lines
            else if (cnf_text[current_line] == 'p') // Parsing cnf info (p cnf <variables> <clauses>)
            {
                if (header_presented)
                    dprintf("Multiple headers presented in file!\n", nullptr);

                header_presented = true;
                sscanf(cnf_text + current_line, "p cnf %d %d", &cnf._variables_count, &cnf._clauses_count);
                dprintf("Readed header: %d vars and %d clauses\n", cnf._variables_count, cnf._clauses_count);
                
                _allocateCNF(cnf, cnf_text + current_sym + 1, cnf._clauses_count);
                cnf._cnf_data_size = 0;
            }
            else if (clause_counter < cnf._clauses_count)
            {
                if (!header_presented)
                    dprintf("No header presented in DIMACS!\n", nullptr);

                cnf._cnf_data_size += _appendClause(cnf._cnf_data + cnf._cnf_data_size, cnf_text + current_line);
                clause_counter++;
            }

            current_line = current_sym + 1;
            while (cnf_text[current_line] == ' ' or
                   cnf_text[current_line] == '\n' or
                   cnf_text[current_line] == '\t')
                current_line++;
        }
    }

    delete[] cnf_text;

    return cnf;
}

long DIMACS::_readFile(const char filename[], char** destination)
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

void DIMACS::_allocateCNF(CNF& cnf, const char* clauses, const uint32_t clauses_count)
{
    uintptr_t size = 0;
    uint32_t clause_counter = 0;

    bool in_number = false;
    bool in_clause = false;

    for (uintptr_t current_sym = 0; clauses[current_sym] != '\0'; current_sym++)
    {
        if (clauses[current_sym] == 'c' or clauses[current_sym] == 'p')
        {
            while (clauses[current_sym] != '\0' and clauses[current_sym] != '\n')
                current_sym++;
            if (clauses[current_sym - 1] == '\0')
                break;
        }

        if (not in_number and ((clauses[current_sym] >= '0' and clauses[current_sym] <= '9') or clauses[current_sym] == '-'))
        {
            if (not in_clause) 
            {
                in_clause = true;
                clause_counter++;
                if (clause_counter > clauses_count)
                    break;
            }
            in_number = true;
            size++;
        }

        if (in_number and not ((clauses[current_sym] >= '0' and clauses[current_sym] <= '9') or clauses[current_sym] == '-'))
            in_number = false;

        if (in_clause and clauses[current_sym] == '\n')
            in_clause = false;
    }

    dprintf("Counted %d literals and %d clauses\n", size, clause_counter);

    if (clause_counter < clauses_count - 1)
        dprintf("DIMACS reader stopped before reading all clauses on clause %d!\n", clause_counter + 1);

    dprintf("Allocating room for %d literals in cnf\n", size);
    cnf._cnf_data_size = size;
    cnf._cnf_data = new Literal[cnf._cnf_data_size] { EmptyLiteral };
}

int DIMACS::_appendClause(Literal* destination, char* clause_string)
{
    int length = strlen(clause_string) + 1;
    bool in_variable = false;
    int literal_start = -1;
    int i = 0;
    int dst_idx = 0;

    while (i < length)
    {
        if (not in_variable and clause_string[i] == '0') break; // Check if line is zero-terminated

        else if (not in_variable and ((clause_string[i] >= '0' and clause_string[i] <= '9') or clause_string[i] == '-'))
        {
            literal_start = i;
            in_variable = true;
        }

        else if (in_variable and (clause_string[i] == ' ' or clause_string[i] == '\t' or clause_string[i] == '\0'))
        {
            char tmp = clause_string[i];
            clause_string[i] = '\0';
            int current = atoi(clause_string + literal_start);
            clause_string[i] = tmp;

            destination[dst_idx++] = current;
            in_variable = false;
        }

        i++;
    }

    destination[dst_idx++] = EmptyLiteral;

    return dst_idx;
}
