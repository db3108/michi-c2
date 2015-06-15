// michi.c -- A minimalistic Go-playing engine
/*
This is a recoding in C (for speed) of the michi.py code by Petr Baudis
avalaible at https://github.com/pasky/michi .

(c) 2015 Petr Baudis <pasky@ucw.cz> Denis Blumstein <db3108@free.fr>
MIT licence (i.e. almost public domain)

The following comments are taken almost verbatim from the michi.py code

A minimalistic Go-playing engine attempting to strike a balance between
brevity, educational value and strength.  It can beat GNUGo on 13x13 board
on a modest 4-thread laptop.

To start reading the code, begin either:
* Bottom up by looking at the goban implementation (in board.c) - starting with
  the Position struct (in michi.h), empty_position() and play_move() functions.
* in the middle of this file by looking at the Monte Carlo playout 
  implementation, starting with the mcplayout() function.
* Top down, by looking at the MCTS implementation, starting with the
  tree_search() function. It is just a loop of tree_descend(),
  mcplayout() and tree_update() round and round.

It may be better to jump around a bit instead of just reading straight
from start to end.

The source is composed in 6 independent parts
- Board routines (in board.c)
- Go heuristics (this file)
- Monte Carlo Playout policy (this file)
- Monte Carlo Tree search (this file)
- User Interface (in ui.c, params.c and debug.c)
- Pattern code : 3x3 and large patterns (in patterns.c)

In C, functions prototypes must be declared before use. 
In order to avoid these declarations, functions are defined before they are 
used, which has the same effect.
This means that the higher level functions are found towards the bottom of
this file. This may not be a good idea in terms of readibility but at least
the order is the same as in the michi python code.

Short bibliography
------------------
[1] Martin Mueller, Computer Go, Artificial Intelligence, Vol.134, No 1-2,
    pp 145-179, 2002
[2] Remi Coulom.  Efficient Selectivity and Backup Operators in Monte-Carlo Tree
    Search.  Paolo Ciancarini and H. Jaap van den Herik.  5th International 
    Conference on Computer and Games, May 2006, Turin, Italy.  2006. 
    <inria-00116992>
[3] Sylvain Gelly, Yizao Wang, Remi Munos, Olivier Teytaud.  Modification of UCT
    with Patterns in Monte-Carlo Go. [Research Report] RR-6062, 2006.
    <inria-00117266v3>
[4] David Stern, Ralf Herbrich, Thore Graepel, Bayesian Pattern Ranking for Move
    Prediction in the Game of Go, In Proceedings of the 23rd international 
    conference on Machine learning, pages 873–880, Pittsburgh, Pennsylvania, 
    USA, 2006
[5] Rémi Coulom. Computing Elo Ratings of Move Patterns in the Game of Go. 
    In ICGA Journal (2007), pp 198-208.
[6] Sylvain Gelly, David Silver. Achieving Master Level Play in 9×9 Computer Go.
    Proceedings of the Twenty-Third AAAI Conference on Artificial Intelligence 
    (2008)
[7] Albert L Zobrist. A New Hashing Method with Application for Game Playing.
    Technical Report #88. April 1970
[8] Petr Baudis. MCTS with Information Sharing, PhD Thesis, 2011
[9] Robert Sedgewick, Algorithms in C, Addison-Wesley, 1990

+ many other PhD thesis accessible on the WEB

[1] can be consulted for the definition of Computer Go terms : 
    points, blocks, eyes, false eyes, liberties, etc.
    and historical bibliography
*/
#include "michi.h"

//========================= Definition of Data Structures =====================
// Given a board of size NxN (N=9, 19, ...), we represent the position
// as an (N+1)*(N+2)+1 Byte array, with 0 (empty), 2 (white), 3 (black), 
// and 1 (off-board border to make rules implementation easier).  
// Coordinates are just indices in this array.
//
// -------------------------------- Global Data -------------------------------
Mark         *mark1, *mark2, *already_suggested;
char         buf[BUFLEN];

