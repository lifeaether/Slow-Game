
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <assert.h>

#include <unistd.h>

typedef int16_t card_t;         // 札
typedef card_t * card_array_t;  // 札の列

typedef enum {
    action_operation_none,       // 行動なし
    action_operation_pass,       // パス
    action_operation_draw,       // 山札から1枚引く
    action_operation_put_left,   // 場の左に札を1枚出す
    action_operation_put_right   // 場の右に札を1枚出す
} action_operation;

typedef struct {
    action_operation    operation;  // 行動
    card_t              card;       // 場に出した場合の札の番号
} action_t;  // ターンでの行動

int32_t card_array_count( const card_array_t cards )
{
    int32_t count = 0;
    while ( cards[count] != 0 ) count++;
    return count;
}

int32_t card_array_at( const card_array_t cards, const int32_t index )
{
    return cards[index];
}

int32_t card_array_read( card_array_t cards, const int32_t max_cards, const char *line )
{
    card_array_t it = cards;
    while ( it - cards < max_cards ) {
        while ( *line != 0 && *line != ' ' ) line++;
        if ( *line == 0 ) break;
        if ( sscanf( line, "%hi", it ) != 1 ) break;
        it++;
    }
    return (int32_t)( it - cards );
}

action_t action_make( const action_operation operation, const card_t card )
{
    action_t action = { operation, card };
    return action;
}

action_t action_read( const char *line )
{
    action_t action = {};
    if ( strcmp( line, "P\n" ) == 0 ) {
        action.operation = action_operation_pass;
    } else if ( strcmp( line, "D\n" ) == 0  ) {
        action.operation = action_operation_draw;
    } else if ( strcmp( line, "\n" ) == 0 ) {
        action.operation = action_operation_none;
    } else if ( sscanf( line, "L%hi\n", &action.card ) == 1 ) {
        action.operation = action_operation_put_left;
    } else if ( sscanf( line, "R%hi\n", &action.card ) == 1 ) {
        action.operation = action_operation_put_right;
    } else {
        assert( 0 );
    }
    return action;
}

void action_write( const action_t action, char *line )
{
    if ( action.operation == action_operation_pass ) {
        strcpy( line, "P\n" );
    } else if ( action.operation == action_operation_draw ) {
        strcpy( line, "D\n" );
    } else if ( action.operation == action_operation_put_left ) {
        sprintf( line, "L%d\n", action.card );
    } else if ( action.operation == action_operation_put_right ) {
        sprintf( line, "R%d\n", action.card );
    } else {
        assert( 0 );
    }
}

static void reset( const int32_t number_of_game )
{
    // ここに 1 ゲームが開始される直前に行う処理を記述します.
}

static void gameset( const int32_t you_point, const int32_t you_score, const int32_t op_point, const int32_t op_score )
{
    // ここに 1 ゲームが終了した時に行う処理を記述します.
}

static action_t play( const int32_t turn, const card_array_t you_hands, const card_array_t op_hands, const card_array_t place_left, const card_array_t place_right, const action_t yout_previous, const action_t op_previous )
{
    // ここに自分のターンの処理を記述します.
    return action_make( action_operation_pass, 0 );
}

int main( const int argc, const char *argv[] )
{
    char line[256];
    while ( fgets( line, sizeof(line), stdin ) ) {
        if ( strcmp( line, "RESET\n" ) == 0 ) {
            int32_t number_of_game;
            fgets( line, sizeof(line), stdin );
            sscanf( line, "%d\n", &number_of_game );
            reset( number_of_game );
            fprintf( stdout, "\n" );
            fflush( stdout );
        } else if ( strcmp( line, "GAMESET\n" ) == 0 ) {
            int32_t you_point, you_score, op_point, op_score;
            fgets( line, sizeof(line), stdin );
            sscanf( line, "%d %d\n", &you_point, &you_score );
            fgets( line, sizeof(line), stdin );
            sscanf( line, "%d %d\n", &op_point, &op_score );
            gameset( you_point, you_score, op_point, op_score);
            fprintf( stdout, "\n" );
            fflush( stdout );
        } else if ( strcmp( line, "PLAY\n" ) == 0 ) {
            int32_t turn;
            card_t you_hands[6] ={}, op_hands[6] = {}, place_left[53] = {}, place_right[53] = {};
            action_t you_previous, op_previous;
            fgets( line, sizeof(line), stdin );
            sscanf( line, "%d\n", &turn );
            fgets( line, sizeof(line), stdin );
            card_array_read( you_hands, sizeof(you_hands)/sizeof(you_hands[0]), line );
            fgets( line, sizeof(line), stdin );
            card_array_read( op_hands, sizeof(op_hands)/sizeof(op_hands[0]), line );
            fgets( line, sizeof(line), stdin );
            card_array_read( place_left, sizeof(place_left)/sizeof(place_left[0]), line );
            fgets( line, sizeof(line), stdin );
            card_array_read( place_right, sizeof(place_right)/sizeof(place_right[0]), line );
            fgets( line, sizeof(line), stdin );
            you_previous = action_read( line );
            fgets( line, sizeof(line), stdin );
            op_previous = action_read( line );
            const action_t action = play( turn, you_hands, op_hands, place_left, place_right, you_previous, op_previous );
            action_write( action, line );
            fprintf( stdout, "%s", line );
            fflush( stdout );
        } else if ( strcmp( line, "QUIT\n" ) == 0 ) {
            break;
        } else {
            assert( 0 );
            break;
        }
    }
    fprintf( stderr, "END\n" );
    return 0;
}
