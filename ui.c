// ui.c -- User Interface (Command Line Interface + gtp interface)
#include "michi.h"
static char buf[40000];

void expand(Position *pos, TreeNode *tree);
Point        allpoints[BOARDSIZE];

static int   game_ongoing=1, i;
static char* version = "1.4";

void usage() {
    fprintf(stderr, "\n\n"
"usage: michi mode [config.gtp]\n\n"
"where mode = \n"
"   * gtp         play gtp commands read from standard input\n"
"   * mcbenchmark run a series of playouts (number set in config.gtp)\n"
"   * mcdebug     run a series of playouts (verbose, nb of sims as above)\n"
"   * tsdebug     run a series of tree searches\n"
"   * defaults    write a template of config file on stdout (defaults values)\n"
"and\n"
"   * config.gtp  an optional file containing gtp commands\n\n");
    exit(-1);
}

//----------------------------- utility routines ------------------------------

char* print_score_histogram(Position *pos, int score_count[2*N*N+1]) 
{
    int cmax=0, nc=0, size2=board_size(pos)*board_size(pos);
    for (int i=0; i <= 2*N*N+1 ; i++)
        if (score_count[i] > cmax) cmax = score_count[i];

    if (cmax > 0) {
        for (int s=-size2 ; s <= size2 ; s++) {
            int n = (50*score_count[s+N*N]) / cmax;
            char c;
            if (s < 0)       c = '-';
            else if (s == 0) c = '=';
            else             c = '+';
            nc += sprintf(buf + nc, "%5d %5d ", s, score_count[s+N*N]);
            for (int i=0 ; i < n ; i++)
                nc += sprintf(buf + nc, "%c", c);
            nc += sprintf(buf + nc, "\n");
        }
    }
    return buf;
}

// --------------------- Evaluation of the final Position ---------------------
void final_status_list(Position *pos, Status block_status[MAX_BLOCKS], 
       Status point_status[BOARDSIZE], char *str_st, Point points[BOARDSIZE])
// Compute the status of all points (evaluation from owner_map[])
{
    Block b;
    Status st;

    for (char *p=str_st ; *p != 0 ; p++) 
        *p = tolower(*p);

    if (strcmp(str_st, "dead") == 0)
        st = DEAD;
    else if (strcmp(str_st, "alive") == 0)
        st = ALIVE;
    else
        st = UNKNOWN;

    slist_clear(points);
    FORALL_POINTS(pos, pt) {
        b = point_block(pos, pt);
        if (b != 0  && block_status[b] == st)
                slist_push(points, pt);
    }
}

//--------------------- functions implementing gtp commands -------------------
char* gogui_analyze_commands(void)
{
    return "param/           General Parameters/param_general\n"
           "param/           Tree Policy Parameters/param_tree\n" 
           "param/           Random Policy Parameters/param_playout\n"
           "plist/gfx   Status Dead/final_status_list dead\n"
           "gfx/gfx   Principal variation/principal_variation %c\n"
           "pspairs/gfx   Best moves/best_moves\n"
           "gfx/gfx   Owner Map/owner_map\n"
           "gfx/gfx   Visit Count/visit_count\n"
           "hstring/           Histogram of Score/score_histogram\n";
}

Color get_color(char *str_color) {
    buf[0] = 0;
    char c = toupper(*str_color);
    if (c != 'B' && c != 'W')
        sprintf(buf, "Error - Invalid color \"%s\"", str_color);
    return (c == 'B') ? BLACK : WHITE;
}

char *read_next_color_point_pair(char **strp, Point *pt) 
{
    char *str, *str2;

    str = strtok(NULL, " \t\n");
    if (str != NULL) {
        *str = toupper(*str);
        if (*str != 'B' && *str != 'W') {
            sprintf(buf, "Error - Invalid color \"%s\"", str);
            goto error;
        }
        str2 = strtok(NULL, " \t\n");
        if (str2 == NULL) {
            sprintf(buf, "Error - Missing point");
            goto error;
        }
        *pt = parse_coord(str2);
        if (*pt == INVALID_VERTEX) {
            sprintf(buf, "Error - Invalid vertex \"%s\"", str2);
            goto error;
        }
    }
    *strp = str;
    return "";      // a new pair (color, point) has been read in memory
error:
    *strp = NULL;
    return buf;
}

