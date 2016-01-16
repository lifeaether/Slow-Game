#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <time.h>
#include <unistd.h>

// constants.
const int32_t k_max_line = 256;

// options
static bool option_verbose = false;
static const char *option_player1 = NULL;
static const char *option_player2 = NULL;
static int32_t option_number_of_games = 1;

void version()
{
    fprintf( stdout, "Slow-Server version 0.02\n" );
}

void usage()
{
    fprintf( stdout, "\n" );
    fprintf( stdout, "使い方\n" );
    fprintf( stdout, "./Slow-Server --player1 EXE1 --player2 EXE2 --number 100\n" );
    fprintf( stdout, "\n" );
    fprintf( stdout, "オプション\n" );
    fprintf( stdout, " --player1 プレイヤー1の実行ファイルを指定.\n" );
    fprintf( stdout, " --player2 プレイヤー2の実行ファイルを指定.\n" );
    fprintf( stdout, " --number 対戦数.\n" );
    fprintf( stdout, " --version バージョン情報表示.\n" );
    fprintf( stdout, " --verbose 動作を出力.\n" );
    fprintf( stdout, "\n" );
}

int run_player( const char *filename, const int pipe_in, const int pipe_out )
{
    int exit_code = EXIT_FAILURE;
    
    // replace stdin, stdout to pipes.
    if ( dup2( pipe_in, STDIN_FILENO ) == -1 ) {
        fprintf( stderr, "error[%s]: dup2 に失敗しました(%d).\n", filename, __LINE__ );
        goto CLEAN;
    }
    if ( dup2( pipe_out, STDOUT_FILENO ) == -1 ) {
        fprintf( stderr, "error[%s]: dup2 に失敗しました(%d).\n", filename, __LINE__ );
        goto CLEAN;
    }
    
    // execute player
    if ( execl( filename, filename ) == -1 ) {
        fprintf( stderr, "error[%s]: execl に失敗しました(%d).\n", filename, __LINE__ );
        goto CLEAN;
    } else {
        // nerver execute following code.
    }
    
    exit_code = EXIT_SUCCESS;
CLEAN:
    // cleanup player.
    if ( option_verbose ) fprintf( stderr, "[%s]プレイヤーを終了します...\n", filename );
    
    if ( close( pipe_in ) == -1 ) {
        fprintf( stderr, "warn[%s]: close に失敗しました(%d).\n", filename, __LINE__ );
    }
    if ( close( pipe_out ) == -1 ) {
        fprintf( stderr, "warn[%s]: close に失敗しました(%d).\n", filename, __LINE__ );
    }
    
    return exit_code;
}

bool write_line( const int fd, const char *line )
{
    const size_t length = strlen( line );
    return write( fd, line, length ) == length && write( fd, "\n", 1 ) == 1;
}

bool read_line( const int fd, char *line, const size_t max_of_line )
{
    size_t bytes = 0;
    for ( ; bytes < max_of_line; ++line, ++bytes ) {
        if ( read( fd, line, 1 ) != 1 ) return false;
        if ( *line == '\n' ) break;
    }
    *line = '\0';
    return true;
}

bool read_to_lineend( const int fd )
{
    char c = 0;
    while ( c != '\n' ) {
        if ( read( fd, &c, 1 ) != 1 ) false;
    }
    return true;
}

typedef enum {
    play_operation_null = 0,
    play_operation_error,
    play_operation_invalid,
    play_operation_pass,
    play_operation_draw,
    play_operation_put_left,
    play_operation_put_right
} play_operation;

typedef struct {
    play_operation operation;
    int16_t card;
} play_action;

play_action play_action_make( const play_operation operation, const int16_t card )
{
    play_action action = { operation, card };
    return action;
}

bool is_equals_play_action( const play_action a1, const play_action a2 )
{
    return a1.operation == a2.operation && a1.card == a2.card;
}

void deck_swap( int16_t *v1, int16_t *v2 )
{
    int16_t t = *v1;
    *v1 = *v2;
    *v2 = t;
}

void deck_shuffle( int16_t *deck, const size_t size )
{
    for ( size_t i = 0; i < size; i++ ) {
        deck_swap( deck+i, deck + i + rand()%(size-i) );
    }
}