//================================ Go heuristics ==============================
// The couple of functions read_ladder_attack / fix_atari is maybe the most 
// complicated part of the whole program (sadly). 
// Feel free to just TREAT IT AS A BLACK-BOX, it's not really that interesting!

Point read_ladder_attack(Position *pos, Point pt, Slist libs)
// Check if a capturable ladder is being pulled out at pt and return a move
// that continues it in that case. Expects its two liberties in libs.
// Actually, this is a general 2-lib capture exhaustive solver.
{
    Point moves[5], sizes[5];   // 4 points should be enough ...
    Point move=0;
    FORALL_IN_SLIST(libs, l) {
        Position pos_l = *pos;
        char *ret = play_move(&pos_l, l);
        if (ret[0]!=0) continue; // move not legal
        // fix_atari() will recursively call read_ladder_attack() back
        // however, ignore 2lib groups as we don't have time to chase them
        slist_clear(moves); slist_clear(sizes);
        int is_atari = fix_atari(&pos_l, pt, SINGLEPT_NOK, TWOLIBS_TEST_NO
                                                           , 0, moves, sizes);
        // if block is in atari and cannot escape, it is caugth in a ladder
        if (is_atari && slist_size(moves) == 0) 
            move = l; 
    }
    return move;   // ladder attack not successful
}

int fix_atari(Position *pos, Point pt, int singlept_ok
        , int twolib_test, int twolib_edgeonly, Slist moves, Slist sizes)
// An atari/capture analysis routine that checks the group at Point pt,
// determining whether (i) it is in atari (ii) if it can escape it,
// either by playing on its liberty or counter-capturing another group.
//
// Return 1 (true) if the group is in atari, 0 otherwise
//        moves : a list of moves that capture or save blocks
//        sizes : list of same lenght as moves (size of corresponding blocks)
// singlept_ok!=0 means that we will not try to save one-point groups
{
    Block b = point_block(pos, pt);
    int   in_atari=1, maxlibs=3;
    Point l, libs[5], blocks[256], blibs[5];

    slist_clear(moves); slist_clear(sizes);
    if (singlept_ok && block_size(pos, b) == 1) return 0;
    if (block_nlibs(pos,b) >= 2) { 
        if (twolib_test && block_nlibs(pos,b) == 2 && block_size(pos,b) > 1) {
            block_compute_libs(pos, b, libs, maxlibs);
            if (twolib_edgeonly
                   && ((line_height(libs[1]))>0 || (line_height(libs[2]))>0)) {
                // no expensive ladder check
                return 0;
            }
            else {
                // check that the block cannot be caught in a working ladder
                // If it can, that's as good as in atari, a capture threat.
                // (Almost - N/A for countercaptures.)
                Point ladder_attack = read_ladder_attack(pos, pt, libs);
                if (ladder_attack) {
                    if(slist_insert(moves, ladder_attack))
                        slist_push(sizes, block_size(pos, b));
                }
            }
        } 
        return 0;  
    }

    block_compute_libs(pos, b, libs, maxlibs);
    Color other=color_other(pos->to_play);
    if (point_color(pos, pt) == other) { 
        // - this is opponent's group, that's enough to capture it
        if (slist_insert(moves, libs[1])) 
            slist_push(sizes, block_size(pos, b));
        return in_atari;
    }

    // This is our group and it is in atari
    // Before thinking about defense, what about counter-capturing a neighbor ?
    make_list_neighbor_blocks_in_atari(pos, b, blocks, pt);
    FORALL_IN_SLIST(blocks, b1) {
        block_compute_libs(pos, b1, blibs, 1);
        if (slist_insert(moves, blibs[1]))
            slist_push(sizes, block_size(pos, b1));
    }

    l = libs[1];
    // We are escaping.  
    // Will playing our last liberty gain/ at least two liberties?
    Position escpos = *pos;
    char *ret = play_move(&escpos, l);
    if (ret[0]!=0)
        return 1;     // oops, suicidal move
    b = point_block(&escpos, l);
    if (block_nlibs(&escpos,b) >= 2) {
        // Good, there is still some liberty remaining - but if it's just the 
        // two, check that we are not caught in a ladder... (Except that we 
        // don't care if we already have some alternative escape routes!)
        if (slist_size(moves)>1 || block_nlibs(&escpos,b)>=3) { 
            if (slist_insert(moves, l))
                slist_push(sizes, block_size(&escpos, b));
        }
        else if (block_nlibs(&escpos,b)==2) {
            block_compute_libs(&escpos,b,libs,2);
            if (read_ladder_attack(&escpos,l,libs) == 0)
                if (slist_insert(moves, l))
                    slist_push(sizes, block_size(&escpos, b));
        }
    }
    return in_atari;
}

