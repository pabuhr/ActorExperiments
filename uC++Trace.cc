#include <iostream>
using namespace std;
#include <chrono>
using namespace chrono;
#include <uActor.h>

int Times = 1'000'000;									// default values
time_point<steady_clock> starttime;

struct Msg : public uActor::Message {} msg;				// regular message
struct TMsg : public uActor::TraceMsg {} tmsg;			// traceable message

_Actor Trace {
	enum { Msgs = 100 };
	int times = 0, mcnt = 0;

	Allocation msgSend( Message & ) {
		if ( times >= Times ) {
			cout << "uC++ Trace Msg " << Times << ' ' << (steady_clock::now() - starttime).count() / Times / Msgs << "ns" << endl;
			times = 0;									// reset
			starttime = steady_clock::now();			// reset timer
			become( &Trace::tmsgSend );
			*this | msg;								// start trace become
			return Nodelete;
		} // if
		for ( int i = 0; i < Msgs; i += 1 ) *this | msg; // send self N messages
		become( &Trace::msgReceive );
		return Nodelete;
	}
	Allocation msgReceive( Message & ) {
		mcnt += 1;
		if ( mcnt == Msgs ) {							// receive N messages and then toggle state
			times += 1;
			mcnt = 0;
			become( &Trace::msgSend );
			*this | msg;
		} // if
		return Nodelete;
	}
	Allocation tmsgSend( Message & ) {
		if ( times >= Times ) {
			cout << "uC++ Trace TMsg " << Times << ' ' << (steady_clock::now() - starttime).count() / Times / Msgs << "ns" << endl;
			return Finished;
		} // if
		for ( int i = 0; i < Msgs; i += 1 ) *this | tmsg; // send self N messages
		become( &Trace::tmsgReceive );
		//tmsg.print();
		return Nodelete;
	}
	Allocation tmsgReceive( Message & ) {				// receive N messages and then toggle state
		mcnt += 1;
		if ( mcnt == Msgs ) {
			times += 1;
			mcnt = 0;
			become( &Trace::tmsgSend );
			tmsg.erase();								// prevent trace build up
			*this | tmsg;
		} // if
		return Nodelete;
	}
  public:
	Trace() { become( &Trace::msgSend ); }
}; // Trace

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
	Trace trace;
	starttime = steady_clock::now();
	trace | msg;
	uActor::stop();										// wait for all actors to terminate
	// malloc_stats();
} // main

// /usr/bin/time -f "%Uu %Ss %Er %Mkb" a.out

// Local Variables: //
// compile-command: "u++-work -g -Wall -Wextra -O3 -nodebug -DNDEBUG -multi uC++Trace.cc" //
// End: //
