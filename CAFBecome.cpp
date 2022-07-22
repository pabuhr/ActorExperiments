#include <iostream>
using namespace std;
#include <chrono>
using namespace chrono;

#include "caf/actor_ostream.hpp"
#include "caf/caf_main.hpp"
#include "caf/event_based_actor.hpp"
using namespace caf;

int Times = 10'000'000;									// default values
time_point<steady_clock> starttime;

class Become : public event_based_actor {
  public:
	Become( caf::actor_config & cfg ) : event_based_actor( cfg ) {
		this->become( receive() );
	}
	behavior receive() {
		return {
			[=]( int & cnt ) -> void {
				if ( cnt >= Times ) {
					aout(this) << "CAF Become " << Times << " " << (steady_clock::now() - starttime).count() / Times << "ns" << endl;
					this->quit();
					return;
				} // if
				//aout(this) << "receive " << cnt << endl;
				this->send( this, cnt + 1 );
				this->become( this->receive2() );
			}
		};
	}
	behavior receive2() {
		return {
			[=]( int & cnt ) -> void {
				//aout(this) << "receive2 " << cnt << endl;
				this->send( this, cnt + 1 );
				this->become( receive() );
			}
		};
	}
}; // Become

void caf_main( actor_system & sys ) {
	auto starter = sys.spawn<Become>();
	starttime = steady_clock::now();
	caf::anon_send( starter, 0 );
}

int main( int argc, char * argv[] ) {
	switch ( argc ) {
	  case 2:
		Times = stoi( argv[1] );
		if ( Times < 1 ) goto Usage;
	  case 1:											// use defaults
		break;
	  default:
	  Usage:
		cerr << "Usage: " << argv[0] << " [ times (> 0) ]" << endl;
		exit( EXIT_FAILURE );
	} // switch

	caf::exec_main_init_meta_objects<>();
	caf::core::init_global_meta_objects();
	return caf::exec_main<>(caf_main, argc, argv);
} // main

// /usr/bin/time -f "%Uu %Ss %Er %Mkb" a.out

// Local Variables: //
// compile-command: "g++-10 -O3 -Wall -std=c++17 -ICAF/actor-framework/libcaf_core -ICAF/actor-framework/libcaf_core/caf -ICAF/actor-framework/build/libcaf_core -LCAF/actor-framework/build/libcaf_core -LCAF/actor-framework/build/libcaf_io CAFBecome.cpp -lcaf_io -lcaf_core -Wl,-rpath=CAF/actor-framework/build/libcaf_core" //
// End: //
