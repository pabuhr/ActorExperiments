#include <iostream>
using namespace std;
#include <uActor.h>
#include <chrono>
using namespace chrono;

int Times = 100'000'000;								// default values
time_point<steady_clock> starttime;

struct Msg : public uActor::Message {} msg;

_CorActor SusRes {
	Allocation status = Nodelete;

	Allocation receive( Message & msg ) {
		Case ( Msg, msg ) {
			resume();
		} // Case
		return status;
	} // SusRes::receive

	void main() {
		// Two sends per iteration => divide Times by 2.
		for ( int cnt = 0; cnt < Times / 2; cnt += 1 ) {
			// state 1
			*this | msg;								// send message to self
			suspend();
			// state 2
			*this | msg;								// send message to self
			suspend();
		} // for
		cout << "uC++ CorActor " << Times << ' ' << (steady_clock::now() - starttime).count() / Times << "ns" << endl;
		status = Finished;
	} // SusRes::main
}; // SusRes

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

	uActor::start();									// start actor system
	starttime = steady_clock::now();
	SusRes susres;
	susres | msg;
	uActor::stop();										// wait for all actors to terminate
} // main

// /usr/bin/time -f "%Uu %Ss %Er %Mkb" a.out

// Local Variables: //
// compile-command: "u++-work -g -Wall -Wextra -O3 -nodebug -DNDEBUG -multi uC++CorActor.cc" //
// End: //