//========================= Montecarlo playout policy =========================
int gen_playout_moves_capture(Position *pos, Slist heuristic_set, float prob,
                                    int expensive_ok, Slist moves, Slist sizes)
// Compute list of candidate next moves in the order of preference (capture)
// heuristic_set is the set of coordinates considered for applying heuristics;
// this is the immediate neighborhood of last two moves in the playout, but
// the whole board while prioring the tree.
{
    int   k, twolib_edgeonly = !expensive_ok;
    Point move2[20], size2[20];

    slist_clear(moves); slist_clear(sizes);
    if (random_int(10000) <= prob*10000.0)
        FORALL_IN_SLIST(heuristic_set, pt)
            if (point_is_stone(pos, pt)) {
                fix_atari(pos, pt, SINGLEPT_NOK, TWOLIBS_TEST,
                                                twolib_edgeonly, move2, size2);
                k=1;
                FORALL_IN_SLIST(move2, move)
                    if (slist_insert(moves, move))
                        slist_push(sizes, size2[k++]);
            }
    return slist_size(moves);
}

int gen_playout_moves_pat3(Position *pos, Slist heuristic_set, float prob,
                                                                Slist moves)
// Compute list of candidate next moves in the order of preference (3x3 pattern)
// heuristic_set is the set of coordinates considered for applying heuristics;
// this is the immediate neighborhood of last two moves in the playout, but
// the whole board while prioring the tree.
{
    slist_clear(moves);
    mark_init(already_suggested);
    if (random_int(10000) <= prob*10000.0)
        FORALL_IN_SLIST(heuristic_set, pt)
            if (point_color(pos,pt) == EMPTY && pat3_match(pos, pt))
               slist_push(moves, pt); 
    mark_release(already_suggested);
    return slist_size(moves);
}

int gen_playout_moves_random(Position *pos, Point moves[BOARDSIZE], Point i0)
// Generate a list of moves (includes false positives - suicide moves;
// does not include true-eye-filling moves), starting from a given board index
// (that can be used for randomization)
{
    Color c=pos->to_play;
    slist_clear(moves);
    for(Point i=i0 ; i<BOARD_IMAX ; i++) {
        if (point_color(pos,i) != EMPTY) continue;   // ignore NOT EMPTY Points
        if (is_eye(pos,i) == c) continue;        // ignore true eyes for player
        slist_push(moves, i); 
    }
    for(Point i=BOARD_IMIN-1 ; i<i0 ; i++) {
        if (point_color(pos,i) != EMPTY) continue;   // ignore NOT EMPTY Points
        if (is_eye(pos,i) == c) continue;        // ignore true eyes for player
        slist_push(moves, i); 
    }
    return slist_size(moves);
}

