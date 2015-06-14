// params.c -- Variable parameters used by the michi program
#include "michi.h"
static char buf[2048];
// ----------------- Initial Values for playouts parameters -------------------
int   N_SIMS             = 2000;
int   RAVE_EQUIV         = 3500;
int   EXPAND_VISITS      = 8;
int   PRIOR_EVEN         = 10;   // should be even number; 0.5 prior 
int   PRIOR_SELFATARI    = 10;   // negative prior
int   PRIOR_CAPTURE_ONE  = 15;
int   PRIOR_CAPTURE_MANY = 30;
int   PRIOR_PAT3         = 10;
int   PRIOR_LARGEPATTERN = 100;  // most moves have relatively small probability
int   PRIOR_CFG[]        =     {24, 22, 8};
int   LEN_PRIOR_CFG      = (sizeof(PRIOR_CFG)/sizeof(int));
int   PRIOR_EMPTYAREA    = 10;
double PROB_HEURISTIC_CAPTURE = 0.9;   // probability of heuristic suggestions
double PROB_HEURISTIC_PAT3    = 0.95;  // being taken in playout
double PROB_SSAREJECT = 0.9;// prob of rejecting suggested self-atari in playout
double PROB_RSAREJECT = 0.5;// prob of rejecting random self-atari in playout
                           // this is lower than above to allow nakade
// ----------------- Initial Values for general parameters --------------------
int   play_until_the_end = 0;
int   random_seed        = 1;
int   REPORT_PERIOD      = 200;
int   verbosity          = 2;
double FASTPLAY20_THRES= 0.8;//if at 20% playouts winrate is >this, stop reading
double FASTPLAY5_THRES = 0.95;//if at 5% playouts winrate is >this, stop reading
double RESIGN_THRES     = 0.2;

unsigned int idum        = 1;
//==================================== Code ===================================

#define PRINT_KEY_VALUE(p, fmt) sprintf(key_value, "%30s " #fmt "\n", #p, p); \
                                strcat(buf, key_value);
#define READ_VALUE(p, fmt) {                                                  \
    char *value = strtok(NULL," \t\n");                                       \
    if (value == NULL)                                                        \
        ret = "Error! No value";                                              \
    else {                                                                    \
        sscanf(value, #fmt, &p);                                              \
        ret = "";                                                             \
    }                                                                         \
}

char* param_playout() 
// Implement gtp command that list or modify parameters of the playout policy
{
    char *param = strtok(NULL," \t\n"), *ret=buf, key_value[80];
    char *known_params = "\nPROB_HEURISTIC_CAPTURE\nPROB_HEURISTIC_PAT3\n"
        "PROB_REJECT_SELF_ATARI_RANDOM\nPROB_REJECT_SELF_ATARI_SUGGESTED\n";

    if (param == NULL) {    // List current values
        buf[0] = 0;
        PRINT_KEY_VALUE(PROB_HEURISTIC_CAPTURE, %.2f);
        PRINT_KEY_VALUE(PROB_HEURISTIC_PAT3, %.2f);
        PRINT_KEY_VALUE(PROB_SSAREJECT, %.2f);
        PRINT_KEY_VALUE(PROB_RSAREJECT, %.2f);
    }
    else if (strcmp(param, "help") == 0)
        ret = known_params;
    else if (strcmp(param, "list") == 0)
        ret = known_params;
    else if (strcmp(param, "PROB_HEURISTIC_CAPTURE") == 0)
        READ_VALUE(PROB_HEURISTIC_CAPTURE, %lf)
    else if (strcmp(param, "PROB_HEURISTIC_PAT3") == 0)
        READ_VALUE(PROB_HEURISTIC_PAT3, %lf)
    else if (strcmp(param, "PROB_SSAREJECT") == 0)
        READ_VALUE(PROB_SSAREJECT, %lf)
    else if (strcmp(param, "PROB_RSAREJECT") == 0)
        READ_VALUE(PROB_RSAREJECT, %lf)
    return ret;
}

