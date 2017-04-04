#include "michi.h"
static char buf[BUFLEN];

//============================= debug subcommands =============================

char decode_env4(int env4, int pt)
{
    int hi, lo;
    env4 >>= pt;
    hi = (env4>>4) & 1;
    lo = env4 & 1;
    return (hi<<1) + lo;
}

char decode_env8(int env8, int pt)
{
    int c;
    if (pt >= 4)
        c = decode_env4(env8>>8, pt-4);
    else
        c = decode_env4(env8&255, pt);
    switch(c) {
        case 0: return 'O';
        case 1: return 'X';
        case 2: return '.';
        case 3: return '#';
    }
    return 0;
}

void print_env8(int env8)
{
    char src[10];                           // src 0 1 2   bits in env8  7 0 4
    src[0] = decode_env8(env8, 7); // NW           3 4 5   bit 0=LSB     3 . 1
    src[1] = decode_env8(env8, 0); // N            6 7 8                 6 2 5
    src[2] = decode_env8(env8, 4); // NE
    src[3] = decode_env8(env8, 3); // W 
    src[4] = '.';                  // Center
    src[5] = decode_env8(env8, 1); // E     
    src[6] = decode_env8(env8, 6); // SW
    src[7] = decode_env8(env8, 2); // S 
    src[8] = decode_env8(env8, 5); // SE

    printf("env8 = %d\n", env8);
    printf("%c %c %c\n", src[0], src[1], src[2]);
    printf("%c %c %c\n", src[3], src[4], src[5]);
    printf("%c %c %c\n", src[6], src[7], src[8]);
}

void print_marker(Position *pos, Mark *marker)
{
    Position pos2 =*pos;
    FORALL_POINTS(pos2, p)
        if (is_marked(marker, p)) pos2.color[p]='*';
    print_pos(&pos2, stdout, 0);
}

