#include <iostream>
using namespace std;
#include <chrono>
using namespace chrono;
#include <uActor.h>

int Actors = 40'000, Set = 100, Rounds = 100, Processors = 1, Batch = 1, Qscale = 512; // default values '
struct Msg : public uActor::Message {} msg;
time_point<steady_clock> starttime;

_Actor Executor {
	static int ids;										// unique actor id generator
	Executor * gstart;
	int id, rounds, recs = 0, sends = 0;

	Allocation receive( Message & msg ) {
		Case ( Msg, msg ) {
			if ( recs == rounds ) return Finished;
		  	if ( recs % Batch == 0 ) {
				for ( int i = 0; i < Batch; i += 1 ) {
					gstart[sends % Set] | msg;			// cycle through set
					sends += 1;
				} // for
			} // if
			recs += 1;
		} // Case
		return Nodelete;
	} // Executor::receive
  public:
	Executor() {
		id = ids++;										// unique actor id, and start point for cycle
		gstart = &this[id / Set * Set - id];			// remember group-start array-element
		rounds = Set * Rounds;							// send at least one message to each group member
	} // Executor::Executor
}; // Executor
int Executor::ids = 0;

size_t malloc_unfreed() { return 16621; }				// unfreed storage from locale
//size_t malloc_expansion() { return 2 * 1024 * 1024; }

int main( int argc, char * argv[] ) {
	locale loc( getenv("LANG") );
	cout.imbue( loc );

	switch ( argc ) {
	  case 7:
		if ( strcmp( argv[6], "d" ) != 0 ) {			// default ?
			Qscale = stoi( argv[6] );
			if ( Qscale < 1 ) goto Usage;
		} // if
	  case 6:
		if ( strcmp( argv[5], "d" ) != 0 ) {			// default ?
			Batch = stoi( argv[5] );
			if ( Batch < 1 ) goto Usage;
		} // if
	  case 5:
		if ( strcmp( argv[4], "d" ) != 0 ) {			// default ?
			Processors = stoi( argv[4] );
			if ( Processors < 1 ) goto Usage;
		} // if
	  case 4:
		if ( strcmp( argv[3], "d" ) != 0 ) {			// default ?
			Rounds = stoi( argv[3] );
			if ( Rounds < 1 ) goto Usage;
		} // if
	  case 3:
		if ( strcmp( argv[2], "d" ) != 0 ) {			// default ?
			Set = stoi( argv[2] );
			if ( Set < 1 ) goto Usage;
		} // if
	  case 2:
		if ( strcmp( argv[1], "d" ) != 0 ) {			// default ?
			Actors = stoi( argv[1] );
			if ( Actors < 1 || Actors <= Set || Actors % Set != 0 ) goto Usage;
		} // if
	  case 1:											// use defaults
		break;
	  default:
	  Usage:
		cerr << "Usage: " << argv[0]
			 << " [ actors (> 0 && > set && actors % set == 0 ) | 'd' (default " << Actors
			 << ") ] [ set (> 0) | 'd' (default " << Set
			 << ") ] [ rounds (> 0) | 'd' (default " << Rounds
			 << ") ] [ processors (> 0) | 'd' (default " << Processors
			 << ") ] [ batch (> 0) | 'd' (default " << Batch
			 << ") ] [ queue scale (> 0) | 'd' (default " << Qscale
			 << ") ]" << endl;
		exit( EXIT_FAILURE );
	} // switch

	uExecutor * executor = new uExecutor( Processors, Processors, // too large for task stack
										  Processors == 1 ? 1 : Processors * Qscale, true, 0 );
	uActor::start( executor );							// start actor system
	Executor * actors = new Executor[Actors];			// too many actors for task stack
	starttime = steady_clock::now();
	for ( int i = 0; i < Actors; i += 1 ) actors[i] | msg; // start actors
	uActor::stop();										// wait for all actors to terminate
	cout << "uC++ Executor " << (steady_clock::now() - starttime).count() / 1'000'000'000.0 << "s" << endl; // '
	delete [] actors;
	delete executor;
	// malloc_stats();
} // main

// /usr/bin/time -f "%Uu %Ss %Er %Mkb" a.out

// Local Variables: //
// compile-command: "u++-work -g -Wall -Wextra -O3 -nodebug -DNDEBUG -multi uC++Executor.cc" //
// End: //
