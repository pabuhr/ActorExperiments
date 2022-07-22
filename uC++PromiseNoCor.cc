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

_CorActor Client {
	enum { Messages = 10'000'000, Post = 1'000 };		// number of message kinds sent
	IntMsg intmsg[Post];
	StrMsg strmsg[Post];
	int results = 0, posted = 0, ints = 0, strs = 0;
	Server & server;
	uActor::Message * msg;								// communication
	Allocation state = Nodelete;

	Allocation receive( uActor::Message & msg ) {
		Client::msg = &msg;
		resume();
		return state;
	}

	void main() {
		for ( unsigned int b = 0; b < (2 * Messages) / (2 * Post); b += 1 ) {
			for ( unsigned int p = 0; p < Post; p += 1 ) { // send work
				server | intmsg[p];						// tell
				server | strmsg[p];
			} // for
			// do other work and then process completed requests from server
			for ( unsigned int p = 0; p < 2 * Post; p += 1 ) { // receive results
				suspend();
				Case( IntMsg, msg ) { ints += 1; }
				else Case( StrMsg, msg ) { strs += 1; }
			} // for
			*this | uActor::startMsg;					// setup for next batch
			suspend();
		} // for
		server | uActor::stopMsg;
		state = Finished;								// mark done
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
// compile-command: "u++-work -g -Wall -Wextra -O3 -nodebug -DNDEBUG -multi uC++PromiseNoCor.cc" //
// End: //