char* param_tree(void) 
// Implement gtp command that list or modify parameters of the tree policy
{
    char *param = strtok(NULL," \t\n"), *ret=buf, key_value[80];
    char *known_params = "\nN_SIMS\nRAVE_EQUIV\nEXPAND_VISITS\n"
        "PRIOR_EVEN\nPRIOR_SELFATARI\nPRIOR_CAPTURE_ONE\nPRIOR_CAPTURE_MANY\n"
        "PRIOR_PAT3\nPRIOR_LARGEPATTERN\nPRIOR_CFG[0]\nPRIOR_CFG[1]\n"
        "PRIOR_CFG[2]\nPRIOR_EMPTYAREA\n";

    if (param == NULL) {    // List current values
        buf[0] = 0;
        PRINT_KEY_VALUE(N_SIMS, %d);
        PRINT_KEY_VALUE(RAVE_EQUIV, %d);
        PRINT_KEY_VALUE(EXPAND_VISITS, %d);
        PRINT_KEY_VALUE(PRIOR_EVEN, %d);
        PRINT_KEY_VALUE(PRIOR_SELFATARI, %d);
        PRINT_KEY_VALUE(PRIOR_CAPTURE_ONE, %d);
        PRINT_KEY_VALUE(PRIOR_CAPTURE_MANY, %d);
        PRINT_KEY_VALUE(PRIOR_PAT3, %d);
        PRINT_KEY_VALUE(PRIOR_LARGEPATTERN, %d);
        PRINT_KEY_VALUE(PRIOR_CFG[0], %d);
        PRINT_KEY_VALUE(PRIOR_CFG[1], %d);
        PRINT_KEY_VALUE(PRIOR_CFG[2], %d);
        PRINT_KEY_VALUE(PRIOR_EMPTYAREA, %d);
    }
    else if (strcmp(param, "help") == 0)
        ret = known_params;
    else if (strcmp(param, "list") == 0)
        ret = known_params;
    else if (strcmp(param, "N_SIMS") == 0)
        READ_VALUE(N_SIMS, %d)
    else if (strcmp(param, "RAVE_EQUIV") == 0)
        READ_VALUE(RAVE_EQUIV, %d)
    else if (strcmp(param, "EXPAND_VISITS") == 0)
        READ_VALUE(EXPAND_VISITS, %d)
    else if (strcmp(param, "PRIOR_EVEN") == 0)
        READ_VALUE(PRIOR_EVEN, %d)
    else if (strcmp(param, "PRIOR_SEdATARI") == 0)
        READ_VALUE(PRIOR_SELFATARI, %d)
    else if (strcmp(param, "PRIOR_CAPTURE_ONE") == 0)
        READ_VALUE(PRIOR_CAPTURE_ONE, %d)
    else if (strcmp(param, "PRIOR_CAPTURE_MANY") == 0)
        READ_VALUE(PRIOR_CAPTURE_MANY, %d)
    else if (strcmp(param, "PRIOR_PAT3") == 0)
        READ_VALUE(PRIOR_PAT3, %d)
    else if (strcmp(param, "PRIOR_LARGEPATTERN") == 0)
        READ_VALUE(PRIOR_LARGEPATTERN, %d)
    else if (strcmp(param, "PRIOR_CFG[0]") == 0)
        READ_VALUE(PRIOR_CFG[0], %d)
    else if (strcmp(param, "PRIOR_CFG[1]") == 0)
        READ_VALUE(PRIOR_CFG[1], %d)
    else if (strcmp(param, "PRIOR_CFG[2]") == 0)
        READ_VALUE(PRIOR_CFG[2], %d)
    else if (strcmp(param, "PRIOR_EMPTYAREA") == 0)
        READ_VALUE(PRIOR_EMPTYAREA, %d)
    return ret;
}

unsigned int true_random_seed(void)
// return a true random seed (which depends on the time)
{
    unsigned int r1, r2, sec, day;
    time_t tm=time(NULL);
    struct tm *tcal=localtime(&tm);
    sec  = tcal->tm_sec + 60*(tcal->tm_min + 60*tcal->tm_hour);
    // day is a coarse (but sufficient for the current purpose) approximation 
    day = tcal->tm_mday + 31*(tcal->tm_mon + 12*tcal->tm_year);
    // Park & Miller random generator (same as qdrandom())
    r1 =  (1664525*sec) + 1013904223;
    r2 = (1664525*day) + 1013904223;
    return (r1^r2);
}

char* param_general(void) 
// Implement gtp command that list or modify general parameters
{
    char *param = strtok(NULL," \t\n"), *ret=buf, key_value[80];
    char *known_params = "\nverbosity\nREPORT_PERIOD\nRESIGN_THRES\n"
                         "FASTPLAY20_THRES\nFASTPLAY5_THRES\n";

    if (param == NULL) {    // List current values
        buf[0] = 0;
        PRINT_KEY_VALUE(play_until_the_end, %d);
        PRINT_KEY_VALUE(random_seed, %d);
        PRINT_KEY_VALUE(REPORT_PERIOD, %d);
        PRINT_KEY_VALUE(verbosity, %d);
        PRINT_KEY_VALUE(RESIGN_THRES, %.2f);
        PRINT_KEY_VALUE(FASTPLAY20_THRES, %.2f);
        PRINT_KEY_VALUE(FASTPLAY5_THRES, %.2f);
    }
    else if (strcmp(param, "help") == 0)
        ret = known_params;
    else if (strcmp(param, "list") == 0)
        ret = known_params;
    else if (strcmp(param, "play_until_the_end") == 0)
        READ_VALUE(play_until_the_end, %d)
    else if (strcmp(param, "random_seed") == 0) {
        READ_VALUE(random_seed, %d)
        if (random_seed == 0)
            idum = true_random_seed();
        else
            idum = random_seed;
    }
    else if (strcmp(param, "REPORT_PERIOD") == 0)
        READ_VALUE(REPORT_PERIOD, %d)
    else if (strcmp(param, "verbosity") == 0)
        READ_VALUE(verbosity, %d)
    else if (strcmp(param, "RESIGN_THRES") == 0)
        READ_VALUE(RESIGN_THRES, %lf)
    else if (strcmp(param, "FASTPLAY20_THRES") == 0)
        READ_VALUE(FASTPLAY20_THRES, %lf)
    else if (strcmp(param, "FASTPLAY5_THRES") == 0)
        READ_VALUE(FASTPLAY5_THRES, %lf)
    return ret;
}
