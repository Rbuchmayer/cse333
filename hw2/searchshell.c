/*
 * Copyright ©2019 Hal Perkins.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Washington
 * CSE 333 for use solely during Winter Quarter 2019 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

// Feature test macro for strtok_r (c.f., Linux Programming Interface p. 63)
#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include "libhw1/CSE333.h"
#include "memindex.h"
#include "filecrawler.h"

static void Usage(void);

int main(int argc, char **argv) {
  if (argc != 2)
    Usage();

  // Implement searchshell!  We're giving you very few hints
  // on how to do it, so you'll need to figure out an appropriate
  // decomposition into functions as well as implementing the
  // functions.  There are several major tasks you need to build:
  //
  //  - crawl from a directory provided by argv[1] to produce and index
  //  - prompt the user for a query and read the query from stdin, in a loop
  //  - split a query into words (check out strtok_r)
  //  - process a query against the index and print out the results
  //
  // When searchshell detects end-of-file on stdin (cntrl-D from the
  // keyboard), searchshell should free all dynamically allocated
  // memory and any other allocated resources and then exit.

  DocTable dt;
  MemIndex in;
  SearchResult* r;
  char* ptr;
  char* tok;
  char input[512];
  int res;

  printf("Indexing \'%s\'\n", argv[1]);
  res = CrawlFileTree(argv[1], &dt, &in);
  
  if (res == 0) {
    Usage();
  }
  while (1) {
    printf("enter query: \n");
    fgets(input, 512, stdin);
    char** query = (char **) malloc(512 * sizeof(char*));
    int len = 0;
    char* word = input;
    
    // separate words
    tok = strtok_r(word, " ", &ptr);
    while(tok != NULL) {
      query[len] = tok;
      len++;
      word = NULL;
      tok = strtok_r(word, " ", &ptr);
    }

    // change new line to null terminator
    char* newln = strchr(query[len-1], '\n');
    if (*newln == '\n') {
      *newln = '\0';
    }
    LinkedList ll = MIProcessQuery(in, query, len);
    if (ll != NULL) {
      LLIter lit = LLMakeIterator(ll, 0);
      while (LLIteratorHasNext(lit)) {
	LLIteratorGetPayload(lit, (void**) &r);
        printf("  %s (%u)\n", DTLookupDocID(dt, r->docid), r->rank);
	LLIteratorNext(lit);
      }
	LLIteratorGetPayload(lit, (void**) &r);
	printf("  %s (%u)\n", DTLookupDocID(dt, r->docid), r->rank);

      free(query);
      LLIteratorFree(lit);
    }
  }
  FreeDocTable(dt);
  FreeMemIndex(in);
  return EXIT_SUCCESS;
}

static void Usage(void) {
  fprintf(stderr, "Usage: ./searchshell <docroot>\n");
  fprintf(stderr,
          "where <docroot> is an absolute or relative " \
          "path to a directory to build an index under.\n");
  exit(EXIT_FAILURE);
}