bool write_reset( const int fd, const int32_t index_of_games )
{
    if ( ! write_line( fd, "RESET" ) ) return false;
    
    char line[k_max_line];
    sprintf( line, "%d", index_of_games );
    if ( ! write_line( fd, line ) ) return false;
    
    return true;
}

bool write_sequence( const int fd, const int16_t *sequence )
{
    char line[k_max_line];
    size_t line_bytes = 0;
    for ( size_t i = 0; *sequence != 0; i++, sequence++ ) {
        line_bytes += sprintf( line + line_bytes, "%d ", *sequence );
    }
    if ( line_bytes > 0 ) {
        line[line_bytes-1] = '\0'; // delete tail space character.
    } else {
        *line = '\0';
    }
    
    return write_line( fd, line );
}

bool write_play_action( const int fd, const play_action action )
{
    if ( action.operation == play_operation_pass ) {
        if ( ! write_line( fd, "P" ) ) return false;
    } else if ( action.operation == play_operation_draw ) {
        if ( ! write_line( fd, "D" ) ) return false;
    } else if ( action.operation == play_operation_put_left ) {
        char line[k_max_line];
        sprintf( line, "L%d", action.card );
        if ( ! write_line( fd, line ) ) return false;
    } else if ( action.operation == play_operation_put_right ) {
        char line[k_max_line];
        sprintf( line, "R%d", action.card );
        if ( ! write_line( fd, line ) ) return false;
    } else if ( action.operation == play_operation_null ) {
        // no action previous.
        if ( ! write_line( fd, "" ) ) return false;
    } else {
        // error action previous.
        if ( ! write_line( fd, "" ) ) return false;
    }
    return true;
}

bool write_play( const int fd, const int32_t index_of_turn, const int16_t *hands_first, const int16_t *hands_second, const int16_t *place_left, const int16_t *place_right, const play_action action_first, const play_action action_second )
{
    if ( ! write_line( fd, "PLAY" ) ) return false;
    {
        // write turn.
        char line[k_max_line];
        sprintf( line, "%d", index_of_turn );
        if ( ! write_line( fd, line ) ) return false;
    }
    
    if ( ! write_sequence( fd, hands_first ) ) false;
    if ( ! write_sequence( fd, hands_second ) ) false;
    if ( ! write_sequence( fd, place_left ) ) false;
    if ( ! write_sequence( fd, place_right ) ) false;
    if ( ! write_play_action( fd, action_first ) ) false;
    if ( ! write_play_action( fd, action_second ) ) false;
    
    return true;
}

bool write_gameset( const int fd, const int32_t point_left, const int32_t point_right, const int32_t score_left, const int32_t score_right )
{
    if ( ! write_line( fd, "GAMESET" ) ) return false;
    
    char line[k_max_line];
    
    sprintf( line, "%d %d", point_left, point_right );
    if ( ! write_line( fd, line ) ) return false;

    sprintf( line, "%d %d", score_left, score_right );
    if ( ! write_line( fd, line ) ) return false;

    return true;
}

int32_t number_of_sequence( const int16_t *sequence )
{
    int32_t count = 0;
    while ( *(sequence++) != 0 ) count++;
    return count;
}

int32_t sum_sequence( const int16_t *sequence )
{
    int32_t sum = 0;
    while ( *sequence != 0 ) sum += *(sequence++);
    return sum;
}

bool index_of_sequence( const int16_t *sequence, const int16_t value, int16_t *index )
{
    const int16_t *it = sequence;
    while ( *it != 0 && *it != value ) it++;
    if ( *it == 0 ) {
        return false;
    } else {
        if ( index ) *index = it - sequence;
        return true;
    }
}

bool is_member_sequence( const int16_t *sequence, const int16_t member )
{
    return index_of_sequence( sequence, member, NULL );
}

void push_sequence( int16_t *sequence, const int16_t value )
{
    const int32_t count = number_of_sequence( sequence );
    memmove( sequence+1, sequence, count * sizeof(int16_t) );
    *sequence = value;
}

bool pick_sequence( int16_t *sequence, const int16_t value )
{
    int16_t index;
    if ( index_of_sequence( sequence, value, &index ) ) {
        const int32_t count = number_of_sequence( sequence );
        memmove( sequence+index, sequence+index+1, (count - index) * sizeof(int16_t) );
        return true;
    } else {
        return false;
    }
}

