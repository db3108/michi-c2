// ui.c -- User Interface (Command Line Interface + gtp interface)
#include "michi.h"
static char buf[2048];

TreeNode* new_tree_node(Position *pos);
void expand(TreeNode *tree);
void free_tree(TreeNode *tree);
Point tree_search(TreeNode *tree, int n, int owner_map[], int disp);

Point        allpoints[BOARDSIZE];

static char* colstr  = "@ABCDEFGHJKLMNOPQRST";
static int   game_ongoing=1, i;
static char* version = "1.3";

void usage() {
    fprintf(stderr, "\n\n"
"usage: michi mode [config.gtp]\n\n"
"where mode = \n"
"   * gtp         play gtp commands read from standard input\n"
"   * mcbenchmark run a series of playouts (number set in config.gtp)\n"
"   * mcdebug     run a series of playouts (verbose, nb of sims as above)\n"
"   * tsdebug     run a series of tree searches\n"
"and\n"
"   * config.gtp  an optional file containing gtp commands\n\n");
    exit(-1);
}

void begin_game(void) {
    c1++; c2=1;
    sprintf(buf,"BEGIN GAME %d, random seed = %u",c1,idum);
    log_fmt_s('I', buf, NULL);
}

//----------------------------- utility routines ------------------------------
void* michi_malloc(size_t size)
// Michi memory allocator - fatal error if allocation failed
{
    void* t = malloc(size);
    if (t == NULL) {
        log_fmt_i('E',"Memory allocation failed (size = %d bytes)", size);
        exit(-1);
    }
    return t;
}

void* michi_calloc(size_t nmemb, size_t size)
// Michi memory allocator - fatal error if allocation failed
{
    void* t = calloc(nmemb, size);
    if (t == NULL) {
        log_fmt_i('E',"Memory allocation failed (size = %d bytes)", size*nmemb);
        exit(-1);
    }
    return t;
}

Point parse_coord(char *s)
{
    char c, str[10];
    int  x,y;
    for (int i=0 ; i<9 ; i++) {
        if (s[i]==0) {
            str[i]=0;
            break;
        }
        str[i] = toupper(s[i]);
    } 
    if(strcmp(str, "PASS") == 0) return PASS_MOVE;
    sscanf(s, "%c%d", &c, &y);
    c = toupper(c);
    if (c<'J') x = c-'@';
    else       x = c-'@'-1;
    return (N-y+1)*(N+1) + x;
}

char* str_coord(Point pt, char str[8])
{
    if (pt == PASS_MOVE) strcpy(str, "pass");
    else if (pt == RESIGN_MOVE) strcpy(str, "resign");
    else {
        int row = pt/(N+1); int col = (pt%(N+1));
        sprintf(str, "%c%d", '@'+col,N+1-row);
        if (str[0] > 'H') str[0]++;
    }
    return str;
}

char* slist_str_as_int(Slist l) {
    buf[0]=0;
    for (int k=1, n=l[0] ; k<=n ; k++) {
        char s[32];
        sprintf(s, " %d", l[k]);
        strcat(buf, s);
    }
    return buf;
}
char* slist_str_as_point(Slist l) {
    buf[0]=0;
    for (int k=1, n=l[0] ; k<=n ; k++) {
        char str[8], s[8];
        sprintf(s, " %s", str_coord(l[k],str));
        strcat(buf, s);
    }
    return buf;
}

void ppoint(Point pt) {
    char str[8];
    str_coord(pt,str);
    fprintf(stderr,"%s\n",str);
}

void make_pretty(Position *pos, char pretty_board[BOARDSIZE], int *capB
                                                                , int *capW)
{
    for (int k=0 ; k<BOARDSIZE ; k++) {
        if (point_color(pos, k) == BLACK)      pretty_board[k] = 'X';
        else if (point_color(pos, k) == WHITE) pretty_board[k] = 'O';
        else if (point_color(pos, k) == EMPTY) pretty_board[k] = '.';
        else                                   pretty_board[k] = ' ';
    }
    *capW = pos->caps[0];
    *capB = pos->caps[1];
}