Point choose_random_move(Position *pos, Point i0, int disp)
// Replace the sequence gen_playout_moves_random(); choose_from()
{
    char   *ret;
    Color c=pos->to_play;
    Info     sizes[20];
    Point    ds[20], i=i0, move=PASS_MOVE;
    Position saved_pos = *pos;

    do {
        if (point_color(pos,i) != EMPTY) goto not_found; 
        if (is_eye(pos,i) == c) goto not_found;  // ignore true eyes for player
        ret = play_move(pos, i);
        if (ret[0] == 0) {    // move OK
            move = i;
            // check if the suggested move did not turn out to be a self-atari
            int r = random_int(10000), tstrej;
            tstrej = r<=10000.0*PROB_RSAREJECT;
            if (tstrej) {
                slist_clear(ds); slist_clear(sizes);
                fix_atari(pos, i, SINGLEPT_OK, TWOLIBS_TEST, 1, ds, sizes);
                if (slist_size(ds) > 0) {
                    if(disp) fprintf(stderr, "rejecting self-atari move %s\n",
                                                           str_coord(i, buf));
                    *pos = saved_pos; // undo move;
                    move = PASS_MOVE;
                    goto not_found;
                }
            }
            break;
        }
not_found:
        i++;
        if (i >= BOARD_IMAX) i = BOARD_IMIN - 1;
    } while (i!=i0);
    return move;
}

Point choose_from(Position *pos, Slist moves, char *kind, int disp)
{
    char   *ret;
    Info   sizes[20];
    Point  move = PASS_MOVE, ds[20];
    Position saved_pos = *pos;

    FORALL_IN_SLIST(moves, pt) {
        if (disp && strcmp(kind, "random")!=0)
            fprintf(stderr,"move suggestion (%s) %s\n", kind,str_coord(pt,buf));
        ret = play_move(pos, pt);
        if (ret[0] == 0) {    // move OK
            move = pt;
            // check if the suggested move did not turn out to be a self-atari
            int r = random_int(10000), tstrej;
            if (strcmp(kind,"random") == 0) tstrej = r<=10000.0*PROB_RSAREJECT;
            else                            tstrej = r<= 10000.0*PROB_SSAREJECT;
            if (tstrej) {
                slist_clear(ds); slist_clear(sizes);
                fix_atari(pos, pt, SINGLEPT_OK, TWOLIBS_TEST, 1, ds, sizes);
                if (slist_size(ds) > 0) {
                    if(disp) fprintf(stderr, "rejecting self-atari move %s\n",
                                                           str_coord(pt, buf));
                    *pos = saved_pos; // undo move;
                    move = PASS_MOVE;
                    continue;
                }
            }
            break;
        }
    }
    return move;
}

double score(Position *pos, int owner_map[])
// compute score for to-play player; this assumes a final position with all 
// dead stones captured and only single point eyes on the board ...
{
    double s=0.0;

    FORALL_POINTS(pos,pt) {
        Color c = point_color(pos, pt);
        if (c == EMPTY) c = is_eyeish(pos,pt);
        if (c == BLACK) {
            s += 1.0;
            owner_map[pt]++;
        }
        else if (c == WHITE) {
            s -= 1.0;
            owner_map[pt]--;
        }
    }
    if (pos->to_play == BLACK) return s - pos->komi;
    else                       return -s + pos->komi;
}

