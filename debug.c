#include "michi.h"
static char buf[BUFLEN];
static int  delta[] = { -N-1,   1,  N+1,   -1, -N,  W,  N, -W};

//============================= messages logging ==============================
FILE    *flog;             // FILE to log messages
int     c1,c2;             // counters for warning messages
int     nmsg;              // number of written log entries

void too_many_msg(void)
// if too many messages have been logged, print a last error and exit 
{
    fprintf(stderr,"Too many messages have been written in log file "
                       " (maximum 100000)\n");
    fprintf(flog,"Too many messages (maximum 100000)\n");
    exit(-1);
}

void log_fmt_s(char type, const char *msg, const char *s)
// Log a formatted message (string parameter)
{
    fprintf(flog, "%c %5d/%3.3d ", type, c1, c2);
    fprintf(flog, msg, s); fprintf(flog, "\n");
    if(type == 'E') {
        fprintf(stderr,"%c %5d/%3.3d ", type, c1, c2);
        fprintf(stderr, msg, s); fprintf(stderr,"\n");
    }
    if(nmsg++ > 1000000) too_many_msg();
}

void log_fmt_i(char type, const char *msg, int n)
// Log a formatted message (int parameter)
{
    sprintf(buf, msg, n);
    log_fmt_s(type, "%s", buf);
}

void log_fmt_p(char type, const char *msg, Point pt)
// Log a formatted message (point parameter)
{
    char str[8];
    sprintf(buf, msg, str_coord(pt,str));
    log_fmt_s(type,"%s", buf);
}
//============================= debug subcommands =============================
void dump_env4(Byte env4, Byte true_env4)
{
    for (int i=0 ; i<8 ; i++) {
        if (i == 4) fprintf(stderr, " ");
        if (env4 & 128)
            fprintf(stderr, "1");
        else
            fprintf(stderr, "0");
        env4 <<= 1;
    }
    fprintf(stderr, " (true: ");
    for (int i=0 ; i<8 ; i++) {
        if (i==4) fprintf(stderr, " ");
        if (true_env4 & 128)
            fprintf(stderr, "1");
        else
            fprintf(stderr, "0");
        true_env4 <<= 1;
    }
    fprintf(stderr, ")\n");
}

int env4_OK(Position *pos)
{
    FORALL_POINTS(pos,pt) {
        if (point_color(pos,pt) == OUT) continue;
        if (point_env4(pos,pt) != compute_env4(pos, pt, 0)) {
            fprintf(stderr, "%s ERR env4 = ", str_coord(pt,buf));
            dump_env4(point_env4(pos,pt), compute_env4(pos,pt,0));
            return 0;
        }
        if (point_env4d(pos,pt) != compute_env4(pos,pt,4)) {
            fprintf(stderr, "%s ERR env4d = ", str_coord(pt,buf));
            dump_env4(point_env4d(pos,pt), compute_env4(pos,pt,4));
            return 0;
        }
    }
    return 1;
}

