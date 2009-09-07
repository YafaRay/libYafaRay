#ifndef Yafray_win32_unistd_h
#define Yafray_win32_unistd_h


int getopt(int argc, char *argv[], char *opstring) ;
/* static (global) variables that are specified as exported by getopt() */ 
extern char *optarg;    /* pointer to the start of the option argument  */ 
extern int   optind;       /* number of the next argv[] to be evaluated    */ 
extern int   opterr;       /* non-zero if a question mark should be returned  */


#endif