char* gogui_play_sequence(Game *game)
{
    char *ret, *str_color;
    Point pt;

    ret = read_next_color_point_pair(&str_color, &pt);
    while (str_color != NULL) {
        Color c = get_color(str_color);
        if (buf[0] != 0) {
            ret = buf;
            break;
        }
        ret = do_play(game, c, pt);
        if (ret[0] != 0)
            break;
        else
            game_ongoing = 1;
        ret = read_next_color_point_pair(&str_color, &pt);
    }
    return ret;
}

char* gogui_setup(Game *game)
{
    char *ret, *str;
    Point pt;
    Position *pos=game->pos;

    if (is_game_board_empty(game)) {
        ret = read_next_color_point_pair(&str, &pt);
        while (str != 0) {
            if (strcmp(str,"B") == 0) {
                slist_push(game->placed_black_stones, pt);
                board_set_color_to_play(pos, BLACK);
                ret = play_move(pos, pt);
            }
            else {
                slist_push(game->placed_white_stones, pt);
                board_set_color_to_play(pos, WHITE);
                ret = play_move(pos, pt);
            }
            if (ret[0] != 0) break;
            ret = read_next_color_point_pair(&str, &pt);
        }
        if (ret[0] == 0) {
            board_set_color_to_play(pos, BLACK);
            board_set_nmoves(pos, 0); 
            board_set_last(pos, PASS_MOVE);
            board_set_last2(pos, PASS_MOVE);
            board_set_last3(pos, PASS_MOVE);
        }
        else
            game_clear_board(game);             // restore the position
    }
    else
        ret = "Error - Board not Empty";
    return ret;
}

char* gogui_setup_player(Game *game)
{
    char *ret="", *str;
    Position *pos=game->pos;

    if (board_nmoves(pos) == 0) {
        str = strtok(NULL, " \t\n");
        if (str != NULL) {
            *str = toupper(*str);
            if (strcmp(str,"B") == 0) 
                board_set_color_to_play(pos, BLACK);
            else if (strcmp(str,"W") == 0) {
                board_set_color_to_play(pos, WHITE);
                board_set_nmoves(pos, 1); 
                slist_push(game->moves, PASS_MOVE);
            }
            else {
                sprintf(buf, "Error - Invalid color \"%s\"", str);
                ret = buf;
            }
        }
        else {
            ret = "Error - Missing color";
        }
    }
    else
        ret = "Error - must follow immediatly gogui_setup";
    return ret;
}

char* gtp_boardsize(Position *pos)
{
    char *ret, *str = strtok(NULL, " \t\n");
    if(str != NULL) {
        int size = atoi(str);
        if (size > N) {
            sprintf(buf, "Error: Trying to set incompatible boardsize %s"
                         " (max=%d)", str, N);
            log_fmt_s('W', buf, NULL);
            ret = buf;
        }
        else {
            board_set_size(pos, size);
            empty_position(pos);
            ret = "";
        }
    }
    else
        ret = "Error missing boardsize";
    return ret;
}

void begin_game(void) {
    c1++; c2=1;
    sprintf(buf,"BEGIN GAME %d, random seed = %u",c1,idum);
    log_fmt_s('I', buf, NULL);
}

char* gtp_clear_board(Game *game, TreeNode **tree)
{
    char *ret;

    if (game_ongoing) begin_game();
    game_ongoing = 0;
    free_tree(*tree);
    *tree = new_tree_node();
    nplayouts_per_second = -1.0;
    ret = game_clear_board(game);
    return ret;
}

char* gtp_cputime(void)
{
    float time_sec = (float) clock() / (float) CLOCKS_PER_SEC;
    sprintf(buf, "%.3f", time_sec);
    return buf;
}

