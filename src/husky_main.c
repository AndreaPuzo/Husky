#include "husky.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

void usage (char * progname, int exit_code) ;
void version (void) ;
void help (char * progname, char * pagename, int exit_code) ;

int main (int argc, char ** argv)
{
  if (0 == argc)
    abort() ;

  if (1 == argc)
    usage(argv[0], EXIT_SUCCESS) ;

  husky_t husky ;

  husky.err_code = HUSKY_SUCCESS ;
  husky.state    = HUSKY_STATE_HALTED ;
  husky.ip       = 0 ;
  husky.fp       = 0 ;
  husky.sp       = 0 ;
  husky.ptr      = 0 ;
  husky.mem_size = HUSKY_MEMORY_SIZE_DEFAULT ;
  husky.mem_data = NULL ;
  husky.err_func = NULL ;
  husky.verbose  = 0 ;

  int i ;
  char * image_name = NULL ;

  for (i = 1 ; i < argc ; ++i) {
    if (0 == strcmp(argv[i], "-v") || 0 == strcmp(argv[i], "--version"))
      version() ;

    if (0 == strcmp(argv[i], "-h") || 0 == strcmp(argv[i], "--help"))
      help(argv[0], NULL, EXIT_SUCCESS) ;

    if (0 == strncmp(argv[i], "--help=", 7))
      help(argv[0], argv[i] + 7, EXIT_SUCCESS) ;

    if (0 == strcmp(argv[i], "--verbose")) {
      husky.verbose = 1 ;
    } else if (0 == strcmp(argv[i], "-m") || 0 == strcmp(argv[i], "--memory")) {
      if (argc == i + 1)
        break ;
      
      ++i ;

      char * endptr = NULL ;
      husky.mem_size = strtoull(argv[i], &endptr, 0) ;

      if (NULL != endptr) {
        if (0 == strcmp(endptr, "_KiB")) {
          husky.mem_size <<= 10 ;
        } else if (0 == strcmp(endptr, "_MiB")) {
          husky.mem_size <<= 20 ;
        } else if (0 == strcmp(endptr, "_GiB")) {
          husky.mem_size <<= 30 ;
        }
      }
    } else {
      image_name = argv[i] ;
      break ;
    }
  }

  if (NULL == image_name) {
    fprintf(stderr, "Error: Ivalid image filename.\n") ;
    exit(EXIT_FAILURE) ;
  }

  if (0 == husky.mem_size) {
    fprintf(stderr, "Error: Ivalid memory size.\n") ;
    exit(EXIT_FAILURE) ;
  }

  u64_t argv_size = 0 ;
  int j = i ;

  for (; i < argc ; ++i) {
    argv_size += strlen(argv[i]) + 1 ;
  }

  u64_t mem_size = husky.mem_size ;
  husky.mem_size += argv_size ;
  husky.mem_data = (u8_t *)calloc(husky.mem_size, sizeof(u8_t)) ;

  if (NULL == husky.mem_data) {
    fprintf(stderr, "Error: Cannot allocate the memory.\n") ;
    exit(EXIT_FAILURE) ;
  }

  if (0 != husky.verbose) {
    fprintf(stderr, "Loading `%s`...\n", image_name) ;
  }

  if (HUSKY_SUCCESS != husky_image_load(&husky, image_name)) {
    free(husky.mem_data) ;
    exit(EXIT_FAILURE) ;
  }

  if (0 != husky.verbose) {
    fprintf(stderr, "Loading %d arguments...\n", argc - j) ;
  }

  u64_t argv_addr = husky.mem_size ;

  for (i = argc - 1 ; j <= i ; --i) {
    u64_t size = strlen(argv[i]) + 1 ;
    
    if (argv_addr < size) {
      fprintf(stderr, "Error: Not enough memory to store the arguments\n") ;
      free(husky.mem_data) ;
      exit(EXIT_FAILURE) ;
    }
    
    argv_addr -= size ;

    if (0 != husky.verbose) {
      fprintf(stderr, "Writing `%s` at 0x%012" PRIX64 "...\n", argv[i], argv_addr) ;
      fprintf(stderr, "  * Argument Address = 0x%012" PRIX64 "\n", argv_addr) ;
      fprintf(stderr, "  * Argument size    = %lu\n", size) ;
      fprintf(stderr, "  * Memory Size      = %lu\n", husky.mem_size) ;
    }

    if (HUSKY_SUCCESS != husky_memory_write(&husky, argv_addr, size, argv[i])) {
      fprintf(stderr, "Error: %s\n", husky_error_as_string(husky.err_code)) ;
      free(husky.mem_data) ;
      exit(EXIT_FAILURE) ;
    }
  }

  argv_addr = husky.mem_size ;

  husky_object_t object ;
  object.p = NULL ;

  if (HUSKY_SUCCESS != husky_stack_push(&husky, object)) {
    free(husky.mem_data) ;
    exit(EXIT_FAILURE) ;
  }

  for (i = argc - 1 ; j <= i ; --i) {
    u64_t size = strlen(argv[i]) + 1 ;
    
    argv_addr -= size ;

    if (0 != husky.verbose) {
      fprintf(stderr, "Pushing `%s`...\n", argv[i]) ;
    }
    
    object.u = argv_addr ;

    if (HUSKY_SUCCESS != husky_stack_push(&husky, object)) {
      fprintf(stderr, "Error: %s\n", husky_error_as_string(husky.err_code)) ;
      free(husky.mem_data) ;
      exit(EXIT_FAILURE) ;
    }
  }

  object.u = argc - j ;

  if (HUSKY_SUCCESS != husky_stack_push(&husky, object)) {
    fprintf(stderr, "Error: %s\n", husky_error_as_string(husky.err_code)) ;
    free(husky.mem_data) ;
    exit(EXIT_FAILURE) ;
  }

  if (0 != husky.verbose) {
    fprintf(stderr, "Running `%s` at 0x%012" PRIX64 "...\n", image_name, husky.ip) ;
  }

  while (HUSKY_STATE_HALTED != husky_state_get(&husky)) {
    husky_clock(&husky) ;

    if (HUSKY_SUCCESS != husky_error_get(&husky)) {
      fprintf(stderr, "Error: %s.\n", husky_error_as_string(husky.err_code)) ;
    }
  }

  if (NULL != husky.mem_data) {
    free(husky.mem_data) ;
  }

  exit(EXIT_SUCCESS) ;
}

