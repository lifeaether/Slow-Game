

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


#include <unistd.h>

// options
static bool option_verbose = false;
static const char *option_player1 = NULL;
static const char *option_player2 = NULL;

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

int run_player( const char *name, const int pipe_in, const int pipe_out )
{
    int exit_code = EXIT_FAILURE;

    // replace stdin, stdout to pipes.
    if ( dup2( pipe_in, STDIN_FILENO ) == -1 ) {
        fprintf( stderr, "error[%s]: dup2 に失敗しました(%d).\n", name, __LINE__ );
        goto CLEAN;
    }
    if ( dup2( pipe_out, STDOUT_FILENO ) == -1 ) {
        fprintf( stderr, "error[%s]: dup2 に失敗しました(%d).\n", name, __LINE__ );
        goto CLEAN;
    }
    

    exit_code = EXIT_SUCCESS;
CLEAN:
    // cleanup player.
    if ( option_verbose ) fprintf( stderr, "[%s]プレイヤーを終了します...\n", name );
    
    if ( close( pipe_in ) == -1 ) {
        fprintf( stderr, "warn[%s]: close に失敗しました(%d).\n", name, __LINE__ );
    }
    if ( close( pipe_out ) == -1 ) {
        fprintf( stderr, "warn[%s]: close に失敗しました(%d).\n", name, __LINE__ );
    }

    return exit_code;
}

int main( const int argc, const char *argv[] )
{
    // get options.
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
            fprintf( stdout, "error: 引数 %s は解釈できません.\n", argv[i] );
            usage();
            return EXIT_FAILURE;
        }
    }
    
    // validate options.
    if ( ! option_player1 ) {
        fprintf( stdout, "error: 引数 --player1 を与えてください.\n" );
        usage();
        return EXIT_FAILURE;
    }
    
    if ( ! option_player2 ) {
        fprintf( stdout, "error: 引数 --player2 を与えてください.\n" );
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
    
    
    // setup pipe.
    if ( option_verbose ) fprintf( stderr, "パイプを準備します...\n" );
    
    int fd_p1[2];
    int fd_p2[2];

    if ( pipe( fd_p1 ) == -1 ) {
        fprintf( stderr, "error: pipe に失敗しました(%d).\n", __LINE__ );
        return EXIT_FAILURE;
    }

    if ( pipe( fd_p2 ) == -1 ) {
        close( fd_p1[0] );
        close( fd_p1[1] );
        fprintf( stderr, "error: pipe に失敗しました(%d).\n", __LINE__ );
        return EXIT_FAILURE;
    }
    
    
    // lauch process.
    if ( option_verbose ) fprintf( stderr, "プロセスを複製します...\n" );

    pid_t pid_p1;
    pid_t pid_p2;
    
    pid_p1 = fork();
    if ( pid_p1 == -1 ) {
        fprintf( stderr, "error: fork に失敗しました(%d).\n", __LINE__ );
        return EXIT_FAILURE;
    }
    
    if ( pid_p1 == 0 ) {
        // player1 child process.
        if ( option_verbose ) fprintf( stderr, "[%s]を実行します...\n", option_player1 );
        
        // close other pipes.
        if ( close( fd_p2[0] ) == -1 ) {
            fprintf( stderr, "warn: close に失敗しました(%d).\n", __LINE__ );
        }
        if ( close( fd_p2[1] ) == -1 ) {
            fprintf( stderr, "warn: close に失敗しました(%d).\n", __LINE__ );
        }
        
        return run_player( option_player1, fd_p1[0], fd_p1[1] );
    }
    
    pid_p2 = fork();
    if ( pid_p2 == -1 ) {
        fprintf( stderr, "error: fork に失敗しました(%d).\n", __LINE__ );
        return EXIT_FAILURE;
    }
    
    if ( pid_p2 == 0 ) {
        // player2 child process.
        if ( option_verbose ) fprintf( stderr, "[%s]を実行します...\n", option_player2 );

        // close other pipes.
        if ( close( fd_p1[0] ) == -1 ) {
            fprintf( stderr, "warn: close に失敗しました(%d).\n", __LINE__ );
        }
        if ( close( fd_p1[1] ) == -1 ) {
            fprintf( stderr, "warn: close に失敗しました(%d).\n", __LINE__ );
        }

        return run_player( option_player2, fd_p2[0], fd_p2[1] );
    }
    
    // game sever parent process.
    if ( option_verbose ) fprintf( stderr, "ゲームを初期化します...\n" );
    
    int exit_code = EXIT_FAILURE;
    
    exit_code = EXIT_SUCCESS;
CLEAN:
    // cleanup.
    if ( option_verbose ) fprintf( stderr, "ゲームを終了します...\n" );
    
    if ( close( fd_p1[0] ) == -1 ) {
        fprintf( stderr, "warn: close に失敗しました(%d).\n", __LINE__ );
    }
    if ( close( fd_p1[1] ) == -1 ) {
        fprintf( stderr, "warn: close に失敗しました(%d).\n", __LINE__ );
    }
    if ( close( fd_p2[0] ) == -1 ) {
        fprintf( stderr, "warn: close に失敗しました(%d).\n", __LINE__ );
    }
    if ( close( fd_p2[1] ) == -1 ) {
        fprintf( stderr, "warn: close に失敗しました(%d).\n", __LINE__ );
    }

    return exit_code;
}