double mcplayout(Position *pos, int amaf_map[], int owner_map[], int disp)
// Start a Monte Carlo playout from a given position, return score for to-play
// player at the starting position; amaf_map is board-sized scratchpad recording// who played at a given position first
{
    double s=0.0;
    int    passes=0, start_n=pos->n;
    Info   sizes[BOARDSIZE];
    Point  last_moves_neighbors[20], moves[BOARDSIZE], move;
    if(disp) fprintf(stderr, "** SIMULATION **\n");

    while (passes < 2 && pos->n < MAX_GAME_LEN) {
        //c2 = pos->n;
        move = 0;
        if(disp) print_pos(pos, stderr, NULL);
        // We simply try the moves our heuristics generate, in a particular
        // order, but not with 100% probability; this is on the border between
        // "rule-based playouts" and "probability distribution playouts".
        make_list_last_moves_neighbors(pos, last_moves_neighbors);

        // Capture heuristic suggestions
        if (gen_playout_moves_capture(pos, last_moves_neighbors,
                               PROB_HEURISTIC_CAPTURE, 0, moves, sizes))
            if((move=choose_from(pos, moves, "capture", disp)) != PASS_MOVE)
                goto found;

        // 3x3 patterns heuristic suggestions
        if (gen_playout_moves_pat3(pos, last_moves_neighbors,
                                           PROB_HEURISTIC_PAT3, moves))
            if((move=choose_from(pos, moves, "pat3", disp)) != PASS_MOVE) 
                goto found;
            
        move = choose_random_move(pos, BOARD_IMIN-1+random_int(N*(N+1)), disp);
found:
        if (move == PASS_MOVE) {      // No valid move : pass
            pass_move(pos);
            passes++;
        }
        else {
            if (amaf_map[move] == 0)      // mark the point with 1 for BLACK
                // pos->n-1 because in michi.py pos is updated after this line
                amaf_map[move] = ((pos->n-1)%2==0 ? 1 : -1);
            passes=0;
        }
        //print_pos(pos, stderr, 0);
    }
    s = score(pos, owner_map);
    if (start_n%2 != pos->n%2) s = -s;
    return s;
}
//========================== Montecarlo tree search ===========================
TreeNode* new_tree_node(Position *pos)
{
    TreeNode *node = michi_calloc(1,sizeof(TreeNode));
    node->pos = *pos;
    node->pv = PRIOR_EVEN; node->pw = PRIOR_EVEN/2;
    return node;
}

void expand(TreeNode *tree)
// add and initialize children to a leaf node
{
    char     cfg_map[BOARDSIZE];
    int      nchildren = 0;
    Info     sizes[BOARDSIZE];
    Point    moves[BOARDSIZE];
    Position pos2;
    TreeNode *childset[BOARDSIZE], *node;
    if (tree->pos.last!=PASS_MOVE)
        compute_cfg_distances(&tree->pos, tree->pos.last, cfg_map);

    // Use light random playout generator to get all the empty points (not eye)
    gen_playout_moves_random(&tree->pos, moves, BOARD_IMIN-1);

    tree->children = michi_calloc(slist_size(moves)+1, sizeof(TreeNode*));
    FORALL_IN_SLIST(moves, pt) {
        pos2 = tree->pos;
        assert(point_color(&tree->pos, pt) == EMPTY);
        char* ret = play_move(&pos2, pt);
        if (ret[0] != 0) continue;
        // pt is a legal move : we build a new node for it
        childset[pt]= tree->children[nchildren++] = new_tree_node(&pos2);
    }
    tree->nchildren = nchildren;

    // Update the prior for the 'capture' and 3x3 patterns suggestions
    gen_playout_moves_capture(&tree->pos, allpoints, 1, 1, moves, sizes);
    int k=1;
    FORALL_IN_SLIST(moves, pt) {
        pos2 = tree->pos;
        char* ret = play_move(&pos2, pt);
        if (ret[0] != 0) continue;
        node = childset[pt];
        if (sizes[k] == 1) {
            node->pv += PRIOR_CAPTURE_ONE;
            node->pw += PRIOR_CAPTURE_ONE;
        }
        else {
            node->pv += PRIOR_CAPTURE_MANY;
            node->pw += PRIOR_CAPTURE_MANY;
        }
        k++;
    }
    gen_playout_moves_pat3(&tree->pos, allpoints, 1, moves);
    FORALL_IN_SLIST(moves, pt) {
        pos2 = tree->pos;
        char* ret = play_move(&pos2, pt);
        if (ret[0] != 0) continue;
        node = childset[pt];
        node->pv += PRIOR_PAT3;
        node->pw += PRIOR_PAT3;
    }

    // Second pass setting priors, considering each move just once now
    copy_to_large_board(&tree->pos);    // For large patterns
    for (int k=0 ; k<tree->nchildren ; k++) {
        node = tree->children[k];
        Point pt = node->pos.last;

        if (tree->pos.last != PASS_MOVE && cfg_map[pt]-1 < LEN_PRIOR_CFG) {
            node->pv += PRIOR_CFG[cfg_map[pt]-1];
            node->pw += PRIOR_CFG[cfg_map[pt]-1];
        }

        int height = line_height(pt);  // 0-indexed
        if (height <= 2 && empty_area(&tree->pos, pt, 3)) {
            // No stones around; negative prior for 1st + 2nd line, positive
            // for 3rd line; sanitizes opening and invasions
            if (height <= 1) {
                node->pv += PRIOR_EMPTYAREA;
                node->pw += 0;
            }
            if (height == 2) {
                node->pv += PRIOR_EMPTYAREA;
                node->pw += PRIOR_EMPTYAREA;
            }
        }

        fix_atari(&node->pos, pt, SINGLEPT_OK, TWOLIBS_TEST, !TWOLIBS_EDGE_ONLY,
                                                                 moves, sizes);
        if (slist_size(moves) > 0) {
            node->pv += PRIOR_SELFATARI;
            node->pw += 0;  // negative prior
        }

        double patternprob = large_pattern_probability(pt);
        if (patternprob > 0.0) {
            double pattern_prior = sqrt(patternprob);       // tone up
            node->pv += pattern_prior * PRIOR_LARGEPATTERN;
            node->pw += pattern_prior * PRIOR_LARGEPATTERN;
        }
    }

    if (tree->nchildren == 0) {
        // No possible move, add a pass move
        pos2 = tree->pos;
        pass_move(&pos2);
        tree->children[0] = new_tree_node(&pos2);
        tree->nchildren = 1;
    }
}

