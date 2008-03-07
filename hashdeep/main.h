
/* $Id: main.h,v 1.14 2007/12/28 01:49:36 jessekornblum Exp $ */

#ifndef __HASHDEEP_H
#define __HASHDEEP_H

#include "common.h"


/* HOW TO ADD A NEW HASHING ALGORITHM

  * Add a value for the algorithm to the hashname_t enumeration

  * Add the functions to compute the hashes. There should be three functions,
    an initialization route, an update routine, and a finalize routine.
    The convention, for an algorithm "foo", is 
    foo_init, foo_update, and foo_final.

   * Add a call to insert the algorithm in setup_hashing_algorithms

   * Update parse_algorithm_name in main.c to handle your algorithm. 

   * Verify that ALGORITHM_NAME_LENGTH doesn't need to be increased.

   * Update the usage function and man page to include the function

*/

typedef enum
  { 
    alg_md5=0, 
    alg_sha1, 
    alg_sha256, 
    alg_tiger,
    alg_whirlpool, 

    /* alg_unknown must always be last in this list */
  alg_unknown 
  } hashname_t;  

#define NUM_ALGORITHMS  alg_unknown

/* When parsing algorithm names supplied by the user, they must be 
   fewer than ALGORITHM_NAME_LENGTH characters. */
#define ALGORITHM_NAME_LENGTH 15 

/* Return codes */
typedef enum 
  {
    status_ok = 0,

    /* Matching hashes */
    status_match,
    status_partial_match,        /* One or more hashes match, but not all */
    status_file_size_mismatch,   /* Implies all hashes match */
    status_file_name_mismatch,   /* Implies all hashes and file size match */   
    status_no_match,             /* Implies none of the hashes match */

    /* Loading sets of hashes */
    status_unknown_filetype,
    status_contains_bad_hashes,
    status_contains_no_hashes,
    status_file_error,

    /* General errors */
    status_out_of_memory,
    status_invalid_hash,  
    status_unknown_error,
    status_omg_ponies

  } status_t;


/* Types of files that contain known hashes */
typedef enum 
  {
    file_plain,
    file_bsd,
    file_hashkeeper,
    file_nsrl_15,
    file_nsrl_20,
    file_encase3,
    file_encase4,
    file_ilook,

    /* Files generated by md5deep with the ten digit filesize at the start 
       of each line */
    file_md5deep_size,

    file_hashdeep_10
    
    /* RBF - file_unknown is already used in common.h now */
//    file_unknown
    
  } filetype_t; 

/* These describe the version of the file format being used, not
   the version of the program. */
#define HASHDEEP_PREFIX  "%%%% "
#define HASHDEEP_VERSION "1.0"


typedef struct _file_data_t
{
  char      * hash[NUM_ALGORITHMS];
  uint64_t    file_size;
  TCHAR      * file_name;
  uint64_t    used;
  /* ID number in set of known hashes. Unique per execution */
  uint64_t    id;  
  struct _file_data_t * next;
} file_data_t;


typedef struct _hashtable_entry_t
{
  status_t                     status; 
  file_data_t                * data;
  struct _hashtable_entry_t  * next;   
} hashtable_entry_t;

/* HASH_TABLE_SIZE must be at least 16 to the power of HASH_TABLE_SIG_FIGS */
#define HASH_TABLE_SIG_FIGS   5
#define HASH_TABLE_SIZE       1048577   

typedef struct _hash_table_t {
  hashtable_entry_t * member[HASH_TABLE_SIZE];
} hashtable_t;


typedef struct _state state;

typedef struct _algorithm_t
{
  char        * name;
  uint16_t      byte_length;
  void        * hash_context; 

  int ( *f_init)(void *);
  int ( *f_update)(void *, unsigned char *, uint64_t );
  int ( *f_finalize)(void *, unsigned char *);

  hashtable_t     * known;

  /* We always store the result in a file_data structure */
  //  unsigned char   * result;

  unsigned char   * hash_sum;
  int             inuse;
  uint64_t        howmany;
} algorithm_t;


/* Primary modes of operation  */
typedef enum  
{ 
  primary_compute, 
  primary_match, 
  primary_match_neg, 
  primary_audit
} primary_t; 


struct _state {

  /* Basic Program State */
  primary_t       primary_function;
  uint64_t        mode;

  /* Command line arguments */
  TCHAR        ** argv;
  int             argc;
  TCHAR         * cwd;

  time_t          start_time, last_time;

  /* The file currently being hashed */
  file_data_t   * current_file;
  int             is_stdin;
  FILE          * handle;
  unsigned char * buffer;
  uint64_t        total_megs;

  /* We don't want to use s->total_bytes, but it's required for hash.c */
  uint64_t        total_bytes;

  uint64_t        bytes_read;

  /* We don't want to use s->full_name, but it's required for hash.c */
  TCHAR         * full_name;
  
  TCHAR         * short_name;
  TCHAR         * msg;

  /* The set of known hashes */
  int             hashes_loaded;
  uint64_t        next_known_id;
  algorithm_t   * hashes[NUM_ALGORITHMS];
  file_data_t   * known, * last;
  uint64_t        hash_round;

  int             banner_displayed;

  /* Size of blocks used in normal hashing */
  uint64_t        block_size;

  /* Size of blocks used in piecewise hashing */
  uint64_t        piecewise_size;
  
  // RBF - Should audit variables be moved out of the state?
  /* For audit mode, the number of each type of file */
  uint64_t        match_exact, match_expect,
    match_partial, match_moved, match_unused, match_unknown, match_total;
};

#include "ui.h"

#include "md5.h"
#include "sha1.h"
#include "sha256.h"
#include "whirlpool.h"
#include "tiger.h"

/* HASH TABLE */
void hashtable_init(hashtable_t *t);
status_t hashtable_add(state *s, hashname_t alg, file_data_t *f);
hashtable_entry_t * hashtable_contains(state *s, hashname_t alg);
void hashtable_destroy(hashtable_entry_t *e);

/* MULTIHASHING */
void multihash_initialize(state *s);
void multihash_update(state *s, unsigned char *buf, uint64_t len);
void multihash_finalize(state *s);


/* MATCHING MODES */
status_t load_match_file(state *s, char *fn);
status_t is_known_file(state *s);


int display_hash_simple(state *s);

/* AUDIT MODE */

void setup_audit(state *s);
int audit_status(state *s);
int display_audit_results(state *s);
int audit_update(state *s);

/* HASHING CODE */

int hash_file(state *s, TCHAR *file_name);
int hash_stdin(state *s);


/* HELPER FUNCTIONS */
void generate_filename(state *s, TCHAR *fn, TCHAR *cwd, TCHAR *input);
void sanity_check(state *s, int condition, char *msg);

/* ----------------------------------------------------------------
   INPUT FILE PROCESSING
   ---------------------------------------------------------------- */
int process_normal(state *s, TCHAR *fn);

#endif /* ifndef __HASHDEEP_H */