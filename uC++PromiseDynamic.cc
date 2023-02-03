#include <iostream>
using namespace std;
#include <chrono>
using namespace chrono;
#include <uActor.h>

int Times = 100'000'000;								// default values
time_point<steady_clock> starttime;

struct PMsg : public uActor::PromiseMsg<int> { int cnt; PMsg( int cnt ) : PromiseMsg( uActor::Delete ), cnt( cnt ) {} };

_Actor Promise {
	Allocation receive( Message & msg ) {
		Case ( PMsg, msg ) {
			if ( msg_d->cnt >= Times ) {
				cout << Times << ' ' << (steady_clock::now() - starttime).count() / Times << "ns" << endl;
				return Delete;
			} // if
			//cout << msg_d->cnt << endl;
			*this | *new PMsg{ msg_d->cnt + 1 };		// message send self
		} // Case
		return Nodelete;
	} // Send:::receive
}; // Promise

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
	*(new Promise) | *new PMsg{ 0 };
	uActor::stop();										// wait for all actors to terminate
} // main

// /usr/bin/time -f "%Uu %Ss %Er %Mkb" a.out

// Local Variables: //
// compile-command: "u++-work -g -Wall -Wextra -O3 -nodebug -DNDEBUG -multi uC++PromiseDynamic.cc" //
// End: //