void free_tree(TreeNode *tree)
// Free memory allocated for the tree
{
    if (tree->children != NULL) {
        for (TreeNode **child = tree->children ; *child != NULL ; child++)
                free_tree(*child);
        free(tree->children);
    }
    free(tree);
}

double rave_urgency(TreeNode *node)
{
    double v = node->v + node->pv;
    double expectation = (node->w + node->pw)/v;
    if (node->av==0) return expectation;

    double rave_expectation = (double) node->aw / (double) node->av;
    double beta = node->av / (node->av + v + (double)v*node->av/RAVE_EQUIV);
    return beta * rave_expectation + (1-beta) * expectation;
}

double winrate(TreeNode *node)
{
    double wr;
    if (node->v>0) wr = (double) node->w / (double) node->v;
    else           wr = -0.1;
    return wr;
}

TreeNode* best_move(TreeNode *tree, TreeNode **except)
// best move is the most simulated one (avoiding nodes in except list)
{
    int vmax=-1;
    TreeNode *best=NULL;

    if (tree->children == NULL) return NULL;

    for (TreeNode **child = tree->children ; *child != NULL ; child++) {
        if ((*child)->v > vmax) {
            int update = 1;
            if (except != NULL) 
                for (TreeNode **n=except ; *n!=NULL ; n++)
                    if (*child == *n) update=0;
            if (update) {
                vmax = (*child)->v;
                best = (*child);
            }
        }
    }
    return best;
}

TreeNode* most_urgent(TreeNode **children, int nchildren, int disp)
{
    int    k=0;
    double urgency, umax=0;
    TreeNode *urgent = children[0];

    // Randomize the order of the nodes
    SHUFFLE(TreeNode *, children, nchildren);

    for (TreeNode **child = children ; *child != NULL ; child++) {
        if (disp)
            dump_subtree(*child, N_SIMS/50, "", stderr, 0);
        urgency = rave_urgency(*child);
        if (urgency > umax) {
            umax = urgency;
            urgent = *child;
        }
        k++;
    }
    return urgent;
}