void print_pos(Position *pos, FILE *f, int *owner_map)
// Print visualization of the given board position
{
    char pretty_board[BOARDSIZE], strko[8];
    int  capB, capW;

    make_pretty(pos, pretty_board, &capB, &capW);
    fprintf(f,"Move: %-3d   Black: %d caps   White: %d caps   Komi: %.1f",
            pos->n, capB, capW, pos->komi); 
    if (pos->ko)
        fprintf(f,"   ko: %s", str_coord(pos->ko,strko)); 
    fprintf(f,"\n");

    for (int row=1, k=N+1, k1=N+1 ; row<=N ; row++) {
        if (pos->last == k+1) fprintf(f, " %-2d(", N-row+1); 
        else                  fprintf(f, " %-2d ", N-row+1);
        k++;k1++;
        for (int col=1 ; col<=N ; col++,k++) {
            fprintf(f, "%c", pretty_board[k]);
            if (pos->last == k+1)    fprintf(f, "(");
            else if (pos->last == k) fprintf(f, ")");
            else                     fprintf(f, " ");
        }
        if (owner_map) {
            fprintf(f, "   ");
            for (int col=1 ; col<=N ; col++,k1++) {
                char c;
                if      ((double)owner_map[k1] > 0.6*N_SIMS) c = 'X';
                else if ((double)owner_map[k1] > 0.3*N_SIMS) c = 'x';
                else if ((double)owner_map[k1] <-0.6*N_SIMS) c = 'O';
                else if ((double)owner_map[k1] <-0.3*N_SIMS) c = 'o';
                else                                         c = '.';
                fprintf(f, " %c", c);
            }
        }
        fprintf(f, "\n");
    }
    fprintf(f, "    ");
    for (int col=1 ; col<=N ; col++) fprintf(f, "%c ", colstr[col]);
    fprintf(f, "\n\n");
}

//----------------------------- time management -------------------------------
#define max(a, b) ((a)<(b) ? (a) : (b))
float saved_time_left;

int nsims(Position *pos) 
// Compute the number of simulations for the next tree_search.
// Simplistic time management (portable if not very accurate).
// Loosely based on Petr Baudis Thesis.
{
    int nstones, moves_to_play;

    if (pos->time_init <= 0) return N_SIMS;     // Game is not time limited

    saved_time_left -= stop_playouts_sec - start_playouts_sec; 
    if (pos->time_left_gtp > 0) {
        pos->time_left = pos->time_left_gtp;
        sprintf(buf, "time left gtp:%d, (internal %.2f)", pos->time_left, 
                                                            saved_time_left);
        log_fmt_s('I', buf, NULL);
    }
    else {
        pos->time_left = saved_time_left;
    }

    if (nplayouts_per_second <= 0.0) {
        nplayouts = N_SIMS;   //       1st genmove
        return N_SIMS;
    }
    sprintf(buf,"time used:      %6d %7.1f", 
            nplayouts_real, stop_playouts_sec-start_playouts_sec);
    log_fmt_s('I', buf, NULL);
    start_playouts_sec = (float) clock() / (float) CLOCKS_PER_SEC;

    nstones = pos->n - pos->caps[0] - pos->caps[1];        // ignore PASS moves
    moves_to_play = ((N*N*3)/4 - nstones)/2;// Hyp: 25 % of EMPTY points at end
    if (moves_to_play < 30) moves_to_play = 30;
    //float denom = moves_to_play - max(moves_to_play-40, 0); 
    nplayouts = (float) pos->time_left / moves_to_play  * nplayouts_per_second;
    if (nplayouts > 100000) nplayouts = 100000;

    sprintf(buf,"time allocated: %6.0f %7.2f %5d %7.1f", 
            nplayouts, nplayouts / nplayouts_per_second, moves_to_play,
            nplayouts_per_second);
    log_fmt_s('I', buf, NULL);
    nplayouts_real = 0;
    return (int) nplayouts;
}