int16_t pop_sequence( int16_t *sequence )
{
    const int32_t count = number_of_sequence( sequence );
    assert( count > 0 );

    const int16_t value = *sequence;
    memmove( sequence, sequence+1, count * sizeof(int16_t) );
    return value;
}

play_action read_play( const int fd )
{
    play_action action = {};
    
    char line[k_max_line];
    if ( read_line( fd, line, sizeof( line ) ) ) {
        if ( strcmp( line, "P" ) == 0 ) {
            action.operation = play_operation_pass;
        } else if ( strcmp( line, "D" ) == 0  ) {
            action.operation = play_operation_draw;
        } else if ( sscanf( line, "L%hi", &action.card ) == 1 ) {
            action.operation = play_operation_put_left;
        } else if ( sscanf( line, "R%hi", &action.card ) == 1 ) {
            action.operation = play_operation_put_right;
        } else {
            action.operation = play_operation_invalid;
        }
    } else {
        action.operation = play_operation_error;
    }
    
    return action;
}

static const size_t place_action_put_candidate_max = 10;
int32_t play_action_put_candidate( play_action *candidates, int16_t *hands, int16_t *place, const play_operation operation )
{
    const play_action * const candidate_begin  = candidates;
    if ( *place == 0 ) {
        for ( int16_t *it = hands; *it != 0; it++ ) {
            *(candidates++) = play_action_make( operation, *it );
        }
    } else {
        int16_t upper = *place == 13 ? 1 : *place + 1;
        int16_t lower = *place == 1 ? 13 : *place - 1;
        if ( is_member_sequence( hands, upper ) ) {
            *(candidates++) = play_action_make( operation, upper );
        }
        if ( upper != lower && is_member_sequence( hands, lower ) ) {
            *(candidates++) = play_action_make( operation, lower );
        }
    }
    
    return (int32_t)( candidates - candidate_begin );
}

static const size_t play_action_candidate_max = 12;
int32_t play_action_candidates( play_action *candidates, const play_action previous, const int16_t *deck, int16_t* hands, const ssize_t max_hands, int16_t *place_left, int16_t *place_right )
{
    const play_action * const candidate_begin  = candidates;
    
    if ( previous.operation == play_operation_pass ) {
        for ( int16_t *it = hands; *it != 0; it++ ) {
            *(candidates++) = play_action_make( play_operation_put_left, *it );
            *(candidates++) = play_action_make( play_operation_put_right, *it );
        }
    } else {
        candidates += play_action_put_candidate( candidates, hands, place_left, play_operation_put_left );
        candidates += play_action_put_candidate( candidates, hands, place_right, play_operation_put_right );
    }
    
    if ( number_of_sequence( hands ) < max_hands && number_of_sequence( deck ) > 0 ) {
        *(candidates++) = play_action_make( play_operation_draw, 0 );
    }
    
    if ( previous.operation != play_operation_pass || (candidates - candidate_begin) == 0 ) {
        *(candidates++) = play_action_make( play_operation_pass, 0 );
    }
    
    return (int32_t)(candidates - candidate_begin);
}

bool is_member_play_actions( const play_action *actions, const play_action member, const size_t size )
{
    for ( ssize_t i = 0; i < size; i++ ) {
        if ( is_equals_play_action( actions[i], member ) ) return true;
    }
    return false;
}

void play_draw( int16_t *deck, int16_t *hands )
{
    const int16_t card = pop_sequence( deck );
    push_sequence( hands, card );
}

void play_put( int16_t *hands, int16_t *place, const int16_t card )
{
    const bool picked = pick_sequence( hands, card );
    assert( picked );
    push_sequence( place, card );
}