char *gtp_final_score(Game *game, int *owner_map, int *score_count)
{
    Score score;
    Position *pos=game->pos;
    Status block_status[BOARDSIZE], point_status[BOARDSIZE];

    compute_all_status(pos, owner_map, score_count, block_status, point_status);
    score = final_score(game, block_status, point_status);
    if (score.raw > 0.0)
        sprintf(buf,"B+%.1lf", score.pessimistic);
    else
        sprintf(buf,"W+%.1lf", -score.pessimistic);
    return buf;
}

char *gtp_final_status_list(Game *game, int *owner_map, int *score_count)
{
    char  *str = strtok(NULL, " \t\n");
    Point points[BOARDSIZE];
    Position *pos=game->pos;
    Status block_status[BOARDSIZE], point_status[BOARDSIZE];

    compute_all_status(pos, owner_map, score_count, block_status, point_status);
    final_status_list(pos, block_status, point_status, str, points);
    return slist_str_as_point(points);
}

char *gtp_genmove(Game *game, TreeNode *tree, int *owner_map, int *score_count)
{
    char *ret;
    Point pt;
    Position *pos = game->pos;

    game_ongoing = 1;
    char  *str = strtok(NULL, " \t\n");
    Color c = get_color(str);
    if (buf[0] != 0) {
        ret = buf;
        goto error;
    }
    
    board_set_color_to_play(pos, c);
    game->computer_color = c;
    pt = genmove(game, tree, owner_map, score_count);
    if (pt == PASS_MOVE)
        pass_move(pos);
    else if (pt != RESIGN_MOVE)
        play_move(pos, pt);
    Info m = pt + (board_captured_neighbors(pos) <<9) 
                + (board_ko_old(pos) << 13) + (c << 22);
    slist_push(game->moves, m);
    ret = str_coord(pt, buf);  
    c2++;
error:
    return ret;
}

char* gtp_undo(Game *game);
char* gtp_gg_undo(Game *game)
{
    char *ret="", *str = strtok(NULL, " \t\n");
    int  n; 

    if (sscanf(str,"%d",&n) == 1) {
        if (n <= board_nmoves(game->pos))
            for (int i=0 ; i<n ; i++) {
                ret = gtp_undo(game);
                if (ret[0] != 0) {
                    sprintf(buf,"%s (in move %d)", ret, i);
                    break;
                }
            }
        else
            ret = "Move list too short";
    }
    else 
        ret = "Parameter not an integer";
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
    char  *ret, *str = strtok(NULL, " \t\n");
    float komi;

    if(str != NULL) {
        if(sscanf(str, "%f", &komi) == 1) {
            board_set_komi(pos, komi);
            ret = "";
        }
        else
            ret = "Error - invalid value";
    }
    else
        ret = "Error - a komi value is expected";
    return ret;
}

char* gtp_loadsgf(Game *game)
{
    char *filename, *ret;
    int  nmoves;

    filename = strtok(NULL, " \t\n");
    if (filename != NULL) {
        ret = strtok(NULL, " \t\n");
        if (ret == NULL) 
            nmoves = 9999;
        else if (sscanf(ret, "%i", &nmoves) != 1)
            nmoves = 9999;
        ret = loadsgf(game, filename, nmoves);
    }
    else
        ret = "Error - missing filename";
    return ret;
}

char* gtp_play(Game *game)
{
    char *ret;

    ret = strtok(NULL, " \t\n");
    Color c = get_color(ret);
    if (buf[0] == 0) {
        char *str = strtok(NULL, " \t\n");
        if(str != NULL) {
            Point pt = parse_coord(str);
            if (pt == INVALID_VERTEX) {
                sprintf(buf,"Error - Invalid vertex %s", str);
                ret = buf;
            }
            else
                ret = do_play(game, c, pt);
        }
        else
            ret = "Error - Missing vertex ";
    }
    else
        ret = buf;
    if (ret[0] != 0)
        game_ongoing = 1;
    return ret;
}

char* gtp_undo(Game *game)
{
    Position *pos = game->pos;
    Point move = slist_pop(game->moves);

    board_set_captured_neighbors(pos, (move >> 9) & 15);
    board_set_ko_old(pos, (move >> 13) & 511);
    char *ret=undo_move(pos);
    board_set_color_to_play(pos, move >> 22);
    if (ret[0]==0) c2--;
    if (board_nmoves(pos) > 3)
        board_set_last3(pos, game->moves[board_nmoves(pos)-2] & 511);
    return ret;
}

