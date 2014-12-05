CC = g++
DEBUG = -g -O3
CFLAGS = -Wall -Wextra -pedantic -Wno-unused-parameter -Wno-unused-variable -std=c++11 -c $(DEBUG) -I/Users/austinma/git/cpyp
LFLAGS = -Wall -Wextra -pedantic -Wno-unused-variable -Wno-unused-parameter -ladept -lboost_serialization $(DEBUG)

all: crf split load_lm score

CRF_OBJECTS = main.o crf.o utils.o ttable.o feature_scorer.o compound_analyzer.o noise_model.o derivation.o
crf: $(CRF_OBJECTS)
	$(CC) $(CRF_OBJECTS) NeuralLM/vocabulary.o $(LFLAGS) -o crf

split: split.o ttable.o utils.o feature_scorer.o compound_analyzer.o derivation.o
	$(CC) split.o ttable.o utils.o feature_scorer.o compound_analyzer.o derivation.o NeuralLM/vocabulary.o $(LFLAGS) -o split

score: score.o ttable.o utils.o feature_scorer.o crf.o derivation.o
	$(CC) score.o ttable.o utils.o feature_scorer.o crf.o derivation.o NeuralLM/vocabulary.o $(LFLAGS) -o score

split.o: split.cc utils.h ttable.h feature_scorer.h compound_analyzer.h derivation.h
	$(CC) $(CFLAGS) split.cc

score.o: score.cc crf.h utils.h feature_scorer.h derivation.h
	$(CC) $(CFLAGS) score.cc

load_lm: load_lm.o utils.o
	$(CC) load_lm.o utils.o NeuralLM/vocabulary.o $(LFLAGS) -o load_lm

load_lm.o: load_lm.cc utils.h NeuralLM/neurallm.h NeuralLM/utils.h
	$(CC) $(CFLAGS) load_lm.cc

noise_model.o: noise_model.cc noise_model.h ttable.h utils.h derivation.h
	$(CC) $(CFLAGS) noise_model.cc

ttable.o: ttable.cc ttable.h utils.h
	$(CC) $(CFLAGS) ttable.cc

utils.o: utils.cc utils.h
	$(CC) $(CFLAGS) utils.cc

derivation.o: derivation.cc derivation.h
	$(CC) $(CFLAGS) derivation.cc

feature_scorer.o: feature_scorer.cc ttable.h feature_scorer.h derivation.h NeuralLM/neurallm.h NeuralLM/context.h
	$(CC) $(CFLAGS) feature_scorer.cc

compound_analyzer.o: compound_analyzer.cc compound_analyzer.h utils.h ttable.h derivation.h
	$(CC) $(CFLAGS) compound_analyzer.cc

main.o: main.cc crf.h utils.h feature_scorer.h compound_analyzer.h noise_model.h derivation.h
	$(CC) $(CFLAGS) main.cc

crf.o: crf.cc crf.h utils.h feature_scorer.h derivation.h
	$(CC) $(CFLAGS) crf.cc

clean:
	rm -f crf
	rm -f ./split
	rm *.o
