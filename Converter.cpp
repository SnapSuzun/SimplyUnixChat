#include "Converter.h"

void Converter::itoa(int i, char* buffer)
{
    buffer[0] = (unsigned char)(i >> 24);
    buffer[1] = (unsigned char)(i >> 16);
    buffer[2] = (unsigned char)(i >> 8);
    buffer[3] = (unsigned char)i;
}

void Converter::itoa(unsigned short int i, char* buffer)
{
    buffer[0] = (unsigned char)(i >> 8);
    buffer[1] = (unsigned char)i;
}

int Converter::atoi(char* buffer)
{
    int i = (int)buffer[0] << 24;
    i |= (int)buffer[1] << 16;
    i |= (int)buffer[2] << 8;
    i |= (int)buffer[3];
    return i;
}

unsigned short int Converter::atosi(char* buffer)
{
    unsigned short int i = (unsigned short int)buffer[0] << 8;
    i |= (unsigned short int)buffer[1];
    return i;
}