char *gtp_set_free_handicap(Game *game, char *str)
{
    char     *ret, s[12];
    Position *pos=game->pos;
    if (is_game_board_empty(game)) {
        ret = slist_from_str_as_point(game->placed_black_stones, str);
        if (ret[0] == 0) {
            FORALL_IN_SLIST(game->placed_black_stones, pt) {
                board_set_color_to_play(pos, BLACK);
                ret = play_move(pos, pt);
                if (ret[0] != 0) {
                    sprintf(buf, "Error invalid vertex %s", str_coord(pt, s));
                    ret = buf;
                    break;
                }
            }
            board_set_color_to_play(pos, WHITE);
            board_set_nmoves(pos, 1); 
            board_set_last(pos, PASS_MOVE);
            board_set_last2(pos, PASS_MOVE);
            board_set_last3(pos, PASS_MOVE);
            slist_clear(game->moves); slist_push(game->moves, PASS_MOVE);
            game->handicap = slist_size(game->placed_black_stones);
            board_set_komi(pos, 0.5);
            ret = "";       // OK
        }
    }
    else
        ret = "Error - Board not Empty";
    return ret;
}

char* gtp_sg_compare_float(void) 
{
    char   *str = strtok(NULL, " \t\n");

    if(str != NULL) {
        if (bestwr < atof(str))
            strcpy(buf, "-1");
        else
            strcpy(buf, "1");
    }
    return buf;
}

char* gtp_storesgf(Game *game)
{
    char *filename, *ret;

    filename = strtok(NULL, " \t\n");
    if (filename != NULL)
        ret = storesgf(game, filename, version);
    else
        ret = "Error - missing filename";
    return ret;
}

