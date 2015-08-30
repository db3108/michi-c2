// sgf.c -- Read and write SGF files for the michi-c gtp engine
#include "michi.h"

char buf[128];    
FILE *f;           // source of the sgf data
Game *game;        // game in which the sgf file will be written
int  nmoves;       // number of moves loaded from the sgf file

// ---------------------- Update of the Game struct ---------------------------
Game *new_game(Position *pos)
{
    Game *game=michi_calloc(1, sizeof(Game));
    game->pos = pos;
    game_clear_board(game);
    return game;
}

void free_game(Game *game)
{
    free(game->pos);
    free(game);
}

char* game_clear_board(Game *game) 
{
    game->handicap = 0;
    slist_clear(game->moves);
    slist_clear(game->placed_black_stones);
    slist_clear(game->placed_white_stones);
    return empty_position(game->pos);
}

int is_game_board_empty(Game *game) 
{
    if (slist_size(game->moves) != 0 
            || slist_size(game->placed_black_stones) != 0
            || slist_size(game->placed_white_stones) != 0)
        return 0;
    else
        return 1;
}

char* do_play(Game *game, Color c, Point pt)
// Play the move (updating the Game struct)
{
    char *ret;
    Color to_play_before;
    int   played=0;
    Info  m;
    Position *pos = game->pos;

    to_play_before = board_color_to_play(pos);
    board_set_color_to_play(pos, c);

    if (point_color(pos,pt) == EMPTY) {
        ret = play_move(pos, pt);
        if (ret[0] == 0) {
            m = pt + (board_captured_neighbors(pos) << 9) 
                   + (board_ko_old(pos) << 13) 
                   + ((to_play_before) << 22);
            played = 1;
        }
        // else illegal move: nothing played
    }
    else if(pt == PASS_MOVE) {
        ret = pass_move(pos);
        m = pt + (board_captured_neighbors(pos) << 9) 
               + (board_ko_old(pos) << 13) 
               + ((to_play_before) << 22);
        played = 1;
    }
    else ret ="Error Illegal move: point not EMPTY\n";
    if (played) {
        c2++;
        slist_push(game->moves, m);
    }
    return ret;
}

// ------------------------ SGF Recursive Parser ------------------------------
// This file implement a Recursive Descent Parser for the SGF grammar 
// References :
// [1] Aho, Lam, Sethi, Ullman. Compilers. 2nd Edition (The Dragon book)
// [2] http://www.red-bean.com/sgf/sgf4.html#2 (SGF)

// The SGF grammar has been adapted from [2] to make it Right Recursive
//
// The recognized grammar is the following :
// Collection       = GameTree RestOfCollection
// RestOfCollection = GameTree RestOfCollection | None
// GameTree         = "(" Sequence RestOfCollection ")"
// Sequence         = Node RestOfSequence
// RestOfSequence   = Node RestOfSequence | None
// Node             = ";" Property { Property }
// Property         = PropIdent { PropValue }
// PropIdent        = see prop_name[] array below 
// PropValue        = "[" ValueType "]"
// ValueType        = Point | None

// The code is simple. There is a function for each non terminal symbol 
// of the grammar with the same name of the non terminal.
// The terminal symbols are handled by the lexical analyser yylex()

// Note: the code may look very inefficient at first sight with a lot of 
//       recursive functions. But it is not. Any current optimizing compiler 
//       (gcc, icc or Microsoft Visual Studio) is able to eliminate easily this
//       "tail recursion".
//       In addition, parsing sgf file is certainly not the most consuming task
//       that michi must accomplish


int  prop;         // current property
int  lineno=1;     // current line of input
int  symbol;       // lookahead symbol
char yytext[8192]; // the text of the current token
int  yyleng;       // length of the string representation of the current token
int  yyval;        // value of the current token

#define UPPERCASE_STRING 256
#define LOWERCASE_STRING 257
#define STRING           258
#define POINT            260
#define NUMBER           261
#define REAL             262

char* prop_name[] = { "",   "AB", "AW",  "B",  "W",  "C",
                      "FF", "AP", "CA", "SZ", "KM", "DT",
                      "PL", "HA", "MULTIGOGM", 
                       "N", "AE", "GM", "GN", "US", "GW",
                      "GB", "DM", "UC", "TE", "BM", "DO",
                      "IT",
                      NULL };
