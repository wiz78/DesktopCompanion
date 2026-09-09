#ifndef TEST_MOCKS_WIRE_H
#define TEST_MOCKS_WIRE_H

#include <cstdint>

class TwoWire
{
public:
    void begin( int sda = 0, int scl = 0 )
    {
        (void)sda;
        (void)scl;
    }

    void setClock( uint32_t freq )
    {
        (void)freq;
    }
};

inline TwoWire Wire;

#endif // TEST_MOCKS_WIRE_H