char* debug(Game *game, char *command)
// Analyse the subcommands of the gtp debug command
{
    char *ret="";
    char *known_commands = "\nblocks_OK\nenv8\nfix_atari\ngen_playout\n"
        "match_pat3\nmatch_pat\nplayout\nprint_mark\nsavepos\nsetpos\n"
        "to_play";
    int  *amaf_map=michi_calloc(BOARDSIZE, sizeof(int)); 
    int  *owner_map=michi_calloc(BOARDSIZE, sizeof(int));
    int  *score_count=michi_calloc(2*N*N+1, sizeof(int));
    Info sizes[BOARDSIZE];
    Point moves[BOARDSIZE];
    Position *pos = game->pos;

    if (command == NULL)
        return "Error - Missing parameters";

    if (strcmp(command, "setpos") == 0) {
        char *str = strtok(NULL, " \t\n");
        while (str != NULL) {
            Info m;
            int  played=0;
            Point pt = parse_coord(str);
            if (point_color(pos, pt) == EMPTY) {
                ret = play_move(pos, pt);        // suppose alternate play
                m = pt + (board_captured_neighbors(pos) << 9) 
                       + (board_ko_old(pos) << 13);
                played = 1;
            }
            else if (pt == PASS_MOVE) {
                ret = pass_move(pos);
                m = pt;
                played = 1;
            }
            else
                ret ="Error Illegal move: point not EMPTY\n";
            str = strtok(NULL, " \t\n");
            if (played)
                slist_push(game->moves, m);
        }
    }
    else if (strcmp(command, "savepos") == 0) {
        char *filename = strtok(NULL, " \t\n");
        FILE *f=fopen(filename, "w");
        print_pos(pos, f, NULL);
        fclose(f);
        ret = "";
    }
    else if (strcmp(command, "playout") == 0) {
        memset(score_count, 0, (2*N*N+1)*sizeof(int));
        mcplayout(pos, amaf_map, owner_map, score_count, 1);
    }
    else if (strcmp(command, "blocks_OK") == 0) {
        char *str = strtok(NULL, " \t\n");
        if(str == NULL) ret = "Error missing point";
        else {
            Point pt = parse_coord(str);
            if (blocks_OK(pos, pt)==1)
                ret = "true";
            else
                ret = "false";
        }
    }
    else if (strcmp(command, "libs") == 0) {
        char *str = strtok(NULL, " \t\n");
        if(str == NULL) ret = "Error missing block";
        else {
            Block b = atoi(str);
            Point libs[BOARDSIZE];
            block_compute_libs(pos, b, libs, BOARDSIZE);
            ret = slist_str_as_point(libs);
        }
    }
    else if (strcmp(command, "gen_playout") == 0) {
        char *suggestion = strtok(NULL, " \t\n");
        if (suggestion != NULL) {
            Point last_moves_neighbors[20];
            make_list_last_moves_neighbors(pos, last_moves_neighbors);
            if (strcmp(suggestion, "capture") == 0)
                gen_playout_moves_capture(pos, last_moves_neighbors,
                                            1.0, 0, moves, sizes);
            else if (strcmp(suggestion, "pat3") == 0)
                gen_playout_moves_pat3(pos, last_moves_neighbors,
                                            1.0, moves);
            ret = slist_str_as_point(moves); 
        }
        else
            ret = "Error - missing [capture|pat3]";
    }
    else if (strcmp(command, "match_pat") == 0) {
        char *str = strtok(NULL, " \t\n");
        if(str == NULL) ret = "Error missing point";
        else {
            copy_to_large_board(pos);
            Point pt = parse_coord(str);
            str = strtok(NULL, " \t\n");
            int verbose=1;
            if (str == NULL)
                verbose=0;
            ret = make_list_pat_matching(pt, verbose);
        }
    }
    else if (strcmp(command, "fix_atari") == 0) {
        int is_atari;
        char *str = strtok(NULL, " \t\n");
        if (str == NULL) {
            ret = "Error -- point missing";
            goto finished;
        }
        Point pt = parse_coord(str);
        if (!point_is_stone(pos,pt)) {
            ret ="Error given point not occupied by a stone";
            goto finished;
        }
        is_atari = fix_atari(pos,pt, SINGLEPT_NOK, TWOLIBS_TEST,0,moves, sizes);
        ret = slist_str_as_point(moves);
        int l = (int)strlen(ret);
        for (int k=l+1 ; k>=0 ; k--) ret[k+1] = ret[k];
        if (l>0)
            ret[1] = ' ';
        if (is_atari) ret[0] = '1';
        else          ret[0] = '0';
    }
    else if (strcmp(command, "env8") == 0) {
        char *str = strtok(NULL, " \t\n");
        if(str == NULL) ret = "Error missing point";
        else {
            Point pt = parse_coord(str);
            int env8 = point_env8(pos, pt);
            print_env8(env8);
        }
    }
    else if (strcmp(command, "pos") == 0) {
        char *str = strtok(NULL, " \t\n");
        if (strcmp(str, "last") == 0)
            str_coord(board_last_move(pos), buf);
        else if (strcmp(str, "last2") == 0)
            str_coord(board_last2(pos), buf);
        else if (strcmp(str, "last3") == 0)
            str_coord(board_last3(pos), buf);
        else if (strcmp(str, "ko") == 0) {
           if (board_ko(pos))
              str_coord(board_ko(pos), buf);
           else
              buf[0] = 0;
        }
        else if (strcmp(str, "to_play") == 0) {
            if (board_color_to_play(pos) == BLACK)
                sprintf(buf, "BLACK");
            else 
                sprintf(buf, "WHITE");
        }
        ret = buf;
    }
    else if (strcmp(command, "print_mark") == 0) {
        char *str = strtok(NULL, " \t\n");
        Mark *marker=already_suggested;
        if (strcmp(str, "mark1") == 0) marker = mark1;
        if (strcmp(str, "mark2") == 0) marker = mark2;
        print_marker(pos, marker);
        ret = "";
    }
    else if (strcmp(command, "help") == 0)
        ret = known_commands;
    else {
        sprintf(buf, "unknown debug subcommand - %s", command);
        ret = buf;
    }
finished:    
    free(amaf_map); free(score_count); free(owner_map);
    return ret;
}