#define NOT_FOUND        0

int find(char *string, char *strlist[]) 
// Return 0 if string is not found in strlist, i != 0 if it is found at pos i
{
    for (int i=1 ; strlist[i] != NULL ; i++) {
        if (strcmp(string, strlist[i]) == 0)
            return i;
    }
    return 0;       // Not Found
}

int yylex(void)
{
    int c, all_upper=1, all_lower=1, number=0, real=0;
    while(isspace(c=fgetc(f)))
        if (c == '\n') lineno++;

    if (!isalnum(c) && c != '+' && c != '-')
        return c;
    else {
        yyleng = 0;
        yytext[yyleng++] = c;
        if (isdigit(c) || c == '+' || c == '-') {
            all_lower = 0;
            all_upper = 0;
            number = 1;
            while ((c=fgetc(f)) != ']' && c != '[') {
                if (!isdigit(c)) {
                    if (c == '.') {
                        real = 1;
                    }
                    else {
                        number = 0;
                        real = 0;
                    }
                }
                yytext[yyleng++] = c;
            }
        }
        else {
            while ((c=fgetc(f)) != ']' && c != '[') {
                all_lower = all_lower && islower(c);
                all_upper = all_upper && isupper(c);
                yytext[yyleng++] = c;
            }
        }
        ungetc(c, f);
        yytext[yyleng] = 0;

        if (real)   return REAL;
        if (number) return NUMBER;

        if (all_upper) {
            yyval = find(yytext, prop_name);
            return UPPERCASE_STRING;
        }
        else if (all_lower) {
            if (yyleng == 2)
                return POINT;
            else
                return LOWERCASE_STRING;
        }
        return STRING;
    }
}

char* token_name(int tok)
{
    if (tok < 256) {
        buf[0] = tok; buf[1] = 0;
    }
    else if (tok == POINT)
        sprintf(buf,"POINT");
    else if (tok == STRING)
        sprintf(buf,"STRING");
    else if (tok == UPPERCASE_STRING)
        sprintf(buf,"UPPERCASE_STRING");
    else if (tok == LOWERCASE_STRING)
        sprintf(buf,"LOWERCASE_STRING");
    else
        sprintf(buf,"token not known");
    return buf;
}

void accept(int tok)
{
    if (symbol == tok) symbol = yylex();
    else {
        fprintf(stderr, "line %d: Syntax Error\n", lineno);
        fprintf(stderr, "expected symbol: \"%s\"", token_name(tok));
        fprintf(stderr, ", read \"%s\"\n",token_name(symbol));
    }
    //printf("%s", token_name(symbol));
    //if (symbol == UPPERCASE_STRING) printf(": %s", yytext);
    //printf("\n");
}

void ValueType(void)
// ValueType = Point | Number | Real | Text | None
{
    if (symbol == POINT || symbol == STRING || symbol == UPPERCASE_STRING
        || symbol == NUMBER || symbol == REAL) {
        accept(symbol);
        //fprintf(stderr, "%s ", yytext);
    }
}