int tree_descend(TreeNode *tree, int amaf_map[], int disp, TreeNode **nodes)
// Descend through the tree to a leaf
{
    int last=0, passes = 0;
    Point move;
    //tree->v += 1;
    nodes[last] = tree;
   
    while (nodes[last]->children != NULL && passes <2) {
        if (disp) print_pos(&nodes[last]->pos, stderr, NULL);
        // Pick the most urgent child
        TreeNode *node = most_urgent(nodes[last]->children, 
                                            nodes[last]->nchildren, disp);
        nodes[++last] = node;
        move = node->pos.last;
        if (disp) { fprintf(stderr, "chosen "); ppoint(move); }

        if (move == PASS_MOVE) passes++;
        else {
            passes = 0;
            if (amaf_map[move] == 0) //Mark the point with 1 for black
                amaf_map[move] = (nodes[last-1]->pos.n%2==0 ? 1 : -1);
        }

        if (node->children == NULL && node->v >= EXPAND_VISITS)
            expand(node);
    }
    return last;
}

void tree_update(TreeNode **nodes,int last,int amaf_map[],double score,int disp)
// Store simulation result in the tree (nodes is the tree path)
{
    for (int k=last ; k>=0 ; k--) {     // walk nodes from leaf to the root
        TreeNode *n= nodes[k];
        if(disp) {
            char str[8]; str_coord(n->pos.last,str);
            fprintf(stderr, "updating %s %d\n", str, score<0.0); 
        }
        n->v += 1;         // TODO put it in tree_descend when parallelize
        n->w += score<0.0; // score is for to-play, node stats for just-played
        
        // Update the node children AMAF stats with moves we made 
        // with their color
        int amaf_map_value = (n->pos.n %2 == 0 ? 1 : -1);
        if (n->children != NULL) {
            for (TreeNode **child = n->children ; *child != NULL ; child++) {
                if ((*child)->pos.last == 0) continue;
                if (amaf_map[(*child)->pos.last] == amaf_map_value) {
                    if (disp) {
                        char str[8];
                        str_coord((*child)->pos.last, str);
                        fprintf(stderr, "  AMAF updating %s %d\n", str,score>0);
                    }
                    (*child)->aw += score > 0; // reversed perspective
                    (*child)->av += 1;
                }
            }
        }
        score = -score;
    }
}

float nplayouts, nplayouts_per_second=-1.0;
float start_playouts_sec, stop_playouts_sec;
int   nplayouts_real;
static float best2, bestr, bestwr;
static Point bestmove;

void collect_infos(TreeNode *tree, int n, TreeNode *best, TreeNode *workspace[])
// Collect infos for the time management
{
    TreeNode *second;

    nplayouts_real += n;
    workspace[0] = best; workspace[1] = 0;
    second = best_move(tree, workspace);
    bestwr= winrate(best);
    best2 = bestwr / winrate(second);
    bestr = bestwr - winrate(best_move(best, NULL));
    sprintf(buf, "best2: %.2f bestr: %.3f bestwr: %.3f", best2, bestr, bestwr);
    log_fmt_s('I',buf,NULL);
}

