

#include <stdio.h>

void version()
{
    fprintf( stdout, "Slow-Server version 0.01\n" );
}

int main( const int argc, const char *argv[] )
{
    version();
    return 0;
}
