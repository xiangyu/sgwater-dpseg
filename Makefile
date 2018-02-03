

# add -g for debugging
# The -std=c++0x flag is needed to compile with Antonios' fixes.
# He used -std=c++11, but this did not work with my compiler.
CFLAGS_BASE = -MMD -O3 -Wall -ffast-math -std=c++0x
ifeq ($(OSTYPE),windows)
CXX = g++
CFLAGS = $(CFLAGS_BASE) -DOS_WINDOWS
else
CXX = g++
CFLAGS = $(CFLAGS_BASE) -DOS_LINUX
endif
CFLAGS_DBG = $(CFLAGS) -g
CFLAGS_OPT = $(CFLAGS) -DNDEBUG
CFLAGS_PRF = $(CFLAGS) -pg -DNDEBUG
CFLAGS_NRM = $(CFLAGS)

CC = $(CXX)
LEX = flex 
LDFLAGS = 

SRC = segment.cc Restaurant.cc BiLexicon.cc State.cc Scoring.cc Utterance.cc Datafile.cc ECArgs.cc
#I think this means any file that has the same prefix
#as one of the source files, and suffix .l,.o,.c
OBJ_DIR_PRF = profile/
OBJ_DIR_OPT = optimized/
OBJ_DIR_DBG = debug/
OBJ_DIR_NRM = norm/
OBJ_OPT = ${SRC:%.cc=$(OBJ_DIR_OPT)%.o}
OBJ_DBG = ${SRC:%.cc=$(OBJ_DIR_DBG)%.o}
OBJ_NRM = ${SRC:%.cc=$(OBJ_DIR_NRM)%.o}
OBJ_PRF = ${SRC:%.cc=$(OBJ_DIR_PRF)%.o}
OBJ_DIR = 

opt: segment

segment: $(OBJ_DIR_OPT) $(OBJ_OPT)
	$(CXX) $(CFLAGS_OPT) $(OBJ_OPT) -o segment $(LDFLAGS)

prf: $(OBJ_DIR_PRF) $(OBJ_PRF) 
	$(CXX) $(CFLAGS_PRF) $(OBJ_PRF) -o segment.prf $(LDFLAGS)

dbg: $(OBJ_DIR_DBG) $(OBJ_DBG) 
	$(CXX) $(CFLAGS_DBG) $(OBJ_DBG) -o segment.dbg $(LDFLAGS)

nrm: $(OBJ_DIR_NRM) $(OBJ_NRM) 
	$(CXX) $(CFLAGS_NRM) $(OBJ_NRM) -o segment.nrm $(LDFLAGS)

all: dbg opt nrm prf 

$(OBJ_DIR_PRF): $(OBJ_DIR)
	-mkdir $(OBJ_DIR_PRF)

$(OBJ_DIR_OPT): $(OBJ_DIR)
	-mkdir $(OBJ_DIR_OPT)

$(OBJ_DIR_DBG): $(OBJ_DIR)
	-mkdir $(OBJ_DIR_DBG)

$(OBJ_DIR_NRM): $(OBJ_DIR)
	-mkdir $(OBJ_DIR_NRM)

$(OBJ_DIR):
	-mkdir $(OBJ_DIR)

$(OBJ_DIR_DBG)%.o: %.cc
	$(CXX)  $(CFLAGS_DBG)  -c $< -o $@

$(OBJ_DIR_NRM)%.o: %.cc
	$(CXX)  $(CFLAGS_NRM)  -c $< -o $@

$(OBJ_DIR_PRF)%.o: %.cc
	$(CXX)  $(CFLAGS_PRF) -c $< -o $@

$(OBJ_DIR_OPT)%.o: %.cc
	$(CXX)  $(CFLAGS_OPT)  -c $< -o $@

.PHONY: clean
clean:
	-rm profile/*.o optimized/*.o debug/*.o norm/*.o
	-rm profile/*.d optimized/*.d debug/*.d norm/*.d
	-rm -f *.d *.o core *.stackdump

.PHONY: real-clean
real-clean: clean
	rm -fr *~ segment segment.exe segment.opt segment.opt.exe

# this command tells GNU make to look for dependencies in *.d files
-include $(patsubst %.l,%.d,$(patsubst %.c,%.d,$(OBJ_DIR_OPT)/$(SRC:%.cc=%.d)))
-include $(patsubst %.l,%.d,$(patsubst %.c,%.d,$(OBJ_DIR_DBG)/$(SRC:%.cc=%.d)))
-include $(patsubst %.l,%.d,$(patsubst %.c,%.d,$(OBJ_DIR_NRM)/$(SRC:%.cc=%.d)))
-include $(patsubst %.l,%.d,$(patsubst %.c,%.d,$(OBJ_DIR_PRF)/$(SRC:%.cc=%.d)))
