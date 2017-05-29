// sgf.c -- Update the Game structure and import/export it as SGF file
#include "michi.h"

FILE *f;           // source of the sgf data
Game *game;        // game in which the sgf file will be written
int  nmoves;       // number of moves loaded from the sgf file
Byte size_already_set;
// Displacements towards the neighbors of a point
//                      North East South  West  NE  SE  SW  NW
static int   delta[] = { -N-1,   1,  N+1,   -1, -N,  W,  N, -W};
static char  buf[4046];    

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

void game_set_komi(Game *game, float komi)
{
    game->komi = komi;
    board_set_komi(game->pos, komi);
}

char* game_clear_board(Game *game) 
{
    game->handicap = 0;
    slist_clear(game->moves);
    slist_clear(game->placed_black_stones);
    slist_clear(game->placed_white_stones);
    game->zhash = 0;
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

void board_set_size_safe(Position *pos, char *str)
// Set the size of the board with check that it is compatible with compilation
{
    int size = atoi(str);
    if (size > N) {
        sprintf(buf, "Error - Trying to set incompatible boardsize %s"
                     " (max=%d)", str, N);
        log_fmt_s('E', buf, NULL);
    }
    else {
        board_set_size(pos, size);
        empty_position(pos);
    }
}

void update_zhash(Game *game, Color c, Point pt)
// Update Zobrist Hash of the position (handle captures)
{
    Point points[BOARDSIZE];

    game->zhash ^= zobrist_hashdata[pt][c];
    if (game->pos->undo_capture) {
        Color other = color_other(c);
        for (int k=0; k<4 ; k++) {
            if (game->pos->undo_capture & (1<<k)) {
                Point n = pt + delta[k];
                compute_big_eye(game->pos, n, points);
                FORALL_IN_SLIST(points, s) {
                    game->zhash ^= zobrist_hashdata[s][other];
                }
            }
        }
    }
}

char* look_for_repetition(Game *game)
{    
    char *ret="";
    int n=game->pos->n;
    for (int k=30 ; k<n ; k++)
        if (game->zhash == game->zhistory[k]) 
            ret = "Error - Positional Superko rule violation";
    game->zhistory[n] = game->zhash;
    return ret;
}

char* do_undo(Game *game)
{
    Position *pos = game->pos;
    Point move = slist_pop(game->moves);

    board_set_captured_neighbors(pos, (move >> 9) & 15);
    board_set_ko_old(pos, (move >> 13) & 511);
    update_zhash(game, color_other(pos->to_play), pos->last);
    char *ret=undo_move(pos);
    board_set_color_to_play(pos, move >> 22);
    if (ret[0]==0) c2--;
    if (board_nmoves(pos) > 3)
        board_set_last3(pos, game->moves[board_nmoves(pos)-2] & 511);
    return ret;
}

char* do_play(Game *game, Color c, Point pt)
// Play the move (updating the Game struct)
{
    char *ret="", str[8];
    Color to_play_before;
    Info  m;
    int   played=0;
    Position *pos = game->pos;

    if (pt == RESIGN_MOVE)
        goto finished;

    to_play_before = board_color_to_play(pos);
    board_set_color_to_play(pos, c);
    if(pt == PASS_MOVE) {
        ret = pass_move(pos);
        m = pt + (board_captured_neighbors(pos) << 9) 
               + (board_ko_old(pos) << 13) 
               + ((to_play_before) << 22);
        played = 1;
    }
    else if (point_color(pos,pt) == EMPTY) {
        ret = play_move(pos, pt);
        if (ret[0] == 0) {
            m = pt + (board_captured_neighbors(pos) << 9) 
                   + (board_ko_old(pos) << 13) 
                   + ((to_play_before) << 22);
            played = 1;
        }
        // else illegal move: nothing played
    }
    else {
        sprintf(buf,"Error Illegal move: %s not EMPTY\n", str_coord(pt,str));
        ret = buf;
    }
    if (played) {
        c2++;
        slist_push(game->moves, m);
        if (pt != PASS_MOVE) {
            ZobristHash zhash_save = game->zhash;
            update_zhash(game, c, pt);
            ret = look_for_repetition(game);
            if (ret[0] != 0) {
                log_fmt_p('I',"try illegal move (superko violation) at %s "
                                                              "(ignored)", pt);
                do_undo(game);
                if (game->zhash != zhash_save)
                    log_fmt_s('W', "Error in undoing zhash", NULL);
            }
        }
    }
finished:
    return ret;
}
// ----------------------- Write Game to SGF file -----------------------------
char* storesgf(Game *game, const char *filename, const char* version)
// Write the sequence of moves stored in the game structure in SGF file
{
    char     date[24], str[8];
    FILE     *f = fopen(filename, "w");
    Position *pos = game->pos;
    int      size = board_size(pos);

    if (f != NULL) {
        // Get the current date
        time_t     tp=time(NULL);
        struct tm *tm=gmtime(&tp);
        strftime(date,24,"%Y-%m-%d",tm);

        // write the root node
        fprintf(f, "(;FF[4]CA[UTF-8]AP[michi-c:%s]SZ[%d]KM[%.1f]DT[%s]\n",
                version, size, board_komi(pos), date);
        if (slist_size(game->placed_black_stones) > 0) {
            fprintf(f, "AB");
            int n = 0;
            FORALL_IN_SLIST(game->placed_black_stones, pt) {
                fprintf(f, "[%s]", str_sgf_coord(pt, str, size));
                if (++n % 10 == 0) fprintf(f, "\n  ");
            }
            fprintf(f, "\n");
        }
        if (slist_size(game->placed_white_stones) > 0) {
            fprintf(f, "AW");
            int n = 0;
            FORALL_IN_SLIST(game->placed_white_stones, pt) {
                fprintf(f, "[%s]", str_sgf_coord(pt, str, size));
                if (++n % 10 == 0) fprintf(f, "\n  ");
            }
            fprintf(f, "\n");
        }

        // write the moves
        FORALL_IN_SLIST(game->moves, m) {
            Color c = m >> 22;
            m &= 511;
            char color = (c == BLACK) ? 'B' : 'W';
            fprintf(f, ";%c[%s]", color, str_sgf_coord(m, str, size));
        }
        fprintf(f, ")\n");
        fclose(f);
        buf[0] = 0;
    }
    else
        sprintf(buf, "Error - can't open file %s", filename);
    
    return buf;
}

// ------------------------ SGF Recursive Parser ------------------------------
// This code implement a Recursive Descent Parser for the SGF grammar 
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
// ValueType        = Point | Number | Real | Text | None

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
char sgferr[80];   // Error message 
// bad hack for reading the sgf files produced by gogui-twogtp
int  read_raw_text=0;

#define UPPERCASE_STRING 256
#define LOWERCASE_STRING 257
#define STRING           258
#define POINT            260
#define NUMBER           261
#define REAL             262

char* prop_name[] = { "",   "AB", "AW",  "B",  "W",  "C",
                      "FF", "AP", "CA", "SZ", "KM", "DT",
                      "PL", "HA", "MULTIGOGM", "RE", "ST",
                       "N", "AE", "GM", "GN", "US", "GW",
                      "GB", "DM", "UC", "TE", "BM", "DO",
                      "IT", "PB", "PW", "BR", "WR", "PC", 
                      "RU", "TM", "OT", 
                      "BL", "WL",      // specific to cgos ?
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

    if (read_raw_text) {
        yyleng = 0;
        all_upper=0;
        all_lower=0;
        while ((c=fgetc(f)) != ']') {
            if (c == '\\')
                c = fgetc(f);        // read \] in comments for example
            all_lower = all_lower && islower(c);
            all_upper = all_upper && isupper(c);
            yytext[yyleng++] = c;
        }
    }
    else {
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
    else if (tok == NUMBER)
        sprintf(buf,"NUMBER");
    else if (tok == REAL)
        sprintf(buf,"REAL");
    else
        sprintf(buf,"token not known");
    return buf;
}

void accept(int tok)
{
    if (symbol == tok) symbol = yylex();
    else {
        fprintf(stderr, "line %d: Syntax Error while reading sgf file\n",
                lineno);
        fprintf(stderr, "expected symbol: \"%s\"", token_name(tok));
        if (symbol < 256) {
            fprintf(stderr, ", read \"%s\"\n",token_name(symbol));
            sprintf(buf, "sgf line %d: expected symbol: \"%s\", read \"%s\"", 
                    lineno, token_name(tok), token_name(symbol));
            log_fmt_s('W', buf, NULL);
        }
        else {
            fprintf(stderr, ", read \"%s\" %s\n",token_name(symbol), yytext);
            sprintf(buf, "sgf line %d: expected symbol: \"%s\", read \"%s\" %s"
                    , lineno, token_name(tok), token_name(symbol), yytext);
            log_fmt_s('W', buf, NULL);
        }
    }
    //fprintf(stderr, "%s", token_name(symbol));
    //if (symbol==UPPERCASE_STRING || symbol==STRING 
    //    || symbol==NUMBER || symbol==REAL) 
    //    fprintf(stderr, ": %s", yytext);
    //fprintf(stderr, "\n");
}

void ValueType(void)
// ValueType = Point | Number | Real | Text | None
{
    if (symbol == POINT || symbol == STRING 
        || symbol == UPPERCASE_STRING || symbol == LOWERCASE_STRING
        || symbol == NUMBER || symbol == REAL) {
        accept(symbol);
        //fprintf(stderr, "%s ", yytext);
    }
}

int set_default_size(Position *pos) {
    board_set_size_safe(pos, "19"); // default value in sgf spec
    game_clear_board(game);
    size_already_set = 1;
    return 19;
}

void PropValue(void)
// PropValue = "[" ValueType "]"
{
    Position *pos=game->pos;

    accept('['); 
    //fprintf(stderr, "%s\n", yytext);
    read_raw_text = 0;  // reset after the raw text has been read (lookahead)
    ValueType();

    // Property Value was read : use it to modify the game
    int size = board_size(pos);
    if (board_nmoves(pos) < nmoves-1 
        && strcmp(prop_name[prop], "B") == 0) {
        if (!size_already_set)
            size = set_default_size(pos);
        if (yytext[0] != 'B')
            do_play(game, BLACK, parse_sgf_coord(yytext, size));
        else
            do_play(game, BLACK, PASS_MOVE);
    }
    else if (board_nmoves(pos) < nmoves-1 
        && strcmp(prop_name[prop], "W") == 0) {
        if (!size_already_set)
            size = set_default_size(pos);
        if (yytext[0] != 'W')
            do_play(game, WHITE, parse_sgf_coord(yytext, size));
        else
            do_play(game, WHITE, PASS_MOVE);
    }
    else if (strcmp(prop_name[prop], "AB") == 0) {
        if (!size_already_set)
            size = set_default_size(pos);
        Point pt = parse_sgf_coord(yytext, size);
        slist_push(game->placed_black_stones, pt);
        board_place_stone(pos, pt, BLACK);
    }
    else if (strcmp(prop_name[prop], "AW") == 0) {
        if (!size_already_set)
            size = set_default_size(pos);
        Point pt = parse_sgf_coord(yytext, size);
        slist_push(game->placed_white_stones, pt);
        board_place_stone(pos, pt, WHITE);
    }
    else if (strcmp(prop_name[prop], "KM") == 0)
        game_set_komi(game, atof(yytext));
    else if (strcmp(prop_name[prop], "SZ") == 0) {
        if (is_game_board_empty(game)) { 
            board_set_size_safe(pos, yytext);
            game_clear_board(game);
        }
        else {
            // Can happen if SZ occurs after AB or AW
            Point    bstones[BOARDSIZE], wstones[BOARDSIZE];

            slist_clear(bstones); slist_clear(wstones);
            slist_append(bstones, game->placed_black_stones);
            slist_append(wstones, game->placed_white_stones);

            board_set_size_safe(pos, yytext);
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
        size_already_set = 1;
    }
    else if (strcmp(prop_name[prop], "HA") == 0)
        game->handicap = atoi(yytext);
    else if (strcmp(prop_name[prop], "OT") == 0)
        strncpy(game->gi.overtime,yytext,31);
    else if (strcmp(prop_name[prop], "PL") == 0) {
        if (strcmp(yytext, "B") == 0)
            board_set_color_to_play(pos, BLACK);
        else
            board_set_color_to_play(pos, WHITE);
    }
    else if (strcmp(prop_name[prop], "PW") == 0)
        strncpy(game->gi.name[WHITE&1],yytext,31);
    else if (strcmp(prop_name[prop], "PB") == 0)
        strncpy(game->gi.name[BLACK&1],yytext,31);
    else if (strcmp(prop_name[prop], "WR") == 0)
        strncpy(game->gi.rank[WHITE&1],yytext,3);
    else if (strcmp(prop_name[prop], "BR") == 0)
        strncpy(game->gi.rank[BLACK&1],yytext,3);
    else if (strcmp(prop_name[prop], "RE") == 0)
        strncpy(game->gi.result,yytext,31);
    else if (strcmp(prop_name[prop], "RU") == 0)
        strncpy(game->gi.rules,yytext,31);
    else if (strcmp(prop_name[prop], "TM") == 0)
        game->time_init = atoi(yytext);

    accept(']');
}

void PropValues(void)
// Propvalues = { PropValue }
{
    if (symbol == '[') {
        PropValue(); 
        if (sgferr[0] !=0) return;    // early abort
        PropValues();
    }
}

void PropIdent(void)
{
    int will_read_raw_text = 0;
    if (symbol == UPPERCASE_STRING) {
        prop = yyval;
        if (strcmp(yytext,"C")==0 || strcmp(yytext,"PC")==0)
            will_read_raw_text = 1;  // raw text after '['
        //if (yyval > 0 ) 
        //    fprintf(stderr, "prop %s: ",  yytext);
        //else
        //    fprintf(stderr, "unknown %s: ",  yytext);
        if (yyval <=0 )
            log_fmt_s('W',"read unknown property %s in sgf file", yytext);
        accept(UPPERCASE_STRING);
        read_raw_text = will_read_raw_text;       // will be reset by PropValue
    }
}

void Property(void)
// Property = PropIdent { PropValue }
{
    PropIdent(); PropValue(); 
    if (sgferr[0] !=0) return;    // early abort
    PropValues();
    //fprintf(stderr, "\n");
}

void Properties(void)
// Properties = { Property }
{
    if (symbol == UPPERCASE_STRING) {
        Property(); 
        if (sgferr[0] !=0) return;    // early abort
        Properties();
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
        Node(); 
        if (sgferr[0] !=0) return;    // early abort
        RestOfSequence();
    }
}

void Sequence(void)
// Sequence = Node RestOfSequence
{
    Node(); 
    if (sgferr[0] !=0) return;    // early abort
    RestOfSequence();
}

void RestOfCollection(void);
void GameTree(void)
// GameTree = "(" Sequence RestOfCollection ")"
{
    accept('('); 
    Sequence(); 
    if (sgferr[0] !=0)
        return;    // early abort
    RestOfCollection(); 
    accept(')');
}

void RestOfCollection(void)
// RestOfCollection = GameTree RestOfCollection | None
{
    if (symbol == '(') {
        GameTree(); 
        if (sgferr[0] !=0) return;    // early abort
        RestOfCollection();
    }
}

void Collection(void)
// Collection = GameTree RestOfCollection
{
    GameTree(); 
    if (sgferr[0] !=0) return;    // early abort
    RestOfCollection();
}

void check_sgf_validity(const char *filename)
{
    char c1=fgetc(f), c2=fgetc(f);
    if (c1 != '(' || c2 != ';')
        log_fmt_s('E', "File %s does not seem to be a SGF file", filename);
    ungetc(c2, f); ungetc(c1, f);
}

char *loadsgf(Game *sgf_game, const char *filename, int sgf_nmoves)
// Load a position from the SGF file filename using a recursive parser for the
// SGF grammar which is described above. See ref [1].
//
// The modifications of the game struct is done in ProPValue()
//
// Note: the registred game is supposed to be a legal game (no check)
{
    f = fopen(filename, "r");  // File shared between all the routines
    game = sgf_game;           // Game in which all the actions take place
    nmoves = sgf_nmoves;       // Number of moves loaded from sgf file

    game_clear_board(game);
    sgferr[0] = 0;
    lineno = 1;
    buf[0] = 0;
    size_already_set = 0;
    if (f != NULL) {
        check_sgf_validity(filename);
        symbol = yylex();
        Collection();
        assert(symbol == '\n' || symbol == EOF);
        if (sgferr[0]!=0)
            sprintf(buf, "%s", sgferr);
        else {
            if (board_color_to_play(game->pos) == BLACK)
                strcpy(buf,"B");
            else
                strcpy(buf,"W");
        }
        fclose(f);
    }
    else {
        sprintf(buf, "Error - can't open file %s", filename);
    }
    return buf;
}