int check_block(Position *pos, Point pt)
// return 1 if block at point pt is OK, else 0
{
    Block b=pos->block[pt];
    char  str[8];
    int   nerrs=0;
    Point stones[BOARDSIZE], libs[BOARDSIZE];

    // Check points where there is no stone
    if (point_color(pos,pt) == OUT) {
        if (point_block(pos, pt) == 0)
            return 1;
        else {
            sprintf(buf,"Wrong block number %d at %s (which is OUT)", b,
                                                        str_coord(pt, str));
            log_fmt_s('E', buf, NULL);
            return 0;
        }
    }
    if (point_color(pos,pt) == EMPTY) {
        if (point_block(pos, pt) == 0)
            return 1;
        else {
            sprintf(buf,"Wrong block number %d at %s (which is EMPTY)", b,
                                                        str_coord(pt, str));
            log_fmt_s('E', buf, NULL);
            return 0;
        }
    }

    // Check point where there is a stone
    if (point_block(pos, pt) == 0) {
        sprintf(buf,"Wrong block number %d at %s (which is NOT EMPTY)", b,
                                                           str_coord(pt, str));
        log_fmt_s('E', buf, NULL);
        if(nerrs++ > 10) return 0;
    }
    compute_block(pos, pt, stones, libs, 256);

    // Global verification
    if (block_size(pos,b) != slist_size(stones)) {
        sprintf(buf,"Wrong size of block %d at %s (%d instead of %d)",
                b, str_coord(pt, str), pos->size[b], slist_size(stones));
        log_fmt_s('E', buf, NULL);
        if(nerrs++ > 10) return 0;
    }
    if (block_nlibs(pos,b) != slist_size(libs)) {
        sprintf(buf,"Wrong nlibs of block %d at %s (%d instead of %d)",
                b, str_coord(pt, str), block_nlibs(pos,b), slist_size(libs));
        log_fmt_s('E', buf, NULL);
        if(nerrs++ > 10) return 0;
    }
    if (block_nlibs(pos,b) ==0) {
        sprintf(buf,"Error block %d(%s) with 0 liberty", b, str_coord(pt, str));
        log_fmt_s('E', buf, NULL);
        if(nerrs++ > 10) return 0;
    }

    // Detailed check of stones
    b = point_block(pos,pt);
    FORALL_IN_SLIST(stones, i) {
        if (point_block(pos,i) != b) {
            sprintf(buf,"Invalid block id (%d) at point %s ",
                    point_block(pos,i), str_coord(i, str));
            log_fmt_s('E', buf, NULL);
            if(nerrs++ > 10) return 0;
        }
    }

    // Detailed check of liberties
    FORALL_IN_SLIST(libs, l) {
        int k = (l-N)>>5, r = (l-N)&31;
        if ((pos->libs[b][k] & (1<<r)) == 0) {
            char str1[8];
            sprintf(buf,"Liberty %s not set for block %d(%s) ",
                                  str_coord(l, str), b, str_coord(pt, str1));
            log_fmt_s('E', buf, NULL);
            if(nerrs++ > 10) return 0;
        }
    }

    return nerrs==0;    // OK if nerrs == 0
}

int blocks_OK(Position *pos, Point pt)
// Return 1 (True) if block at point pt and blocks in contact with pt are OK
{
    Block b=pos->block[pt];
    int   k;
    Point n;
    if (check_block(pos, pt)==0) return 0;

    FORALL_NEIGHBORS(pos, pt, k, n)
        if (pos->block[n] != b && check_block(pos, n)==0) return 0;
    return 1;               // if all tests succeed blocks around pt are OK
}

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

char* debug(Position *pos) 
{
    char *command = strtok(NULL," \t\n"), *ret="";
    char *known_commands = "\nblocks_OK\nenv8\nfix_atari\ngen_playout\n"
        "ko\nmatch_pat3\nmatch_pat\nplayout\nprint_mark\nsavepos\nsetpos\n";
    int  amaf_map[BOARDSIZE], owner_map[BOARDSIZE];
    Info sizes[BOARDSIZE];
    Point moves[BOARDSIZE];

    if (strcmp(command, "setpos") == 0) {
        char *str = strtok(NULL, " \t\n");
        while (str != NULL) {
            Point pt = parse_coord(str);
            if (point_color(pos,pt) == EMPTY)
                ret = play_move(pos, pt);        // suppose alternate play
            else if (pt == PASS_MOVE)
                ret = pass_move(pos);
            else
                ret ="Error Illegal move: point not EMPTY\n";
            str = strtok(NULL, " \t\n");
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
        mcplayout(pos, amaf_map, owner_map, 1);
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
            block_compute_libs(pos, b, libs);
            ret = slist_str_as_point(libs);
        }
    }
    else if (strcmp(command, "ko") == 0) {
        if (pos->ko)
            str_coord(pos->ko, buf);
        else
            buf[0] = 0;
        ret = buf;
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
        if (str == NULL)
            return ret = "Error -- point missing";
        Point pt = parse_coord(str);
        if (!point_is_stone(pos,pt)) {
            ret ="Error given point not occupied by a stone";
            return ret;
        }
        is_atari = fix_atari(pos,pt, SINGLEPT_NOK, TWOLIBS_TEST,0,moves, sizes);
        ret = slist_str_as_point(moves);
        int l = strlen(ret);
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
            int env8=(pos->env4d[pt]<<8) + pos->env4[pt];
            print_env8(env8);
        }
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
        
    return ret;
}
