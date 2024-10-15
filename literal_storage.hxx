#pragma once
#include <cstdint>
#include "literal.hxx"

typedef uint8_t byte;

class LiteralStorage
{
public:
    LiteralStorage(int size);
    LiteralStorage(LiteralStorage&) = delete;
    ~LiteralStorage();

    void Reset();
    void SetPosUsage(Literal literal);
    void SetNegUsage(Literal literal);
    bool IsPosUsed(Literal literal);
    bool IsNegUsed(Literal literal);
    bool IsPure(Literal literal);

private:

    int _size = 0;
    int _usage_data_bytes_size = 0;
    byte* _usage_data = nullptr;
};


LiteralStorage::LiteralStorage(int size)
{
    _size = size;
    _usage_data_bytes_size = _size / 4 + (size % 4 == 0 ? 0 : 1);
    _usage_data = new byte[_usage_data_bytes_size] { 0 };
}

LiteralStorage::~LiteralStorage()
{
    if (_usage_data)
    {
        delete[] _usage_data;
        _usage_data_bytes_size = 0;
        _size = 0;
    }
}

void LiteralStorage::Reset()
{
    for (int i = 0; i < _usage_data_bytes_size; i++)
        _usage_data[i] = 0b00000000u;
}

void LiteralStorage::SetPosUsage(Literal literal)
{
    int data_idx = literal / 4;
    int elem_idx = literal % 4;

    byte old_elem = _usage_data[data_idx];
}
