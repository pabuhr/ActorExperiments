#include <iostream>
using namespace std;
#include <chrono>
using namespace chrono;

#include "caf/actor_ostream.hpp"
#include "caf/caf_main.hpp"
#include "caf/event_based_actor.hpp"
using namespace caf;

int Actors = 40'000, Set = 100, Rounds = 100, Processors = 1, Batch = 1; // ' default values
time_point<steady_clock> starttime;
actor * actors;
int actorCnt = 0;

class Executor : public event_based_actor {
	static int ids;										// unique actor id generator
	actor * gstart;
	int id, rounds, group, recs = 0, sends = 0;

	behavior make_behavior() override {
		return {
			[=]( int & ) -> void {
				if ( recs == rounds ) {
					if ( __atomic_add_fetch( &actorCnt, 1, __ATOMIC_SEQ_CST ) == Actors ) {
						aout(this) << Processors << " " << (steady_clock::now() - starttime).count() / 1'000'000'000.0 << "s" << endl;
					} // if
					this->quit();
					return;
				} // if
				if ( recs % Batch == 0 ) {
					for ( int i = 0; i < Batch; i += 1 ) {
						this->send( gstart[sends % Set], 0 ); // cycle through group
						sends += 1;
					}
				}
				//aout(this) << id << " " << cnt << endl;
				recs += 1;
			}
		};
	}
  public:
	Executor( caf::actor_config & cfg ) : event_based_actor( cfg ) {
		id = ids++;										// unique actor id, and start point for cycle
		gstart = &actors[id / Set * Set];				// remember group-start array-element
		rounds = Set * Rounds;							// send at least one message to each group member
		// aout(this) << "id " << id << " rounds " << rounds << " group " << group << endl;
	}
}; // Executor
int Executor::ids = 0;

void caf_main( actor_system & sys ) {
	for ( int i = 0; i < Actors; i += 1 ) {				// create actors
		actors[i] = sys.spawn<Executor>();
	} // for
	starttime = steady_clock::now();
	for ( int i = 0; i < Actors; i += 1 ) {				// start actors
		caf::anon_send( actors[i], 0 );
	} // for
} // caf_main

int main( int argc, char * argv[] ) {
	switch ( argc ) {
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
			 << ") ]" << endl;
		exit( EXIT_FAILURE );
	} // switch

	//cout << Actors << " " << Set << " " << Rounds << " " << Processors << endl;
	actors = new actor[Actors];

	caf::core::init_global_meta_objects();
	caf::exec_main_init_meta_objects<>();

	caf::actor_system_config cfg;
	cfg.set( "caf.scheduler.max-threads", Processors );
	caf::actor_system system { cfg };

	return caf::exec_main<>(caf_main, argc, argv);
} // main
//CAF_MAIN()

// /usr/bin/time -f "%Uu %Ss %Er %Mkb" a.out

// Local Variables: //
// compile-command: "g++-10 -Wall -O3 -std=c++17 -ICAF/actor-framework/libcaf_core -ICAF/actor-framework/libcaf_core/caf -ICAF/actor-framework/build/libcaf_core -LCAF/actor-framework/build/libcaf_core -LCAF/actor-framework/build/libcaf_io CAFExecutor.cpp -lcaf_io -lcaf_core -Wl,-rpath=CAF/actor-framework/build/libcaf_core" //
// End: //
