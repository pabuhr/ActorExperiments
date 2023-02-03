#include <string>
#include <iostream>
using namespace std;
#include <uActor.h>
#include <chrono>
using namespace chrono;

struct IntMsg : public uActor::SenderMsg { int val; };
struct StrMsg : public uActor::SenderMsg { string val; };

size_t Messages = 100'000, Processors = 4, QScale = 256, Times = 10;
time_point<steady_clock> starttime;

_Actor Server {
	Allocation receive( uActor::Message & msg ) {
		Case( IntMsg, msg ) { msg_d->val = 7; *msg_d->sender() | msg; }
		else Case( StrMsg, msg ) { msg_d->val = "XYZ"; *msg_d->sender() | msg; }
		else Case( StopMsg, msg ) { return Finished; }
		return Nodelete;								// reuse actor
	}
};

Server * servers;

_CorActor Client {
	IntMsg * intmsg;
	StrMsg * strmsg;
	size_t results = 0;

	uActor::Message * msg;
	Allocation state = Nodelete;						// communication

	Allocation receive( uActor::Message & msg ) {
		Client::msg = &msg;
		resume();
		return state;
	}

	void main() {
		for ( size_t times = 0;; times += 1 ) {
			for ( size_t i = 0; i < Messages; i += 1 ) { // send out work
				servers[i] | intmsg[i];					// tell send
				servers[i] | strmsg[i];
			}
			for ( size_t i = 0; i < 2 * Messages; i += 1 ) {
				suspend();
				Case( IntMsg, msg ) results += 1;
				else Case( StrMsg, msg ) results += 1;
			}
		  if ( times == Times - 1 ) break;
			results = 0;
			*this | uActor::startMsg;
			suspend();
		}
		for ( unsigned int i = 0; i < Messages; i += 1 ) {
			servers[i] | uActor::stopMsg;
		}
		state = Finished;
	}
  public:
	Client() {
		intmsg = new IntMsg[Messages];
		strmsg = new StrMsg[Messages];
	}
	~Client() {
		delete [] strmsg;
		delete [] intmsg;
	}
};

int main( int argc, char * argv[] ) {
	switch ( argc ) {
	  case 5:
		if ( strcmp( argv[4], "d" ) != 0 ) {			// default ?
			Times = stoi( argv[4] );
			if ( Times < 1 ) goto Usage;
		} // if
	  case 4:
		if ( strcmp( argv[3], "d" ) != 0 ) {			// default ?
			QScale = stoi( argv[3] );
			if ( QScale < 1 ) goto Usage;
		} // if
	  case 3:
		if ( strcmp( argv[2], "d" ) != 0 ) {			// default ?
			Processors = stoi( argv[2] );
			if ( Processors < 1 ) goto Usage;
		} // if
	  case 2:
		if ( strcmp( argv[1], "d" ) != 0 ) {			// default ?
			Messages = stoi( argv[1] );
			if ( Messages < 1 ) goto Usage;
		} // if
	  case 1:											// use defaults
		break;
	  default:
	  Usage:
		cerr << "Usage: " << argv[0]
			 << ") ] [ messages (> 0) | 'd' (default " << Messages
			 << ") ] [ processors (> 0) | 'd' (default " << Processors
			 << ") ] [ qscale (> 0) | 'd' (default " << QScale
			 << ") ] [ Times (> 0) | 'd' (default " << Times
			 << ") ]" << endl;
		exit( EXIT_FAILURE );
	} // switch

	uExecutor executor( Processors, Processors, Processors == 1 ? 1 : Processors * QScale, true, -1 );
	uActor::start( &executor );							// start actor system
	servers = new Server[Messages];
	Client client;
	client | uActor::startMsg;							// start actors
	time_point<steady_clock> starttime = steady_clock::now();
	uActor::stop();										// wait for all actors to terminate
	cout << Processors << " " << (steady_clock::now() - starttime).count() / 1'000'000'000.0 << "s" << endl;
	delete [] servers;
}

// Local Variables: //
// compile-command: "u++-work -g -Wall -Wextra -O3 -nodebug -DNDEBUG -multi uC++NoPromiseRepeatCor.cc" //
// End: //
