#include <iostream>
#include <fstream>
#include <string>
#include "ECArgs.h"
#include "typedefs.h"
#include "utils.h"
#include "Utterance.h"
#include "Scoring.h"
#include "Datafile.h"
#include "State.h"

using namespace std;
// global variables
Count debug_level;
Float HYPERSAMPLING_RATIO(.1); // the standard deviation for new hyperparm proposals
bool SAMPLE_HYPERPARAMETERS(0);

int main(int argc, char* argv[])
{
  //list the options that require arguments
  ECArgs arguments(argc, argv, string("aAbUmuMiIqvreotwWT"));
  if (arguments.isset('h')) {
    cout << "Usage: segment [input_file]" << endl
	 << "-l (print reference lexicon stats without running EM)" << endl
	 << "-a <alpha0> (total unigram generator weight)" << endl
	 << "-A <alpha1> (total bigram generator weight)" << endl
	 << "-b <prior prob of word boundary>" << endl
	 << "-U <base prob of utt boundary>" << endl
	 << "-H (turn on sampling of hyperparameters)" << endl
	 << "-m [m|m2|se] (P_w: unigram generator model)" << endl
	 << "\t m: uniform monkeys" << endl
	 << "\t m2: monkeys w/ true phoneme distr." << endl
	 << "-u [m|u|y|k|b] (P_u: bigram generator model)" << endl
	 << "\t m: uniform monkeys" << endl
	 << "\t m2: monkeys w/ true phoneme distr." << endl
	 << "\t u: unigrams, sampled tokens" << endl
	 << "\t t: unigrams, sampled tables" << endl
	 << "\t y: unigrams, true types" << endl
	 << "\t k: unigrams, true tokens" << endl
	 << "\t k: bigrams, true types" << endl
	 << "\t (do not use -u if you want only unigram model)" << endl
	 << "\t M <mixing param> (proportion of noise in generator)" << endl
	 << "-i <number of iterations>" << endl
	 << "-I [utt|pho|ran|true|True] (type of init.  Default = random.)" << endl
      	 << "-e [samp|lmax|gmax] (type of evaluation)" << endl
	 << "-o <output_filename_base>" << endl
	 << "-q <Q> (print results summary every Q iters to stdout)" << endl
	 << "-t <N> (print trace statistics every N iters to output.stat)" << endl
	 << "-w <N> (print segmentation every N iters to output.words, with -w0 printing only final segmentation.)" << endl
	 << "-W <N> (print trace statistics and segmentation every N iters starting halfway through final annealing to output.stats and output.words)" << endl
	 << "-T <T> (maintain constant temperature T)" << endl
	 << "-V (prints version number)" << endl
	 << "-v N (verbose level)" << endl
	 << "\t 1: print segmentations" << endl
	 << "\t 2: print final lexicon" << endl
	 << "\t 3: print both" << endl
	 << "-d (debug)" << endl
	 << "-r random seed" << endl;
    exit(0);
  }
  if (arguments.isset('V')) {
    cout << "Version: 1.2" << endl;
    exit(0);
  }
   //input file
  string filename;
  if (arguments.nargs() > 0) {
    filename = arguments.arg(0);
  }
  else {
    filename = "test.in";
  }
  // output files and printing frequencies
  string file_base;
  ofstream words_os;
  if (arguments.isset('o')) 
    file_base = arguments.value('o').c_str();
  ofstream stats_os;
  bool print_stats = false;
  bool print_end = false;
  Count stats_freq = 0;
  if (arguments.isset('t') || arguments.isset('W')) {
    if (file_base == "") {
      cerr << "options t, w, and W require option o" << endl;
      exit(0);
    }
    string file = file_base + ".stats";
    stats_os.open(file.c_str());
    if (arguments.isset('t')) {
      print_stats = true;
      stats_freq = strtol(arguments.value('t').c_str(), NULL, 10);
      if (arguments.isset('W')) {
	cerr << "options t and W are incompatible" << endl;
	exit(0);
      }
    }
    if (arguments.isset('W')) {
      print_end = true;
      stats_freq = strtol(arguments.value('W').c_str(), NULL, 10);
    }
  }
  bool print_words = false;
  Count words_freq = 0;
  if (arguments.isset('w') || arguments.isset('W')) {
    if (file_base == "") {
      cerr << "options t, w, and W require option o" << endl;
      exit(0);
    }
    string file = file_base + ".words";
    words_os.open(file.c_str());
    if (arguments.isset('w')) {
      print_words = true;
      words_freq = strtol(arguments.value('w').c_str(), NULL, 10);
      if (arguments.isset('W')) {
	cerr << "options w and W are incompatible" << endl;
	exit(0);
      }
    }
    if (arguments.isset('W')) 
      words_freq = strtol(arguments.value('W').c_str(), NULL, 10);
  }
  // additional output stuff
  debug_level = 0;
  if (arguments.isset('d')) 
    debug_level = stringToInt(arguments.value('d'));
  int verbose_level = 0;
  if (arguments.isset('v')) {
    verbose_level = stringToInt(arguments.value('v'));
    if (verbose_level < -1 || verbose_level > 3) {
      cout << "verbose level must be -1 to 3.  Setting verbose = 0."  << endl;
      verbose_level = 0;
    }
  }
  // The stl uses lrand48() if possible, otherwise
  // rand(), so we need to seed the correct one.
  //  string seed_type;
  int seed;
  if (arguments.isset('r')) {
    seed = strtol(arguments.value('r').c_str(), NULL,10);
  } 
  else {
    seed = time(0);
  }
  srand(seed);
  try {
    DatafileBase* data = new Datafile(filename);
    Scoring scoring;
    Float alpha = 20;
    Float alpha1 = 0;
    Float p_boundary = .5;
    Float p_utt_boundary = .5;
    int ngram = 1; //word dependency model. 1=unigram. 2=bigram.
    string uni_model("m");
    if (arguments.isset('m'))
      uni_model = arguments.value('m');
    string bi_model("m");
    if (arguments.isset('u')) {
      bi_model = arguments.value('u');
      ngram = 2;
      alpha = 3000;
      alpha1 = 100;
      p_boundary = .2;
    }
    if (arguments.isset('a')) 
      alpha = strtod(arguments.value('a').c_str(), NULL);
    if (arguments.isset('A')) 
      alpha1 = strtod(arguments.value('A').c_str(), NULL);
    if (arguments.isset('b')) 
      p_boundary = strtod(arguments.value('b').c_str(), NULL);
    if (arguments.isset('U')) 
      p_utt_boundary = strtod(arguments.value('U').c_str(), NULL);
    if (arguments.isset('H')) 
      SAMPLE_HYPERPARAMETERS = true;
    string b_init("ran");
    if (arguments.isset('I'))
      b_init = arguments.value('I');
    Float noise = .0001;
    if (arguments.isset('M')) 
      noise = strtod(arguments.value('M').c_str(), NULL);

    cout << "Segmenting " << filename;
    cout << " using " << ngram << "-gram model" << endl;
    cout << "Boundary initialization: " << b_init << endl;
    Utterance::set_init(b_init);
    State::set_models(uni_model,bi_model,ngram,noise);
    State state(data, alpha, p_boundary, alpha1, p_utt_boundary);

    Count iters = 1000;
    if (arguments.isset('i'))
      iters = strtol(arguments.value('i').c_str(), NULL, 10);
    cout << "Sampling " << iters 
	    << " iterations" << endl;
    string eval = "samp";
    if (arguments.isset('e'))
      eval = arguments.value('e');
    if (eval == "lmax")
	cout << "evaluating using local max" << endl;
    else if (eval == "gmax") 
      cout << "evaluating using global max" << endl;
    else if (eval == "samp") 
      cout << "evaluating a sample" << endl;
    else
      error("unknown evaluation method");
    Count print_freq = 0;
    if (arguments.isset('q')) {
      print_freq = strtol(arguments.value('q').c_str(), NULL, 10);
      cerr << "printing status every " << print_freq << " iters" << endl;
    }
    if (verbose_level == 4) {
      cout << state << endl;
    }
    //cout << state.get_lexicon() << endl;
    cout << "random seed = " << seed << endl;
    cout << "alphabet size = " << state.alphabet_size() << endl;
    //annealing stuff
    bool anneal = true;
    Count temp_incr = 10;  //how many increments of temperature to get to T = 1
    if (iters && iters < temp_incr) temp_incr = iters;
    Count iter_incr = iters/temp_incr; //raise temp each iter_incr iters
    Float temp;
    Fs temperatures;
    if (b_init == "True" || arguments.isset('T')) {
      anneal = false;
      temp = 1;
      if (arguments.isset('T'))
	temp = strtod(arguments.value('T').c_str(), NULL);
      cout << "Not doing annealing. T = " << temp << endl;
    }
    else {
      temp = 0;
      for (Count i = 1; i <=temp_incr; i++) {
	temp += 1.0/temp_incr;
	temperatures.push_back(temp);
      }
      if (eval == "gmax") {// use additional iterations to anneal to 0
	iters = 3*iters;
	temp = 1;
	for (Count i = 1; i <=temp_incr*2; i++) {
	  temp *= 1.2;
	  temperatures.push_back(temp);
	}
      }
      cout << "Raising temperature in " << temp_incr << " increments: " 
	   << temperatures << endl;
    }
    if (print_stats)
      state.print_stats_header(stats_os);

    //begin sampling loop
    Count temp_index = 0;
    for (Count i=0; i<iters; i++) {
      if ((i%10) == 0) cerr << ".";
      if (anneal && ((i%iter_incr) == 0)){
	temp = temperatures[temp_index++];
	cerr << "iter " << i << ": temp = " << temp << endl;
      }
      if (print_freq && 
	  ((i==100) || (i % print_freq == 0))) {
	cerr << i << " p_cont=" << state.p_cont() 
	     << " " << state.log_posterior() << endl;
	cout << "Before iteration " << i << 
	  ", with p_cont=" << state.p_cont() <<
	  ", log posterior = " << state.log_posterior() << ":" << endl;
	state.score_utterances(scoring);
	scoring.print_segmentation_summary();
	scoring.print_results();
	scoring.reset();
      }
      if (print_stats && (i % stats_freq == 0)) {
	stats_os << i << ",\t";
	state.print_stats(stats_os);
      }
      if (print_words && words_freq && (i % words_freq == 0)) {
	words_os << state << endl;
      }
      if (print_end && (i%words_freq == 0) &&
	  (!anneal ||
	   (temp == temperatures.back() && i%iter_incr >= iter_incr/2))) {
	stats_os << i << ",\t";
	state.print_stats(stats_os);
	words_os << state << endl;
      }
      state.sample(temp);
    } //end of sampling loop

    if (eval == "lmax")
	state.sample(10000); //like doing a local max instead of sample.
    //cout << state.get_lexicon() << endl;

    //print final stats
    if (print_stats || print_end) {
      stats_os << iters << ",\t";
      state.print_stats(stats_os);
    }
    if (print_words || print_end) {
      words_os << state;
    }

    state.score_utterances(scoring);
    if (verbose_level == 1 || verbose_level == 3) {
      cout << "State: " << endl;
      cout << state;
      // don't remove this line without making sure it isn't
      // assumed by scripts to be in the output.
      cout << "_nstrings=xxx" << endl << endl;
    }
    if (arguments.isset('l')) {
      scoring.print_reference_lexicon();
    }
    else {
    cout << endl;
    cerr << iters << " iterations" << endl;
    if (verbose_level == 2 || verbose_level == 3) {
      scoring.print_segmented_lexicon();
      cout << endl;
    }
    if (verbose_level > -1) 
      scoring.print_final_results();
    cout << "p_cont=" << state.p_cont() 
	 << ", log prob = " << state.log_posterior() << endl;
    }
//     cout << "Generating utterances:" << endl;
//     for (Count j = 0; j < 1000; j++) {
//     state.generate();
//     }
    delete data;
  }
  catch (FileError& e) {
    cout << "Error: couldn't open input file " << filename << endl;
  }
  catch (UtteranceLength& e) {
    e.print();
  }
}

