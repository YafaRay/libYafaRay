#pragma once

#ifndef YAFARAY_WINUNISTD_H
#define YAFARAY_WINUNISTD_H


int getopt__(int argc, char **argv, char *opstring) ;
/* static (global) variables that are specified as exported by getopt() */
extern char *optarg__;    /* pointer to the start of the option argument  */
extern int   optind__;       /* number of the next argv[] to be evaluated    */
extern int   opterr__;       /* non-zero if a question mark should be returned  */


#endif