#include "algorithms_util.h"

FinalResults<long double> hill_climbing(unsigned num_min_iterations, unsigned num_iterations, std::vector<Deck*> your_decks , Process& proc, Requirement & requirement
#ifndef NQUEST
		, Quest & quest
#endif
);

FinalResults<long double> simulated_annealing(unsigned num_min_iterations, unsigned num_iterations, std::vector<Deck*> your_decks, Process& proc, Requirement & requirement
#ifndef NQUEST
		, Quest & quest
#endif
);

FinalResults<long double> genetic_algorithm(unsigned num_min_iterations, unsigned num_iterations, std::vector<Deck*> your_decks, Process& proc, Requirement & requirement
#ifndef NQUEST
		, Quest & quest
#endif
);

FinalResults<long double> forts_climbing(unsigned num_iterations, Process& proc);

FinalResults<long double> beam_climb(unsigned num_min_iterations, unsigned num_iterations, std::vector<Deck*> your_decks, Process& proc, Requirement & requirement
#ifndef NQUEST
		, Quest & quest
#endif
);
