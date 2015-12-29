

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void version()
{
    fprintf( stdout, "Slow-Server version 0.01\n" );
}

void usage()
{
    fprintf( stdout, "\n" );
    fprintf( stdout, "使い方\n" );
    fprintf( stdout, "./Slow-Server --player1 EXE1 --player2 EXE2\n" );
    fprintf( stdout, "\n" );
    fprintf( stdout, "オプション\n" );
    fprintf( stdout, " --player1 プレイヤー1の実行ファイルを指定.\n" );
    fprintf( stdout, " --player2 プレイヤー2の実行ファイルを指定.\n" );
    fprintf( stdout, " --version バージョン情報表示.\n" );
    fprintf( stdout, " --verbose 動作を出力.\n" );
    fprintf( stdout, "\n" );
}

int main( const int argc, const char *argv[] )
{
    // get options.
    bool option_verbose = false;
    const char *option_player1 = NULL;
    const char *option_player2 = NULL;
    
    for ( int i = 1; i < argc; i++ ) {
        if ( i+1 < argc && strcmp( argv[i], "--player1" ) == 0 ) {
            option_player1 = argv[++i];
        } else if ( i+1 < argc && strcmp( argv[i], "--player2" ) == 0 ) {
            option_player2 = argv[++i];
        } else if ( strcmp( argv[i], "--version" ) == 0 ) {
            version();
            return EXIT_SUCCESS;
        } else if ( strcmp( argv[i], "--verbose" ) == 0 ) {
            option_verbose = true;
        } else {
            fprintf( stdout, "エラー: 引数 %s は解釈できません.\n", argv[i] );
            usage();
            return EXIT_FAILURE;
        }
    }
    
    // validate options.
    if ( ! option_player1 ) {
        fprintf( stdout, "エラー: 引数 --player1 を与えてください.\n" );
        usage();
        return EXIT_FAILURE;
    }
    
    if ( ! option_player2 ) {
        fprintf( stdout, "エラー: 引数 --player2 を与えてください.\n" );
        usage();
        return EXIT_FAILURE;
    }
    
    // print options.
    if ( option_verbose ) {
        fprintf( stdout, "オプション\n" );
        fprintf( stdout, " --player1 %s\n", option_player1 );
        fprintf( stdout, " --player2 %s\n", option_player2 );
        fprintf( stdout, " --verbose %s\n", option_verbose ? "true" : "false" );
    }
    
    
    return 0;
}
