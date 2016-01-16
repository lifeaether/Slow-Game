
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <assert.h>

#include <unistd.h>

typedef int16_t card_t;         // 札
typedef card_t * card_array_t;  // 札の配列.

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

//!
//! @brief  札の枚数を返します.
//!
//! @param  cards   [in]札の配列.
//!
//! @return cardsの枚数.
//!
int32_t card_array_count( const card_array_t cards )
{
    int32_t count = 0;
    while ( cards[count] != 0 ) count++;
    return count;
}

//!
//! @brief  配列の先頭からindex番目のカードを返します
//!
//! @param  cards   [in]札の配列
//! @param  index   [in]index番目
//!
//! @return cardsの配列のindex番目の札の番号
//!
int32_t card_array_at( const card_array_t cards, const int32_t index )
{
    return cards[index];
}

//!
//! @brief  配列の先頭のカードを返します
//!
//! @param  cards   [in]札の配列 ( 1枚以上 )
//!
//! @return cardsの配列の先頭の札の番号
//!
//! @note   札の配列が0枚の時に呼び出してはいけません.
//!
int32_t card_array_top( const card_array_t cards )
{
    return cards[0];
}

//!
//! @brief  札の配列が0枚であるか
//!
//! @param  cards   [in]札の配列
//!
//! @retval true    札が0枚だった
//! @retval false   札が1枚以上
//!
bool card_array_is_empty( const card_array_t cards )
{
    return cards[0] == 0;
}

//!
//! @brief  札の配列にn番が含まれるか
//!
//! @param  cards   [in]札の配列
//! @param  n       [in]調べる番号
//!
//! @retval true    cardsにn番が含まれる
//! @retval false   cardsにn番が含まれない
//!
bool card_array_is_member( const card_array_t cards, const int32_t n )
{
    const int32_t count = card_array_count( cards );
    for ( int32_t index = 0; index < count; index++ ) {
        const card_t card = card_array_at( cards, index );
        if ( card == n ) {
            return true;
        }
    }
    return false;
}

//!
//! @brief  ターンの行動を表す action_t を作成します
//!
//! @param  operation   [in]行動. enum action_operation のいづれか
//! @param  card        [in]cardを選択する行動の場合, ここに札番号を指定する
//!
//! @return 行動
//!
action_t action_make( const action_operation operation, const card_t card )
{
    action_t action = { operation, card };
    return action;
}


int32_t action_put_candidates( action_t *candidates, const action_operation operation, const card_array_t hands, const card_array_t place )
{
    int32_t candidate_count = 0;
    if ( card_array_is_empty( place ) ) {
        for ( int32_t index = 0; index < card_array_count( hands ); index++ ) {
            const card_t card = card_array_at( hands, index );
            candidates[candidate_count] = action_make( operation, card );
            candidate_count++;
        }
    } else {
        const card_t card = card_array_top( place );
        int16_t upper = card == 13 ? 1 : card + 1;
        int16_t lower = card == 1 ? 13 : card - 1;
        if ( card_array_is_member( hands, lower ) ) {
            candidates[candidate_count] = action_make( operation, lower );
            candidate_count++;
        }
        if ( card_array_is_member( hands, upper ) ) {
            candidates[candidate_count] = action_make( operation, upper );
            candidate_count++;
        }
    }
    return candidate_count;
}