void PropValue(void)
// PropValue = "[" ValueType "]"
{
    accept('['); ValueType();

    // Property Value was read : use it to modify the game
    int size = board_size(game->pos);
    if (board_nmoves(game->pos) < nmoves-1 
        && strcmp(prop_name[prop], "B") == 0) {
        if (yytext[0] != 'B')
            do_play(game, BLACK, parse_sgf_coord(yytext, size));
        else
            do_play(game, BLACK, PASS_MOVE);
    }
    else if (board_nmoves(game->pos) < nmoves-1 
        && strcmp(prop_name[prop], "W") == 0) {
        if (yytext[0] != 'W')
            do_play(game, WHITE, parse_sgf_coord(yytext, size));
        else
            do_play(game, WHITE, PASS_MOVE);
    }
    else if (strcmp(prop_name[prop], "AB") == 0) {
        Point pt = parse_sgf_coord(yytext, size);
        slist_push(game->placed_black_stones, pt);
        board_place_stone(game->pos, pt, BLACK);
    }
    else if (strcmp(prop_name[prop], "AW") == 0) {
        Point pt = parse_sgf_coord(yytext, size);
        slist_push(game->placed_white_stones, pt);
        board_place_stone(game->pos, pt, WHITE);
    }
    else if (strcmp(prop_name[prop], "KM") == 0)
        board_set_komi(game->pos, atof(yytext));
    else if (strcmp(prop_name[prop], "SZ") == 0) {
        if (is_game_board_empty(game)) { 
            board_set_size(game->pos, atoi(yytext));
            game_clear_board(game);
        }
        else {
            // Can happen if SZ occurs after AB or AW
            Point    bstones[BOARDSIZE], wstones[BOARDSIZE];
            Position *pos=game->pos;

            slist_clear(bstones); slist_clear(wstones);
            slist_append(bstones, game->placed_black_stones);
            slist_append(wstones, game->placed_white_stones);

            board_set_size(game->pos, atoi(yytext));
            game_clear_board(game);
            
            FORALL_IN_SLIST(bstones, pt) {
                board_set_color_to_play(pos, BLACK);
                play_move(pos, pt);
                slist_push(game->placed_black_stones, pt);
            }
            FORALL_IN_SLIST(wstones, pt) {
                board_set_color_to_play(pos, WHITE);
                play_move(pos, pt);
                slist_push(game->placed_white_stones, pt);
            }
        }
    }
    else if (strcmp(prop_name[prop], "PL") == 0) {
        if (strcmp(yytext, "B") == 0)
            board_set_color_to_play(game->pos, BLACK);
        else
            board_set_color_to_play(game->pos, WHITE);
    }

    accept(']');
}

void PropValues(void)
// Propvalues = { PropValue }
{
    if (symbol == '[') {
        PropValue(); PropValues();
    }
}

void PropIdent(void)
{
    if (symbol == UPPERCASE_STRING) {
        prop = yyval;
        //if (yyval > 0 ) 
        //    fprintf(stderr, "prop %s: ",  yytext);
        //else
        //    fprintf(stderr, "unknown %s: ",  yytext);
        accept(UPPERCASE_STRING);
    }
}

void Property(void)
// Property = PropIdent { PropValue }
{
    PropIdent(); PropValue(); PropValues();
    //fprintf(stderr, "\n");
}

void Properties(void)
// Properties = { Property }
{
    if (symbol == UPPERCASE_STRING) {
        Property(); Properties();
    }
}

void Node(void)
// Node = ";" Properties
{
    accept(';'); Properties();
}

void RestOfSequence(void)
// RestOfSequence = Node RestOfSequence | None
{
    if (symbol == ';') { 
        Node(); RestOfSequence();
    }
}

void Sequence(void)
// Sequence = Node RestOfSequence
{
    Node(); RestOfSequence();
}

void RestOfCollection(void);
void GameTree(void)
// GameTree = "(" Sequence RestOfCollection ")"
{
    accept('('); Sequence(); RestOfCollection(); accept(')');
}

void RestOfCollection(void)
// RestOfCollection = GameTree RestOfCollection | None
{
    if (symbol == '(') {
        GameTree(); RestOfCollection();
    }
}

void Collection(void)
// Collection = GameTree RestOfCollection
{
    GameTree(); RestOfCollection();
}

char *loadsgf(Game *sgf_game, char *filename, int sgf_nmoves)
// Load a position from the SGF file filename using a recursive parser for the
// SGF grammar which is described at the top of the current file.
// See ref [1].
//
// The modifications of the game struct is done in ProPValue()
{
    f = fopen(filename, "r");  // File shared between all the routines
    game = sgf_game;           // Game in which all the actions take place
    nmoves = sgf_nmoves;       // Number of moves loaded from sgf file

    game_clear_board(game);
    if (f != NULL) {
        symbol = yylex();
        Collection();
        assert(symbol == '\n' || symbol == EOF);
        if (board_color_to_play(game->pos) == BLACK)
            strcpy(buf,"B");
        else
            strcpy(buf,"W");
        fclose(f);
    }
    else {
        sprintf(buf, "Error - can't open file %s", filename);
    }
    return buf;
}
