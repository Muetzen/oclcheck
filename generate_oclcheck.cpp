#include "parse_header.h"
#include <iostream>

int main (int argc, char **argv)
{
    ParseHeader ph;

    std::string ret = ph.parse ("/usr/include/CL/cl.h");
    if ( ! ret.empty ())
    {
        std::cerr << ret << "\n";
        return 1;
    }

    ph.printErrorStringMethod ();
    ph.printClTypeMethods ();
    ph.printMethods ();

    return 0;
}
