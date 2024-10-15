#include <cstdint>
#include <list>
#include <cstring>
#include <cstdlib>
#include <cstdio>

typedef int32_t Literal;

class Clause
{
public:
    Clause(); /// Initializes empty clause
    Clause(char* clause_string, const int clause_string_len); /// Initializing clause based on char*
    Clause(Clause&) = delete;
    //~Clause(); /// Clause destructor

    int Size();
    bool Contains(Literal other);
    bool Contains(Clause& other);
    bool Trivial();
    bool Empty();

    void RemoveLiteral(Literal literal);

    void Print(bool zero_terminated);

private:
    std::list<Literal> literals;
};

Clause::Clause() {}

Clause::Clause(char* clause_string, const int clause_string_len = 0)
{
    int length = clause_string_len ? clause_string_len : strlen(clause_string) + 1;
    bool in_variable = false;
    int literal_start = -1;
    int i = 0;
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

            literals.push_back(current);
            in_variable = false;
        }

        i++;
    }
}

int Clause::Size()
{
    return literals.size();
}

bool Clause::Contains(Literal other)
{
    for (auto literal : literals)
    {
        if (literal == other)
            return true;
    }

    return false;
}

bool Clause::Contains(Clause& other)
{
    for (auto other_literal : other.literals)
    {
        if (not Contains(other_literal))
            return false;
    }

    return true;
}

bool Clause::Trivial()
{
    for (auto literal : literals)
    {
        if (Contains(-literal))
            return true;
    }

    return false;
}

bool Clause::Empty()
{
    return literals.empty();
}

void Clause::RemoveLiteral(Literal literal)
{
    literals.remove(literal);
}

void Clause::Print(bool zero_terminated = false)
{
    for (auto literal : literals)
        printf("%d ", literal);

    printf(zero_terminated ? "0\n" : "\n");
}

