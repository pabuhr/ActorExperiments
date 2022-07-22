#include <string>
#include <iostream>
using namespace std;
#include <uActor.h>

struct IntMsg : public uActor::SenderMsg { int val; };
struct StrMsg : public uActor::SenderMsg { string val; };

int serverDelay = 0;
#define Delay( N ) for ( volatile int delay = 0; delay < N; delay += 1 ) {}

_Actor Server {
	Allocation receive( uActor::Message & msg ) {
        Delay( serverDelay )
		Case( IntMsg, msg ) { msg_d->val = 7; *msg_d->sender() | *msg_d; }
		else Case( StrMsg, msg ) { msg_d->val = "XYZ"; *msg_d->sender() | *msg_d; }
		else Case( StopMsg, msg ) { return Finished; }
		return Nodelete;								// reuse actor
	}
};

_Actor Client {
	enum { Messages = 10'000'000, Post = 1'000 };		// number of message kinds sent
	IntMsg intmsg[Post];
	StrMsg strmsg[Post];
	int results = 0, posted = 0, ints = 0, strs = 0;
	Server & server;

	Allocation check() {
		if ( posted != 2 * Post ) { return Nodelete; }
		results += posted; posted = 0;
		if ( results >= 2 * Messages ) { server | uActor::stopMsg; return Finished; }
		*this | uActor::startMsg;						// setup for next batch
		become( &Client::receive );
		return Nodelete;								// reuse actor
	}

	Allocation receive( uActor::Message & ) {
		for ( unsigned int i = 0; i < Post; i += 1 ) {	// send work
			server | intmsg[i];							// tell
			server | strmsg[i];
		}
		// do other work and then process completed requests from server
		become( &Client::receive2 );					// switch to results
		return Nodelete;								// reuse actor
	}

	Allocation receive2( uActor::Message & msg ) {		// receive results
		Case( IntMsg, msg ) { ints += 1; }
		else Case( StrMsg, msg ) { strs += 1; }
		posted += 1;
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
	cout << "NoPromise server delay " << serverDelay << endl;
}

// Local Variables: //
// compile-command: "u++-work -g -Wall -Wextra -O3 -nodebug -DNDEBUG -multi uC++PromiseNo.cc" //
// End: //