play_action play( play_action action, const play_action previous, int16_t *deck, int16_t *hands, const ssize_t max_hands, int16_t *place_left, int16_t *place_right )
{
    play_action candidates[play_action_candidate_max] = {};
    const int32_t number_of_candidates = play_action_candidates( candidates, previous, deck, hands, max_hands, place_left, place_right );
    assert( number_of_candidates > 0 );
    
    if ( ! is_member_play_actions( candidates, action, number_of_candidates ) ) {
        // select force an action in candidtes.
        fprintf( stderr, "warn: そのプレイヤーの行動はできません. 自動的に行動は選択されます.\n" );
        action = candidates[0];
    }
    
    if ( action.operation == play_operation_pass ) {
        // no operation.
    } else if ( action.operation == play_operation_draw ) {
        play_draw( deck, hands );
    } else if ( action.operation == play_operation_put_left ) {
        play_put( hands, place_left, action.card );
    } else if ( action.operation == play_operation_put_right ) {
        play_put( hands, place_right, action.card );
    } else {
        fprintf( stderr, "error: 存在しない行動です.\n" );
        assert( 0 );
    }
    
    return action;
}

bool game_is_end( const int16_t *deck_p1, const int16_t *deck_p2, const int16_t *hands_p1, const int16_t *hands_p2 )
{
    return (*deck_p1 == 0 && *hands_p1 == 0) || (*deck_p2 == 0 && *hands_p2 == 0 );
}

void print_hands( FILE *fp, const int16_t *hands )
{
    while ( *hands != 0 ) {
        fprintf( fp, "%d ", *(hands++) );
    }
}

void print_action( FILE *fp, const play_action action )
{
    if ( action.operation == play_operation_pass ) {
        fprintf( fp, "パス" );
    } else if ( action.operation == play_operation_draw ) {
        fprintf( fp, "山札から１枚引く" );
    } else if ( action.operation == play_operation_put_left ) {
        fprintf( fp, "左に %d を置く", action.card );
    } else if ( action.operation == play_operation_put_right ) {
        fprintf( fp, "右に %d を置く", action.card );
    } else {
        fprintf( fp, "未定義の行動 %d", action.card );
    }
}

