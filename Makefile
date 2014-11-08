CC = g++
DEBUG = -g -O3
CFLAGS = -Wall -std=c++11 -c $(DEBUG) -I/Users/austinma/git/cpyp
LFLAGS = -Wall -ladept $(DEBUG)

all: crf split

CRF_OBJECTS = main.o conll.o crf.o utils.o ttable.o feature_scorer.o compound_analyzer.o noise_model.o
crf: $(CRF_OBJECTS)
	$(CC) $(LFLAGS) $(CRF_OBJECTS) -o crf

split: split.o ttable.o utils.o feature_scorer.o compound_analyzer.o
	$(CC) $(LFLAGS) split.o ttable.o utils.o feature_scorer.o compound_analyzer.o -o split

split.o: split.cc utils.h ttable.h feature_scorer.h compound_analyzer.h
	$(CC) $(CFLAGS) split.cc

noise_model.o: noise_model.cc noise_model.h ttable.h utils.h
	$(CC) $(CFLAGS) noise_model.cc

ttable.o: ttable.cc ttable.h utils.h
	$(CC) $(CFLAGS) ttable.cc

utils.o: utils.cc utils.h
	$(CC) $(CFLAGS) utils.cc

feature_scorer.o: feature_scorer.cc ttable.h feature_scorer.h
	$(CC) $(CFLAGS) feature_scorer.cc

compound_analyzer.o: compound_analyzer.cc compound_analyzer.h utils.h ttable.h
	$(CC) $(CFLAGS) compound_analyzer.cc

main.o: main.cc crf.h utils.h feature_scorer.h compound_analyzer.h noise_model.h
	$(CC) $(CFLAGS) main.cc

crf.o: crf.cc crf.h utils.h feature_scorer.h
	$(CC) $(CFLAGS) crf.cc

conll.o:
	$(CC) $(CFLAGS) conll.cc

clean:
	rm -f crf
	rm -f ./split
	rm *.o