void update_nplayouts_per_seconds()
// Update the number of playouts per seconds
{
    stop_playouts_sec=(float) clock() / (float) CLOCKS_PER_SEC;
    nplayouts_per_second = nplayouts_real / 
                            (stop_playouts_sec-start_playouts_sec); 
}

//--------------------- functions implementing gtp commands -------------------
char* gogui_analyse_commands(void)
{
    return "param/Params General/param_general\n"
           "param/Params Tree Policy/param_tree\n" 
           "param/Params Playouts/param_playout\n";
}

char* gtp_boardsize(Position *pos)
{
    char *ret, *str = strtok(NULL, " \t\n");
    if(str != NULL) {
        int size = atoi(str);
        if (size != N) {
            sprintf(buf, "Error: Trying to set incompatible boardsize %s"
                         " (!= %d)", str, N);
            log_fmt_s('E', buf, NULL);
            ret = buf;
        }
        else
            ret = "";
    }
    else
        ret = "Error missing boardsize";
    return ret;
}

char* gtp_clear_board(Position *pos, TreeNode *tree)
{
    char *ret;
    if (game_ongoing) begin_game();
    game_ongoing = 0;
    free_tree(tree);
    ret = empty_position(pos);
    tree = new_tree_node(pos);
    nplayouts_per_second = -1.0;
    return ret;
}

char* gtp_cputime(void)
{
    float time_sec = (float) clock() / (float) CLOCKS_PER_SEC;
    sprintf(buf, "%.3f", time_sec);
    return buf;
}

char *gtp_genmove(Position *pos, TreeNode *tree, int *owner_map)
{
    char *ret;
    c2++; game_ongoing = 1;
    Point pt;
    if (!play_until_the_end && pos->last == PASS_MOVE && pos->n>2) {
            log_fmt_s('I', "Opponent pass. I pass", NULL);
            pt = PASS_MOVE;
    }
    else {
        free_tree(tree);
        tree = new_tree_node(pos);
        int n = nsims(pos);
        pt = tree_search(tree, n, owner_map, 0);
        if ((int)nplayouts_real == (int)nplayouts)
            pt = try_search_again(tree, n, owner_map, 0);
        update_nplayouts_per_seconds();
    }
    if (pt == PASS_MOVE)
        pass_move(pos);
    else if (pt != RESIGN_MOVE)
        play_move(pos, pt);
    ret = str_coord(pt, buf);  
    return ret;
}

char* gtp_known_command(char *known_commands)
{
    char *ret, *command = strtok(NULL, " \t\n");
    if (strstr(known_commands,command) != NULL)
        ret = "true";
    else
        ret = "false";
    return ret;
}

char* gtp_komi(Position *pos)
{
    char *ret, *str = strtok(NULL, " \t\n");
    if(str != NULL) {
        if(sscanf(str, "%f", &pos->komi) == 1)
            ret = "";
        else
            ret = "Error - invalid value";
    }
    else
        ret = "Error - a komi value is expected";
    return ret;
}

char* gtp_play(Position *pos)
{
    char *ret;
    c2++; game_ongoing = 1;
    ret = strtok(NULL, " \t\n");            // color is ignored
    char *str = strtok(NULL, " \t\n");
    if(str != NULL) {
        Point pt = parse_coord(str);
        if (point_color(pos,pt) == EMPTY)
            ret = play_move(pos, pt);           // suppose alternate play
        else {
            if(pt == PASS_MOVE) ret = pass_move(pos);
            else ret ="Error Illegal move: point not EMPTY\n";
        }
    }
    else
        ret = "Error - Invalid vertex ";
    return ret;
}