int run_game( const int p1_in, const int p1_out, const int p2_in, const int p2_out )
{
    if ( option_verbose ) fprintf( stderr, "ゲームを初期化します...\n" );
    
    // score of total games.
    int32_t score_p1 = 0;
    int32_t score_p2 = 0;
    
    for ( int32_t index_of_game = 0; index_of_game < option_number_of_games; index_of_game++ ) {
        if ( option_verbose ) fprintf( stderr, "第 %000d ゲームを開始\n", index_of_game+1 );

        // deck.
        int16_t deck_p1[] = {1,2,3,4,5,6,7,8,9,10,11,12,13,1,2,3,4,5,6,7,8,9,10,11,12,13,0};
        int16_t deck_p2[] = {1,2,3,4,5,6,7,8,9,10,11,12,13,1,2,3,4,5,6,7,8,9,10,11,12,13,0};
        const size_t max_number_of_deck = sizeof( deck_p1 ) / sizeof( deck_p1[0] );
        const int32_t max_number_of_cars_in_deck_p1 = number_of_sequence( deck_p1 );
        const int32_t max_number_of_cars_in_deck_p2 = number_of_sequence( deck_p2 );
        assert( max_number_of_cars_in_deck_p1 == max_number_of_cars_in_deck_p2 );
        assert( memcmp( deck_p1, deck_p2, sizeof( deck_p1 ) ) == 0 );
        deck_shuffle( deck_p1, number_of_sequence( deck_p1 ) );
        deck_shuffle( deck_p2, number_of_sequence( deck_p2 ) );
        
        // hands.
        const size_t max_number_of_hands = 5;
        int16_t hands_p1[max_number_of_hands+1] = {};
        int16_t hands_p2[max_number_of_hands+1] = {};
        
        // place. 0 terminated.
        int16_t place_left[max_number_of_deck*2] = {};
        int16_t place_right[max_number_of_deck*2] = {};
        
        // last turn action.
        play_action last_p1 = {};
        play_action last_p2 = {};
        
        if ( ! write_reset( p1_in, index_of_game ) ) return EXIT_FAILURE;
        if ( ! read_to_lineend( p1_out ) ) return EXIT_FAILURE;

        if ( ! write_reset( p2_in, index_of_game ) ) return EXIT_FAILURE;
        if ( ! read_to_lineend( p2_out ) ) return EXIT_FAILURE;
        
        for ( int32_t index_of_turn = 0; ! game_is_end( deck_p1, deck_p2, hands_p1, hands_p2 ); index_of_turn++ ) {

            // print game.
            {
                fprintf( stdout, "ターン数: %d\n", index_of_turn+1 );
                fprintf( stdout, "場: 左%d 右%d\n", *place_left, *place_right );
                fprintf( stdout, "P1の手札: " );
                print_hands( stdout, hands_p1 );
                fprintf( stdout, "\n" );
                fprintf( stdout, "P2の手札: " );
                print_hands( stdout, hands_p2 );
                fprintf( stdout, "\n" );
            }
            
            if ( (index_of_turn + index_of_game) % 2 == 0 ) {
                if ( ! write_play( p1_in, index_of_turn, hands_p1, hands_p2, place_left, place_right, last_p1, last_p2 ) ) {
                    return EXIT_FAILURE;
                }
                
                const play_action action = read_play( p1_out );
                if ( action.operation == play_operation_error ) {
                    return EXIT_FAILURE;
                }
                
                last_p1 = play( action, last_p1, deck_p1, hands_p1, max_number_of_hands, place_left, place_right );
                
                // print result.
                {
                    fprintf( stdout, "P1の行動: " );
                    print_action( stdout, last_p1 );
                    fprintf( stdout, "\n\n" );
                }
            } else {
                if ( ! write_play( p2_in, index_of_turn, hands_p2, hands_p1, place_left, place_right, last_p2, last_p1 ) ) {
                    return EXIT_FAILURE;
                }
                
                const play_action action = read_play( p2_out );
                if ( action.operation == play_operation_error ) {
                    return EXIT_FAILURE;
                }
                
                last_p2 = play( action, last_p2, deck_p2, hands_p2, max_number_of_hands, place_left, place_right );
                // print result.
                {
                    fprintf( stdout, "P2の行動: " );
                    print_action( stdout, last_p2 );
                    fprintf( stdout, "\n\n" );
                }
            }
        }
        
        {
            const int32_t sum_p1 = sum_sequence( deck_p1 ) + sum_sequence( hands_p1 );
            const int32_t sum_p2 = sum_sequence( deck_p2 ) + sum_sequence( hands_p2 );
            int32_t points_p1 = 0;
            int32_t points_p2 = 0;
            if ( sum_p1 == 0 ) {
                points_p1 += sum_p2;
                points_p2 -= sum_p2;
            }
            if ( sum_p2 == 0 ) {
                points_p1 -= sum_p1;
                points_p2 += sum_p1;
            }
            
            score_p1 += points_p1;
            score_p2 += points_p2;
            
            // print score.
            {
                fprintf( stdout, "P1 POINTS: %d / %d\n", points_p1, score_p1 );
                fprintf( stdout, "P2 POINTS: %d / %d\n", points_p2, score_p2 );
            }
            
            if ( ! write_gameset( p1_in, points_p1, points_p2, score_p1, score_p2 ) ) {
                return EXIT_FAILURE;
            }
            if ( ! read_to_lineend( p1_out ) ) {
                return EXIT_FAILURE;
            }
            
            if ( ! write_gameset( p2_in, points_p2, points_p1, score_p2, score_p1 ) ) {
                return EXIT_FAILURE;
            }
            if ( ! read_to_lineend( p2_out ) ) {
                return EXIT_FAILURE;
            }
        }
    }
    
    return EXIT_SUCCESS;
}

