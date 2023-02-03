#include <iostream>
using namespace std;
#include <chrono>
using namespace chrono;
#include <uActor.h>

int Times = 100'000'000;								// default values
time_point<steady_clock> starttime;

struct Msg : public uActor::Message { int cnt; Msg( int cnt ) : cnt( cnt ) {} } msg{ 0 }; // default Nodelete

_Actor Become {
	Allocation receive( Message & msg ) {				// state 1
		Case ( Msg, msg ) {
			if ( msg_d->cnt >= Times ) {
				cout << Times << ' ' << (steady_clock::now() - starttime).count() / Times << "ns" << endl;
				return Finished;
			} // if
			//cout << "receive " << msg_d->cnt << endl;
			msg_d->cnt += 1;
			*this | msg;								// message send to self
			become( &Become::receive2 );				// change state
		} // Case
		return Nodelete;
	} // Become::receive

	Allocation receive2( Message & msg ) {				// state 2
		Case ( Msg, msg ) {
			//cout << "receive2 " << msg_d->cnt << endl;
			msg_d->cnt += 1;
			*this | msg;								// message send to self
			become( &Become::receive );					// change state
		} // Case
		return Nodelete;
	} // Become::receive2
}; // Become

int main( int argc, char * argv[] ) {
	switch ( argc ) {
	  case 2:
		if ( strcmp( argv[1], "d" ) != 0 ) { Times = stoi( argv[1] ); }
		if ( Times < 1 ) goto Usage;
	  case 1:											// use defaults
		break;
	  default:
	  Usage:
		cerr << "Usage: " << argv[0] << " [ times (> 0) ]" << endl;
		exit( EXIT_FAILURE );
	} // switch

	uActor::start();									// start actor system
	starttime = steady_clock::now();
	Become become;
	become | msg;
	uActor::stop();										// wait for all actors to terminate
} // main

// /usr/bin/time -f "%Uu %Ss %Er %Mkb" a.out

// Local Variables: //
// compile-command: "u++-work -g -Wall -Wextra -O3 -nodebug -DNDEBUG -multi uC++Become.cc" //
// End: //
