#include <iostream>
#include <string>
using namespace std;
#include <uActor.h>

struct IntMsg : public uActor::PromiseMsg<int> { int val; };
struct StrMsg : public uActor::PromiseMsg<string> { string val; };

int serverDelay = 0;
#define Delay( N ) for ( volatile int delay = 0; delay < N; delay += 1 ) {}

_Actor Server {
	Allocation receive( uActor::Message & msg ) {
        Delay( serverDelay )
		Case( IntMsg, msg ) { msg_d->delivery( 7 ); }
		else Case( StrMsg, msg ) { msg_d->delivery( "XYZ" ); }
		else Case( StopMsg, msg ) { return Finished; }
		return Nodelete;								// reuse actor
	}
};

int numFastPath = 0;

_Actor Client {
	enum { Messages = 10'000'000, Post = 1'000 };		// number of message kinds sent
	IntMsg intmsg[Post];  uActor::Promise<int> pi[Post];
	StrMsg strmsg[Post];  uActor::Promise<string> ps[Post];
	int results = 0, posted = 0, ints = 0, strs = 0;
	Server & server;

	Allocation check() {
		if ( posted != 2 * Post ) { return Nodelete; }
		results += posted; posted = 0;
		if ( results >= 2 * Messages ) { server | uActor::stopMsg; return Finished; }
		*this | uActor::startMsg;						// setup for next batch
		return Nodelete;								// reuse actor
	}

	Allocation receive( uActor::Message & ) {			// receive callback messages
		for ( unsigned int i = 0; i < Post; i += 1 ) {	// send out work
			pi[i].reset(); pi[i] = server || intmsg[i];	// ask send
			ps[i].reset(); ps[i] = server || strmsg[i];
		} // for
		// do other work and then process completed requests from server
		for ( unsigned int i = 0; i < Post; i += 1 ) {	// any promise fulfilled ?
			#define DUP { posted += 1; return check(); }
			if ( pi[i].then( [this]( int ) { ints += 1; DUP } ) ) numFastPath += 1;
			if ( ps[i].then( [this]( string ) { strs += 1; DUP } ) ) numFastPath += 1;
		} // for
		return check();
	}
  public:
	Client( Server & server ) : server( server ) {}
};

int main( int argc, char * argv[] ) {
    switch ( argc ) {
	  case 2:
		if ( strcmp( argv[1], "d" ) != 0 ) {			// default ?
			if ( stoi( argv[1] ) < 0 ) goto Usage;
			serverDelay = stoi( argv[1] );
		} // if
	  case 1:											// use defaults
		break;
	  default:
	  Usage:
		cerr << "Usage: " << argv[0]
			 << " [ delay >= 0 | d ]" << endl;
		exit( EXIT_FAILURE );
	} // switch

    uProcessor p[1];
	uActor::start();									// start actor system
	Server server;
	Client client( server );
	client | uActor::startMsg;							// start actors
	uActor::stop();										// wait for all actors to terminate
	cout << "Promise server delay " << serverDelay << "  numFastPath " << numFastPath << endl;
}

// Local Variables: //
// compile-command: "u++-work -g -Wall -Wextra -O3 -nodebug -DNDEBUG -multi uC++Promise.cc" //
// End: //