int main( const int argc, const char *argv[] )
{
    // get options.
    for ( int i = 1; i < argc; i++ ) {
        if ( i+1 < argc && strcmp( argv[i], "--player1" ) == 0 ) {
            option_player1 = argv[++i];
        } else if ( i+1 < argc && strcmp( argv[i], "--player2" ) == 0 ) {
            option_player2 = argv[++i];
        } else if ( i+1 < argc && strcmp( argv[i], "--number" ) == 0 ) {
            option_number_of_games = atoi( argv[++i] );
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
        fprintf( stdout, " --number %d\n", option_number_of_games );
        fprintf( stdout, " --verbose %s\n", option_verbose ? "true" : "false" );
    }
    
    // srand
    srand( time( NULL ) );
    
    // setup pipe.
    if ( option_verbose ) fprintf( stderr, "パイプを準備します...\n" );
    
    int fd_p1_in[2];
    int fd_p1_out[2];
    int fd_p2_in[2];
    int fd_p2_out[2];
    
    if ( pipe( fd_p1_in ) == -1 ) {
        fprintf( stderr, "error: pipe に失敗しました(%d).\n", __LINE__ );
        return EXIT_FAILURE;
    }
    
    if ( pipe( fd_p1_out ) == -1 ) {
        fprintf( stderr, "error: pipe に失敗しました(%d).\n", __LINE__ );
        return EXIT_FAILURE;
    }
    
    if ( pipe( fd_p2_in ) == -1 ) {
        fprintf( stderr, "error: pipe に失敗しました(%d).\n", __LINE__ );
        return EXIT_FAILURE;
    }
    
    if ( pipe( fd_p2_out ) == -1 ) {
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
        if ( close( fd_p2_in[0] ) == -1 ) {
            fprintf( stderr, "warn: close に失敗しました(%d).\n", __LINE__ );
        }
        if ( close( fd_p2_in[1] ) == -1 ) {
            fprintf( stderr, "warn: close に失敗しました(%d).\n", __LINE__ );
        }
        if ( close( fd_p2_out[0] ) == -1 ) {
            fprintf( stderr, "warn: close に失敗しました(%d).\n", __LINE__ );
        }
        if ( close( fd_p2_out[1] ) == -1 ) {
            fprintf( stderr, "warn: close に失敗しました(%d).\n", __LINE__ );
        }
        
        return run_player( option_player1, fd_p1_in[0], fd_p1_out[1] );
    }
    
    pid_p2 = fork();
    if ( pid_p2 == -1 ) {
        fprintf( stderr, "error: fork に失敗しました(%d).\n", __LINE__ );
        return EXIT_FAILURE;
    }
    
    if ( pid_p2 == 0 ) {
        // player1 child process.
        if ( option_verbose ) fprintf( stderr, "[%s]を実行します...\n", option_player1 );
        
        // close other pipes.
        if ( close( fd_p1_in[0] ) == -1 ) {
            fprintf( stderr, "warn: close に失敗しました(%d).\n", __LINE__ );
        }
        if ( close( fd_p1_in[1] ) == -1 ) {
            fprintf( stderr, "warn: close に失敗しました(%d).\n", __LINE__ );
        }
        if ( close( fd_p1_out[0] ) == -1 ) {
            fprintf( stderr, "warn: close に失敗しました(%d).\n", __LINE__ );
        }
        if ( close( fd_p1_out[1] ) == -1 ) {
            fprintf( stderr, "warn: close に失敗しました(%d).\n", __LINE__ );
        }
        
        return run_player( option_player1, fd_p2_in[0], fd_p2_out[1] );
    }
    
    // start game.
    const int exit_code = run_game( fd_p1_in[1], fd_p1_out[0], fd_p2_in[1], fd_p2_out[0] );
    
    // cleanup.
    if ( option_verbose ) fprintf( stderr, "ゲームを終了します...\n" );
    
    if ( close( fd_p1_in[0] ) == -1 ) {
        fprintf( stderr, "warn: close に失敗しました(%d).\n", __LINE__ );
    }
    if ( close( fd_p1_in[1] ) == -1 ) {
        fprintf( stderr, "warn: close に失敗しました(%d).\n", __LINE__ );
    }
    if ( close( fd_p1_out[0] ) == -1 ) {
        fprintf( stderr, "warn: close に失敗しました(%d).\n", __LINE__ );
    }
    if ( close( fd_p1_out[1] ) == -1 ) {
        fprintf( stderr, "warn: close に失敗しました(%d).\n", __LINE__ );
    }
    if ( close( fd_p2_in[0] ) == -1 ) {
        fprintf( stderr, "warn: close に失敗しました(%d).\n", __LINE__ );
    }
    if ( close( fd_p2_in[1] ) == -1 ) {
        fprintf( stderr, "warn: close に失敗しました(%d).\n", __LINE__ );
    }
    if ( close( fd_p2_out[0] ) == -1 ) {
        fprintf( stderr, "warn: close に失敗しました(%d).\n", __LINE__ );
    }
    if ( close( fd_p2_out[1] ) == -1 ) {
        fprintf( stderr, "warn: close に失敗しました(%d).\n", __LINE__ );
    }
    
    return exit_code;
}