char *gtp_time_left(Position *pos)
{
    char *ret, *str = strtok(NULL, " \t\n");
    if(str != NULL) {
        str = strtok(NULL, " \t\n");
        if(str != NULL) {
            if (sscanf(str,"%d", &pos->time_left_gtp)==1)
                ret = "";
            else {
                sprintf(buf, "Error - %s is not an integer", str);
                ret = buf;
            }
        }
        else
            ret = "Error missing time parameter";
    }
    else
        ret = "Error missing color parameter";
    return ret;
}

char* gtp_time_settings(Position *pos) 
// Time setting (only absolute time is implemented)
{
    char *ret, *str = strtok(NULL, " \t\n");
    if(str != NULL){
        if (sscanf(str,"%d", &pos->time_init)==1) {
            saved_time_left = pos->time_init;
            ret = "";
        }
        else {
            sprintf(buf, "Error - %s is not an integer", str);
            ret = buf;
        }
    }
    else
        ret = "Error missing time parameter";
    return ret;
}

//-------------------------------- main programs ------------------------------
double mcbenchmark(int n, Position *pos, int amaf_map[], int owner_map[])
// run n Monte-Carlo playouts from empty position, return avg. score
{
    double sumscore = 0.0;
    idum = 1;
    for (int i=0 ; i<n ; i++) {
        if (i%10 == 0) {
            if (i%50 == 0) fprintf(stderr, "\n%5d", i);
            fprintf(stderr, " ");
        }
        fprintf(stderr, ".");
        empty_position(pos); memset(amaf_map, 0, BOARDSIZE*sizeof(int)); 
        sumscore += mcplayout(pos, amaf_map, owner_map, 0); 
    }
    fprintf(stderr, "\n");
    return sumscore/n;
}

void gtp_io(FILE *f, FILE *out)
{
    char line[BUFLEN], *cmdid, *command, msg[BUFLEN], *ret;
    char *known_commands="cputime\ndebug subcmd\ngenmove\n"
        "gogui-analyze_commands\nhelp\nknown_command\nkomi\nlist_commands\n"
        "name\nparam_general\nparam_playout\nparam_tree\nplay\n"
        "protocol_version\nquit\ntime_left\ntime_settings\nversion";
    int      *owner_map=michi_calloc(BOARDSIZE, sizeof(int));
    TreeNode *tree;
    Position *pos, pos2;
    
    pos = &pos2;
    empty_position(pos);
    tree = new_tree_node(pos);

    for(;;) {
        ret = "";
        if (fgets(line, BUFLEN, f) == NULL) break;
        line[strlen(line)-1] = 0;
        log_fmt_s('C', line, NULL);
        command = strtok(line, " \t\n");
        if (command == NULL) continue;          // ignore newline
        if (command[0] == '#') continue;        // ignore comment line
        if (sscanf(command, "%d", &i) == 1) {
            cmdid = command;
            command = strtok(NULL, " \t\n");
        }
        else
            cmdid = "";

        if (strcmp(command, "play")==0)
            ret = gtp_play(pos);
        else if (strcmp(command, "genmove") == 0)
            ret = gtp_genmove(pos, tree, owner_map);
        else if (strcmp(command, "time_left") == 0)
            ret = gtp_time_left(pos);
        else if (strcmp(command, "cputime") == 0)
            ret = gtp_cputime();
        else if (strcmp(command, "boardsize") == 0)
            ret = gtp_boardsize(pos);
        else if (strcmp(command, "clear_board") == 0)
            ret = gtp_clear_board(pos,tree);
        else if (strcmp(command,"debug") == 0)
            ret = debug(pos);
        else if (strcmp(command,"gogui-analyze_commands") == 0)
            ret = gogui_analyse_commands();
        else if (strcmp(command,"help") == 0)
            ret = known_commands;
        else if (strcmp(command,"known_command") == 0)
            ret = gtp_known_command(known_commands);
        else if (strcmp(command,"komi") == 0)
            ret = gtp_komi(pos);
        else if (strcmp(command,"list_commands") == 0)
            ret = known_commands;
        else if (strcmp(command,"name") == 0)
            ret = "michi-c";
        else if (strcmp(command,"param_general") == 0)
            ret = param_general();
        else if (strcmp(command,"param_playout") == 0)
            ret = param_playout();
        else if (strcmp(command,"param_tree") == 0)
            ret = param_tree();
        else if (strcmp(command,"protocol_version") == 0)
            ret = "2";
        else if (strcmp(command, "time_settings") == 0)
            ret = gtp_time_settings(pos);
        else if (strcmp(command,"version") == 0)
            ret = version;
        else if (strcmp(command,"quit") == 0) {
            printf("=%s \n\n", cmdid);
            log_hashtable_synthesis();
            break;
        }
        else {
            sprintf(msg, "Warning: Ignoring unknown command - %s\n", command);
            ret = msg;
        }
        if (verbosity > 1) 
            print_pos(pos, stderr, owner_map);

        if ((ret[0]=='E' && ret[1]=='r')
                        || ret[0]=='W') fprintf(out,"?%s %s\n\n", cmdid, ret);
        else                            fprintf(out,"=%s %s\n\n", cmdid, ret);
        fflush(out);
    }
}

