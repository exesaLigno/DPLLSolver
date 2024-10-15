#pragma once
#include <cstdint>
#include "literal.hxx"
#include "dprintf.hxx"

typedef uint8_t byte;

#define capacityof(type) (sizeof(type) * 4)

class LiteralStorage
{
public:
    LiteralStorage();
    LiteralStorage(int size);
    LiteralStorage(LiteralStorage&) = delete;
    ~LiteralStorage();

    void Allocate(int size);

    void Reset();
    void SetUsage(Literal literal);
    byte GetUsage(Literal literal);
    bool Pure(Literal literal);
    int PureCount();

private:

    int _size = 0;
    int _usage_data_bytes_size = 0;
    byte* _usage_data = nullptr;

    int _pure_count = 0;
};

LiteralStorage::LiteralStorage() { }

LiteralStorage::LiteralStorage(int size)
{
    Allocate(size);
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

void LiteralStorage::Allocate(int size)
{
    _size = size;
    _usage_data_bytes_size = _size / capacityof(byte) + ((size % capacityof(byte)) == 0 ? 0 : 1);
    _usage_data = new byte[_usage_data_bytes_size] { 0 };
}

void LiteralStorage::Reset()
{
    _pure_count = 0;
    for (int i = 0; i < _usage_data_bytes_size; i++)
        _usage_data[i] = (byte) 0b00000000u;
}

void LiteralStorage::SetUsage(Literal literal)
{
    bool negative = literal < 0;
    Literal original_literal = negative ? -literal : literal;

    int data_idx = (original_literal - 1) / capacityof(byte);
    int elem_idx = (original_literal - 1) % capacityof(byte);
    int shift = ((capacityof(byte) - 1) - elem_idx) * 2;
    byte elem_mask = 0x3 << shift;
    byte flag_mask = negative ? 0b01010101 : 0b10101010;

    byte old_elem = _usage_data[data_idx];
    byte new_elem = old_elem | (elem_mask & flag_mask);
    _usage_data[data_idx] = new_elem;

    byte old_usage = (old_elem & elem_mask) >> shift;
    byte new_usage = (new_elem & elem_mask) >> shift;

    bool was_pure = (old_usage == 0b01 or old_usage == 0b10);
    bool is_pure = (new_usage == 0b01 or new_usage == 0b10);

    _pure_count += (was_pure and not is_pure) ? -1 :
                   (not was_pure and is_pure) ? 1 :
                   0;
}

byte LiteralStorage::GetUsage(Literal literal)
{
    bool negative = literal < 0;
    Literal original_literal = negative ? -literal : literal;

    int data_idx = (original_literal - 1) / capacityof(byte);
    int elem_idx = (original_literal - 1) % capacityof(byte);
    int shift = ((capacityof(byte) - 1) - elem_idx) * 2;
    byte elem_mask = 0x3 << shift;

    return (_usage_data[data_idx] & elem_mask) >> shift;
}

bool LiteralStorage::Pure(Literal literal)
{
    byte usage_info = GetUsage(literal);

    return usage_info == 0b01 or usage_info == 0b10;
}

int LiteralStorage::PureCount()
{
    return _pure_count;
}