void usage (char * progname, int exit_code)
{
  fprintf(stderr, "Usage: %s [options...] IMAGE [arguments...]\n", progname) ;
  exit(exit_code) ;
}

void version (void)
{
  fprintf(
    stderr                                ,
    "Husky %d.%d.%d under MIT license.\n"
    "Copyright (C) 2023 - Andrea Puzo.\n"
    "Built on %s at %s.\n"                ,
    __HUSKY_VERSION_MAJOR                 ,
    __HUSKY_VERSION_MINOR                 ,
    __HUSKY_VERSION_PATCH                 ,
    __DATE__                              ,
    __TIME__
  ) ;
  exit(EXIT_SUCCESS) ;
}

void help (char * progname, char * pagename, int exit_code)
{
  if (NULL == pagename || 0 == *pagename) {
    fprintf(stderr, "Usage: %s [options...] IMAGE [arguments...]\n", progname) ;

    fprintf(
      stderr ,
      "Options:\n"
      "  -h , --help        --- Print this help page.\n"
      "  -v , --version     --- Print the version.\n"
      "  -m , --memory SIZE --- Set the amount of memory.\n"
      "       --verbose     --- Print misc information.\n"
      "Notes:\n"
      "  * SIZE is an unsigned integer. You can also append\n"
      "         `_KiB`, `_MiB` or `_GiB`.\n"
    ) ;
  } else {
    exit_code = EXIT_FAILURE ;
  }

  exit(exit_code) ;
}
