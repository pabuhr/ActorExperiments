#include <iostream>
using namespace std;
#include <chrono>
using namespace chrono;
#include <uActor.h>

int Times = 100'000'000;								// default values
time_point<steady_clock> starttime;

struct SMsg : public uActor::SenderMsg { int cnt; SMsg( int cnt ) : cnt( cnt ) {} } smsg{ 0 }; // default Nodelete

_Actor Sender {
	Allocation receive( Message & msg ) {
		Case ( SMsg, msg ) {
			if ( msg_d->cnt >= Times ) {
				cout << "uC++ Sender Static " << Times << ' ' << (steady_clock::now() - starttime).count() / Times << "ns" << endl;
				return Finished;
			} // if
			//cout << msg_d->cnt << endl;
			msg_d->cnt += 1;
			*this | *msg_d;								// static actor / message send self
		} // Case
		return Nodelete;
	} // Send:::receive
}; // Sender

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
	Sender trace;
	trace | smsg;
	uActor::stop();										// wait for all actors to terminate
} // main

// /usr/bin/time -f "%Uu %Ss %Er %Mkb" a.out

// Local Variables: //
// compile-command: "u++-work -g -Wall -Wextra -O3 -nodebug -DNDEBUG -multi uC++SenderStatic.cc" //
// End: //