int michi_console(int argc, char *argv[]) 
{
    char *command;

    // Initialization of all global data
    flog = fopen("michi.log", "w");
    setbuf(flog, NULL);                // guarantees that log is unbuffered
    log_fmt_s('I',"michi-c Version %s", version);
    log_fmt_i('I',"size of struct Position = %d bytes", sizeof(Position));
    make_pat3set();
    log_fmt_s('I', "time          :  nsims seconds moves  ns/sec", NULL);

    if (argc < 2)
        usage();
    command = argv[1];
    if (argc == 3) {
        FILE *f=fopen(argv[2],"r");
        if (f != NULL) {
            gtp_io(f,flog);             // read gtp commands (setup parameters)
            fclose(f);
        }
        else {
            log_fmt_s('E',"cannot open config file -> %s",argv[2]);
            exit(-1);
        }
    }
    init_large_patterns("patterns.prob", "patterns.spat");

    already_suggested = michi_calloc(1, sizeof(Mark));
    mark1 = michi_calloc(1, sizeof(Mark));
    mark2 = michi_calloc(1, sizeof(Mark));
    Position *pos = michi_malloc(sizeof(Position));
    int      *amaf_map=michi_calloc(BOARDSIZE, sizeof(int)); 
    int      *owner_map=michi_calloc(BOARDSIZE, sizeof(int));
    empty_position(pos);
    TreeNode *tree = new_tree_node(pos);
    expand(tree);
    slist_clear(allpoints);
    FORALL_POINTS(pos,pt)
        if (point_color(pos,pt) == EMPTY) slist_push(allpoints,pt);

    // Execution 
    if (strcmp(command,"gtp") == 0)
        gtp_io(stdin, stdout);
    else if (strcmp(command,"mcdebug") == 0) {
        idum = 1;
        for (int i=0 ; i<N_SIMS ; i++) {
            c1 = i;
            fprintf(stderr, "%lf\n", mcplayout(pos, amaf_map, owner_map, 1)); 
            fprintf(stderr,"mcplayout %d\n", i);
            empty_position(pos);
        }
    }
    else if (strcmp(command,"mcbenchmark") == 0)
        printf("%lf\n", mcbenchmark(N_SIMS, pos, amaf_map, owner_map)); 
    else if (strcmp(command,"tsdebug") == 0) {
        Point move=tree_search(tree, 100, amaf_map, 0);
        fprintf(stderr, "move = %s\n", str_coord(move,buf));
        if (move != PASS_MOVE && move != RESIGN_MOVE)
            play_move(&tree->pos,move);
        print_pos(&tree->pos, stderr, NULL); 
    }
    else
        usage();

    // Cleanup
    free_tree(tree); free(pos);
    free(amaf_map); free(owner_map);
    free(already_suggested); free(mark1); free(mark2);
    fclose(flog);
    return 0;
}
