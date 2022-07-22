#include <string>
#include <iostream>
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

_CorActor Client {
	enum { Messages = 10'000'000, Post = 1'000 };		// number of message kinds sent
	IntMsg intmsg[Post];  uActor::Promise<int> pi[Post];
	StrMsg strmsg[Post];  uActor::Promise<string> ps[Post];
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
				pi[p].reset(); pi[p] = server || intmsg[p];	// ask send
				ps[p].reset(); ps[p] = server || strmsg[p];
			} // for
			// do other work and then process completed requests from server
			for ( unsigned int p = 0; p < Post; p += 1 ) { // any promise fulfilled ?
				#define SLOW { resume(); return state; }
				#define FAST { numFastPath += 1; posted += 1; }
				if ( pi[p].then( [this]( int ) { ints += 1; SLOW } ) ) FAST
				if ( ps[p].then( [this]( string ) { strs += 1; SLOW } ) ) FAST
			} // for
			for ( ; posted < Post * 2; posted += 1 ) { // receive remaining results
				suspend();
			} // for
			posted = 0;
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
	cout << "Promise server delay " << serverDelay << "  numFastPath " << numFastPath << endl;
}

// Local Variables: //
// compile-command: "u++-work -g -Wall -Wextra -O3 -nodebug -DNDEBUG -multi uC++PromiseCor.cc" //
// End: //
