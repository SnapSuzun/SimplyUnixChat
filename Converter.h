#pragma once

class Converter {
public:
    static void itoa(int i, char* buffer);
    static void itoa(unsigned short int i, char* buffer);
    static int atoi(char* buffer);
    static unsigned short int atosi(char* buffer);
private:
    Converter() {}
    Converter(const Converter& orig) {}
    virtual ~Converter() {}
};