Point tree_search(TreeNode *tree, int n, int owner_map[], int disp)
// Perform MCTS search from a given position for a given #iterations
{
    double s;
    int *amaf_map=michi_calloc(BOARDSIZE, sizeof(int)), i, last; 
    TreeNode *best, *nodes[500];

    // Initialize the root node if necessary
    if (tree->children == NULL) expand(tree);
    memset(owner_map,0,BOARDSIZE*sizeof(int));

    for (i=0 ; i<n ; i++) {
        memset(amaf_map, 0, BOARDSIZE*sizeof(int));
        if (i>0 && i % REPORT_PERIOD == 0) print_tree_summary(tree, i, stderr); 
        last = tree_descend(tree, amaf_map, disp, nodes);
        Position pos = nodes[last]->pos;
        s = mcplayout(&pos, amaf_map, owner_map, disp);
        tree_update(nodes, last, amaf_map, s, disp);
        // Early stop test
        best = best_move(tree, NULL);
        double best_wr = winrate(best);
        if ( (i>n*0.05 && best_wr > FASTPLAY5_THRES)
              || (i>n*0.2 && best_wr > FASTPLAY20_THRES)) {
            best = best_move(tree, NULL);
            sprintf(buf,"tree_search() breaks at %d (%.3f)", i, winrate(best));
            log_fmt_s('I', buf, NULL);
            nplayouts_real += i;
            goto finished;
        }
    }

    best = best_move(tree, NULL);
    collect_infos(tree, n, best, nodes);
    
finished:
    dump_subtree(tree, N_SIMS/50, "", stderr, 1);
    print_tree_summary(tree, i, stderr);

    free(amaf_map);
    if (best->pos.last == PASS_MOVE && best->pos.last2 == PASS_MOVE)
        bestmove = PASS_MOVE;
    else if (((double) best->w / (double) best->v) < RESIGN_THRES) 
        bestmove = RESIGN_MOVE;
    else
        bestmove = best->pos.last;
    return bestmove;
}

Point try_search_again(TreeNode *tree, int n, int owner_map[], int disp)
// Check if the situation is clear enough after the first tree search.
// If not, search for a second period of time
{
    if (tree->pos.n >  10 && tree->pos.n < 100) {
        if (bestwr < 0.4                             // program is behind
            || best2 < 2.0  || fabs(bestr) > 0.02) { // unclear situation
            // think harder hoping we will recover
            log_fmt_s('I', "thinking time extended", NULL);
            tree_search(tree, n, owner_map, 0);
        }
    }
    return bestmove;
}

//-------------------- routines for printing the tree results -----------------

void dump_subtree(TreeNode *node, double thres, char *indent, FILE *f
                                                                , int recurse)
// print this node and all its children with v >= thres.
{
    char str[8], str_winrate[8], str_rave_winrate[8];
    str_coord(node->pos.last, str);
    if (node->v) sprintf(str_winrate, "%.3f", winrate(node)); 
    else         sprintf(str_winrate, "nan");
    if (node->av) sprintf(str_rave_winrate, "%.3f", (double)node->aw/node->av); 
    else          sprintf(str_rave_winrate, "nan");
    fprintf(f,"%s+- %s %5s "
              "(%6d/%-6d, prior %3d/%-3d, rave %6d/%-6d=%5s, urgency %.3f)\n"
           ,indent, str, str_winrate, node->w, node->v, node->pw, node->pv
           ,node->aw, node->av, str_rave_winrate, rave_urgency(node));
    if (recurse) {
        char new_indent[BUFLEN];
        sprintf(new_indent,"%s   ", indent);
        for (TreeNode **child = node->children ; *child != NULL ; child++)
            if ((*child)->v >= thres)
                dump_subtree((*child), thres, new_indent, f, 0);
    }
}

void print_tree_summary(TreeNode *tree, int sims, FILE *f)
{
    char best_seq[32]="", can[128]="", str[8], tmp[32];
    int k;
    TreeNode *best_node[5]={0,0,0,0,0}, *node;

    for (k=0 ; k<5 ; k++) { 
        best_node[k] = best_move(tree, best_node);
        if (best_node[k] != NULL) {
            str_coord(best_node[k]->pos.last,str);
            if (best_node[k]->v)
                sprintf(tmp, " %s(%.3f)", str, winrate(best_node[k]));
            else
                sprintf(tmp, " %s(nan)", str);
            strcat(can, tmp);
        }
    }
    node = tree;
    for (k=0 ; k<5 ; k++) {
        node = best_move(node, NULL);
        if (node == NULL) break;
        str_coord(node->pos.last, str);
        strcat(str, " ");
        strcat(best_seq, str);
    }
    fprintf(f,"[%4d] winrate %.3f | seq %s| can %s\n",sims
                                    ,winrate(best_node[0]), best_seq, can);   
}
