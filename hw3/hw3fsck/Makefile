# Copyright ©2019 Hal Perkins.  All rights reserved.  Permission is
# hereby granted to students registered for University of Washington
# CSE 333 for use solely during Winter Quarter 2019 for purposes of
# the course.  No other use, copying, distribution, or modification
# is permitted without prior written consent. Copyrights for
# third-party components of this work must be honored.  Instructors
# interested in reusing these course materials should contact the
# author.

AR = ar
ARFLAGS = rcs
CC = gcc
CXX = g++
CFLAGS = -g -Wall -I.
LDFLAGS = -L../libhw2 -lhw2 -L../libhw1/ -lhw1
HEADERS = FileIndexCkecker.h IndexTableChecker.h DocIDTableChecker.h \
          DocTableChecker.h HashTableChecker.h fileindexchecker.h
OBJS = hw3fsck.o fileindexutil.o FileIndexChecker.o IndexTableChecker.o \
       DocIDTableChecker.o DocTableChecker.o HashTableChecker.o

all: hw3fsck

hw3fsck: $(OBJS)
	$(CXX) $(CFLAGS) -o hw3fsck $(OBJS) $(LDFLAGS)

%.o: %.cc $(HEADERS)
	$(CXX) $(CFLAGS) -c $<

clean: FORCE
	/bin/rm -f *.o *~ hw3fsck

FORCE:
