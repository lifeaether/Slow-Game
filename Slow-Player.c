
#include <stdio.h>

int main( const int argc, const char *argv[] )
{
    char line[256];
    fgets( line, sizeof(line), stdin );
    
    fprintf( stderr, "%s", line );
    
    fprintf( stdout, "Hello!\n" );
    fflush( stdout );
    
    fgets( line, sizeof(line), stdin );
    return 0;
}