char *gtp_time_left(Game *game)
{
    char *ret, *str = strtok(NULL, " \t\n");
    if(str != NULL) {
        str = strtok(NULL, " \t\n");
        if(str != NULL) {
            if (sscanf(str,"%d", &game->time_left_gtp)==1)
                ret = "";       // OK
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

char* gtp_time_settings(Game *game) 
// Time setting (only absolute time is implemented)
{
    char *ret, *str = strtok(NULL, " \t\n");
    if(str != NULL){
        if (sscanf(str,"%d", &game->time_init)==1) {
            saved_time_left = game->time_init;
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

//---------------- gtp commands for doing graphics in gogui -------------------
char* gtp_best_moves(TreeNode *tree)
{
    TreeNode *best_node[5];
    compute_best_moves(tree, "  ", best_node, buf);
    return buf;
}

char* gtp_owner_map(Position *pos, int owner_map[BOARDSIZE])
{
    char str[12], str2[20];
    double nsims=nplayouts_real, p;

    sprintf(buf, "INFLUENCE");
    FORALL_POINTS(pos, pt) {
        if (point_color(pos, pt) != OUT) {
            str_coord(pt, str);
            p = owner_map[pt] / nsims;
            // p = -1 for WHITE, 1 for BLACK absolute ownership of point i
            if (p < -.8)
                p = -1.0;
            else if (p < -.5)
                p = -0.7;
            else if (p < -.2)
                p = -0.4;
            else if (p < 0.2)
                p = 0.0;
            else if (p < 0.5)
                p = 0.4;
            else if (p < 0.8)
                p = 0.7;
            else
                p = 1.0;
            sprintf(str2, " %3s %.1lf", str, p);
            strcat(buf, str2);
        }
    }
    return buf;
}

char* gtp_principal_variation(Position *pos, TreeNode *tree)
{
    char  str[10];
    Color c = board_color_to_play(pos);
    Point moves[6];
    compute_principal_variation(tree, moves);
    strcpy(buf,"VAR");
    FORALL_IN_SLIST(moves, pt) {
        if (c == BLACK) strcat(buf, " W ");
        else            strcat(buf, " B ");
        strcat(buf, str_coord(pt, str));
        c = color_other(c);
    }
    return buf;
}

char* gtp_visit_count(Position *pos, TreeNode *tree)
{
    char str[12], str2[20];
    double dvmax;
    int   vmax=0;
    Point best_move = PASS_MOVE;
    TreeNode *n;

    if (tree->children == NULL) {
        sprintf(buf, "INFLUENCE");
        return buf;
    }

    for(TreeNode **child=tree->children ; *child != NULL ; child++) {
        n = *child;
        if (n->v > vmax) { 
            vmax = n->v;
            best_move = n->move;
        }
    }
    dvmax = vmax;

    sprintf(buf, "INFLUENCE");
    for(TreeNode **child=tree->children ; *child != NULL ; child++) {
        n = *child;
        Point pt = n->move;
        double p = n->v / dvmax;
        p = round(sqrt(p)*10.0)/10.0;
        str_coord(pt, str);
        sprintf(str2, " %3s %.1lf", str, p);
        strcat(buf, str2);
    }
    if (board_color_to_play(pos) == BLACK)
        strcat(buf, "\nVAR w ");
    else
        strcat(buf, "\nVAR b");
    strcat(buf, str_coord(best_move, str));
    return buf;
}

void display_live_gfx(Position *pos, TreeNode *tree,
                                                     int owner_map[BOARDSIZE])
// use the live gfx capability of gogui to do animation during the search
{
    char *ret=buf;
    buf[0]=0;
    if (strcmp(Live_gfx,"owner_map") == 0) 
        ret = gtp_owner_map(pos, owner_map);
    else if (strcmp(Live_gfx,"visit_count") == 0)
        ret = gtp_visit_count(pos, tree);
    else if (strcmp(Live_gfx,"principal_variation") == 0) {
        board_set_color_to_play(pos, color_other(board_color_to_play(pos)));
        ret = gtp_principal_variation(pos, tree);
        board_set_color_to_play(pos, color_other(board_color_to_play(pos)));
    }
    else if (strcmp(Live_gfx,"best_moves") == 0)
        ret = gtp_best_moves(tree);
    fprintf(stderr,"gogui-gfx: %s\n", ret);
}

//-------------------------------- main programs ------------------------------
double mcbenchmark(int n, Position *pos, int amaf_map[], int owner_map[],
                                                      int score_count[2*N*N+1])
// run n Monte-Carlo playouts from empty position, return average score
{
    double sumscore = 0.0;
    idum = 1;
    for (int i=0 ; i<n ; i++) {
        if (i%10 == 0) {
            if (i%100 == 0) {
                if (i%500 == 0) fprintf(stderr, "\n%5d", i);
                fprintf(stderr, " ");
            }
            fprintf(stderr, ".");
        }
        empty_position(pos); memset(amaf_map, 0, BOARDSIZE*sizeof(int)); 
        sumscore += mcplayout(pos, amaf_map, owner_map, score_count, 0); 
    }
    fprintf(stderr, "\n");
    return sumscore/n;
}

void gtp_io(Game *game, FILE *f, FILE *out, int owner_map[], int score_count[])
// Loop that process the gtp commands send by the controller
{
    char line[BUFLEN], *cmdid, *command, msg[BUFLEN], *ret;
    char *known_commands="best_moves\nboardsize\nclear_board\ncputime\n"
        "debug <subcmd>\nfinal_score\nfinal_status_list\ngenmove\n"
        "gogui-analyze_commands\ngogui-play_sequence\ngogui-setup\n"
        "gogui-setup_player\ngg-undo\nhelp\nkgs-genmove_cleanup\n"
        "known_command\nkomi\nloadsgf\nlist_commands\n"
        "name\nowner_map\nparam_general\nparam_playout\nparam_tree\nplay\n"
        "principal_variation\nprotocol_version\nquit\nscore_histogram\n"
        "set_free_handicap\nsg_compare_float\nstoresgf\n"
        "time_left\ntime_settings\nundo\nversion\nvisit_count";
    TreeNode *tree;
    tree = new_tree_node();
    Position *pos = game->pos;

    for(;;) {
        assert(game->moves[0] == board_nmoves(pos));
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
            ret = gtp_play(game);
        else if (strcmp(command, "genmove") == 0)
            ret = gtp_genmove(game, tree, owner_map, score_count);
        else if (strcmp(command, "time_left") == 0)
            ret = gtp_time_left(game);
        else if (strcmp(command, "cputime") == 0)
            ret = gtp_cputime();
        else if (strcmp(command, "undo") == 0)
            ret = gtp_undo(game);
        else if (strcmp(command, "best_moves") == 0)
            ret = gtp_best_moves(tree);
        else if (strcmp(command, "boardsize") == 0)
            ret = gtp_boardsize(pos);
        else if (strcmp(command, "clear_board") == 0)
            ret = gtp_clear_board(game, &tree);
        else if (strcmp(command,"debug") == 0)
            ret = debug(game);
        else if (strcmp(command,"final_score") == 0)
            ret = gtp_final_score(game, owner_map, score_count);
        else if (strcmp(command,"final_status_list") == 0)
            ret = gtp_final_status_list(game, owner_map, score_count);
        else if (strcmp(command,"gogui-analyze_commands") == 0)
            ret = gogui_analyze_commands();
        else if (strcmp(command,"gogui-play_sequence") == 0)
            ret = gogui_play_sequence(game);
        else if (strcmp(command,"gogui-setup") == 0)
            ret = gogui_setup(game);
        else if (strcmp(command,"gogui-setup_player") == 0)
            ret = gogui_setup_player(game);
        else if (strcmp(command,"gg-undo") == 0)
            ret = gtp_gg_undo(game);
        else if (strcmp(command,"help") == 0)
            ret = known_commands;
        else if (strcmp(command,"kgs-genmove_cleanup") == 0) {
            play_until_the_end = 1;
            ret = gtp_genmove(game, tree, owner_map, score_count);
        }
        else if (strcmp(command,"known_command") == 0)
            ret = gtp_known_command(known_commands);
        else if (strcmp(command,"komi") == 0)
            ret = gtp_komi(pos);
        else if (strcmp(command,"loadsgf") == 0)
            ret = gtp_loadsgf(game);
        else if (strcmp(command,"list_commands") == 0)
            ret = known_commands;
        else if (strcmp(command,"name") == 0)
            ret = "michi-c";
        else if (strcmp(command,"owner_map") == 0)
            ret = gtp_owner_map(pos, owner_map);
        else if (strcmp(command,"param_general") == 0)
            ret = param_general();
        else if (strcmp(command,"param_playout") == 0)
            ret = param_playout();
        else if (strcmp(command,"param_tree") == 0)
            ret = param_tree();
        else if (strcmp(command, "principal_variation") == 0)
            ret = gtp_principal_variation(pos, tree);
        else if (strcmp(command,"protocol_version") == 0)
            ret = "2";
        else if (strcmp(command, "score_histogram") == 0)
            ret = print_score_histogram(pos, score_count);
        else if (strcmp(command, "set_free_handicap") == 0) {
            char *str=command + strlen(command) + 1;
            ret = gtp_set_free_handicap(game, str);
        }
        else if (strcmp(command, "sg_compare_float") == 0)
            ret = gtp_sg_compare_float();
        else if (strcmp(command,"storesgf") == 0)
            ret = gtp_storesgf(game);
        else if (strcmp(command, "time_settings") == 0)
            ret = gtp_time_settings(game);
        else if (strcmp(command,"version") == 0)
            ret = version;
        else if (strcmp(command,"visit_count") == 0)
            ret = gtp_visit_count(pos, tree);
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
             || (ret[0]=='W' && ret[1]=='a')) 
                                        fprintf(out,"?%s %s\n\n", cmdid, ret);
        else                            fprintf(out,"=%s %s\n\n", cmdid, ret);
        fflush(out);
    }
    free_tree(tree);
}

int michi_console(int argc, char *argv[]) 
{
    char *command;
    double score;

    // Setup log
    flog = fopen("michi.log", "w");
    setbuf(flog, NULL);                // guarantees that log is unbuffered
    log_fmt_s('I',"michi-c Version %s", version);
    log_fmt_i('I',"size of struct Position = %d bytes", sizeof(Position));
    log_fmt_i('I',"size of struct TreeNode = %d bytes", sizeof(TreeNode));
    log_fmt_i('I',"size of int       = %d bytes", sizeof(int));
    log_fmt_i('I',"size of long      = %d bytes", sizeof(long));
    log_fmt_i('I',"size of long long = %d bytes", sizeof(long long));
    log_fmt_s('T', "time          :  nsims seconds  sims/s moves tleft", NULL);
    log_fmt_s('S', "search: idum dkomi 1st (2nd) rep best2  bestr bestwr "
                   "games/s", NULL);

    // Run time checks that michi requirements are fulfilled
    if (sizeof(int) < 4)
        fatal_error("michi-c needs size of int >= 4 bytes (see michi.log)");
    if (sizeof(long long) < 8)  // should never happen by C99 standard
        fatal_error("michi-c needs size of long long >= 8 bytes(see michi.log)");

    // Initialization of all global data
    make_pat3set();
    already_suggested = michi_calloc(1, sizeof(Mark));
    board_init();
    int      *amaf_map=michi_calloc(BOARDSIZE, sizeof(int)); 
    int      *owner_map=michi_calloc(BOARDSIZE, sizeof(int));
    int      *score_count=michi_calloc(2*N*N+1, sizeof(int));
    Position *pos = new_position();
    Game *game = new_game(pos);
    slist_clear(allpoints);
    FORALL_POINTS(pos,pt)
        if (point_color(pos,pt) == EMPTY) slist_push(allpoints,pt);

    if (argc < 2)
        usage();
    command = argv[1];
    if (argc == 3) {
        // Optional initialization file (contains gtp commands)
        FILE *f=fopen(argv[2],"r");
        if (f != NULL) {
            gtp_io(game, f,flog, owner_map, score_count);
            fclose(f);
        }
        else {
            log_fmt_s('E',"cannot open config file -> %s",argv[2]);
            exit(-1);
        }
    }
    init_large_patterns("patterns.prob", "patterns.spat");

    // Execution 
    if (strcmp(command,"gtp") == 0)
        gtp_io(game, stdin, stdout, owner_map, score_count);
    else if (strcmp(command,"mcdebug") == 0) {
        idum = 1;
        int disp=0;
        memset(owner_map, 0, BOARDSIZE*sizeof(int));
        memset(score_count, 0, (2*N*N+1)*sizeof(int));
        for (int i=0 ; i<N_SIMS ; i++) {
            c1 = i;
            if (i==100000) disp = 1;
            score = mcplayout(pos, amaf_map, owner_map, score_count, disp);
            print_pos(pos, stderr, NULL);
            fprintf(stderr, "mcplayout %4d %12u %6.1lf\n", i, idum, score); 
            empty_position(pos);
        }
    }
    else if (strcmp(command,"mcbenchmark") == 0) {
        memset(owner_map, 0, BOARDSIZE*sizeof(int));
        memset(score_count, 0, (2*N*N+1)*sizeof(int));
        printf("%lf\n", 
                mcbenchmark(N_SIMS, pos, amaf_map, owner_map, score_count)); 
    }
    else if (strcmp(command,"tsdebug") == 0) {
        TreeNode *tree = new_tree_node();
        empty_position(pos);
        expand(pos, tree);
        memset(owner_map, 0, BOARDSIZE*sizeof(int));
        memset(score_count, 0, (2*N*N+1)*sizeof(int));
        Point move=tree_search(pos, tree, N_SIMS, owner_map, score_count, 0);
        fprintf(stderr, "move = %s\n", str_coord(move,buf));
        if (move != PASS_MOVE && move != RESIGN_MOVE)
            play_move(pos,move);
        print_pos(pos, stderr, NULL); 
        free_tree(tree);
    }
    else if (strcmp(command,"defaults") == 0)
        make_params_default(stdout);
    else
        usage();

    // Cleanup
    free_game(game); 
    free(amaf_map); free(score_count); free(owner_map);
    free(already_suggested);
    free_large_patterns();
    board_finish();
    fclose(flog);
    return 0;
}