//!
//! @brief  ターン中の行動候補をすべて取得します
//!
//! @param  candidate   [out]取得した行動. ( k_action_candidate_max よりも大きい配列を渡してください )
//! @param  hands       [in]そのターンのプレイヤーの手札
//! @param  place_left  [in]場の左の山
//! @param  place_right [in]場の右の山
//! @param  previous    [in]そのターンのプレイヤーの前回の行動
//!
//! @return 候補の数 (引数candidatesに代入された数)
//!
static const size_t k_action_candidate_max = 12;  //!< candidatesに代入されうる最大の数
int32_t action_candidates( action_t *candidates, const card_array_t hands, const card_array_t place_left, const card_array_t place_right, const action_t previous, const int32_t count_of_draw )
{
    int32_t candidate_count = 0;
    if ( previous.operation == action_operation_pass ) {
        for ( int32_t index = 0; index < card_array_count( hands ); index++ ) {
            const card_t card = card_array_at( hands, index );
            candidates[candidate_count] = action_make( action_operation_put_left, card );
            candidate_count++;
            candidates[candidate_count] = action_make( action_operation_put_right, card );
            candidate_count++;
        }
    } else {
        candidate_count += action_put_candidates( &candidates[candidate_count], action_operation_put_left, hands, place_left );
        candidate_count += action_put_candidates( &candidates[candidate_count], action_operation_put_right, hands, place_right );
    }
    
    if ( card_array_count( hands ) < 5 && count_of_draw < 26 ) {
        candidates[candidate_count] = action_make( action_operation_draw, 0 );
        candidate_count++;
    }

    if ( candidate_count == 0 || previous.operation != action_operation_pass ) {
        candidates[candidate_count] = action_make( action_operation_pass, 0 );
        candidate_count++;
    }
    
    return candidate_count;
}

// 1ゲーム中に引いた札の数を数える.
static int32_t count_of_draw = 0;


//!
//! @brief  1ゲーム開始時に呼び出されます
//!
//! @param number_of_game  [in]n回目のゲーム開始
//!
void reset( const int32_t number_of_game )
{
    // ここに 1 ゲームが開始される直前に行う処理を記述します.
    count_of_draw = 0;
    
    // 乱数を初期化する.
    srand( (int)time( NULL ) );
}

//!
//! @brief  1ゲーム終了時に呼び出されます
//!
//! @param you_point    [in]今回のゲームで得た自分のポイント
//! @param you_score    [in]これまでのゲームで得た自分のポイントの合計
//! @param op_point     [in]今回のゲームで得た相手のポイント
//! @param op_score     [in]これまでのゲームで得た相手のポイントの合計
//!
void gameset( const int32_t you_point, const int32_t you_score, const int32_t op_point, const int32_t op_score )
{
    // ここに 1 ゲームが終了した時に行う処理を記述します.
}

//!
//! @brief  自分のターンに呼び出されます.
//!
//! @param  you_hands    [in]自分の手札
//! @param  op_hands     [in]相手の手札
//! @param  place_left   [in]場の左の山
//! @param  place_right  [in]場の右の山
//! @param  you_previous [in]前回の自分の行動
//! @param  op_previous  [in]前回の相手の行動
//!
static void action_write( const action_t action, char *line );
action_t play( const int32_t turn, const card_array_t you_hands, const card_array_t op_hands, const card_array_t place_left, const card_array_t place_right, const action_t you_previous, const action_t op_previous )
{
    // ここに自分のターンの処理を記述します.
   
    // 引いたカードの枚数を数える
    if ( you_previous.operation == action_operation_draw ) {
        count_of_draw++;
    }
    
    // このターンの行動の候補を取得する.
    action_t candidates[k_action_candidate_max];
    const int32_t candidate_count = action_candidates( candidates, you_hands, place_left, place_right, you_previous, count_of_draw );
    
    // 候補の中からランダムで実行. 但し, パス以外の行動ができるときはパスを除く.
    if ( candidate_count > 1 && candidates[candidate_count-1].operation == action_operation_pass ) {
        return candidates[ rand() % (candidate_count-1) ];
    } else {
        return candidates[0];
    }
}


int32_t card_array_read( card_array_t cards, const int32_t max_cards, const char *line )
{
    card_array_t it = cards;
    while ( it - cards < max_cards ) {
        while ( *line != 0 && *line == ' ' ) line++;
        if ( *line == 0 ) break;
        if ( sscanf( line, "%hi", it ) != 1 ) break;
        while ( *line != 0 && *line != ' ' ) line++;
        it++;
    }
    *it = 0;
    assert( card_array_count( cards ) == (int32_t)( it - cards ) );
    return (int32_t)( it - cards );
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
