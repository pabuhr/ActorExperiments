#include <string>
using namespace std;
#include <uActor.h>
#include <chrono>
using namespace chrono;
#include <iostream>

struct IntMsg : public uActor::PromiseMsg< int > {
	int val;   IntMsg() {}
};
struct StrMsg : public uActor::PromiseMsg< string > {
	string val;   StrMsg() {}
};

size_t Messages = 1'000'000, Processors = 10, QScale = 2;
time_point<steady_clock> starttime;

_Actor Server {
	Allocation receive( uActor::Message & msg ) {
		Case( IntMsg, msg ) { msg_d->delivery( 7 ); }
		else Case( StrMsg, msg ) { msg_d->delivery( "XYZ" ); }
		else Case( StopMsg, msg ) { return Finished; }
		return Nodelete;								// reuse actor
	}
};

Server * servers;

_Actor Client {
	IntMsg * intmsg;   uActor::Promise<int> * pi;
	StrMsg * strmsg;   uActor::Promise<string> * ps;
	size_t results = 0, times = 0;
	size_t wi = 0, ws = 0;

	#define ICB [this]( int ) { this->results += 1; if ( this->results == 2 * Messages ) { terminateServers(); return Finished; } return Nodelete; }
	#define SCB [this]( string ) { this->results += 1; if ( this->results == 2 * Messages ) { terminateServers(); return Finished; } return Nodelete; }

	void terminateServers() {
		for ( unsigned int i = 0; i < Messages; i += 1 ) {
			servers[i] | uActor::stopMsg;
		} // for
		cout << Processors << " " << (steady_clock::now() - starttime).count() / 1'000'000'000.0 << "s"
			 << " " << wi << " " << ws
			 << endl;
	}

	Allocation receive( uActor::Message & ) {			// receive callback messages
		starttime = steady_clock::now();				// start time here to avoid measuring heap allocations
		for ( size_t i = 0; i < Messages; i += 1 ) {	// send out work
			pi[i] = servers[i] || intmsg[i];			// ask send
			ps[i] = servers[i] || strmsg[i];
		}
		// Do some other work and then process already completed requests from server.
		for ( size_t i = 0; i < Messages; i += 1 ) {	// any promise fulfilled ?
			if ( pi[i].then( ICB ) ) wi += 1;
			if ( ps[i].then( SCB ) ) ws += 1;
		}
		if ( results == 2 * Messages ) { terminateServers(); return Finished; }	// all messages handled ?
		// Any outstanding server messages are handled by implicit callbacks from the server.
		return Nodelete;								// reuse actor
	}
  public:
	Client() {
		intmsg = new IntMsg[Messages];
		strmsg = new StrMsg[Messages];
		pi = new uActor::Promise<int>[Messages];
		ps = new uActor::Promise<string>[Messages];
	}
	~Client() {
		delete[] intmsg;
		delete[] strmsg;
		delete[] pi;
		delete[] ps;
	}
};

int main( int argc, char * argv[] ) {
	switch ( argc ) {
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
			 << ") ]" << endl;
		exit( EXIT_FAILURE );
	} // switch

	uExecutor executor( Processors, Processors, Processors == 1 ? 1 : Processors * QScale, true, -1 );
	uActor::start( &executor );							// start actor system
	servers = new Server[Messages];
	Client client;
	client | uActor::startMsg;							// start actors
	uActor::stop();										// wait for all actors to terminate
	delete[] servers;
//	delete executor;
}

// Local Variables: //
// compile-command: "u++-work -g -Wall -Wextra -O3 -nodebug -DNDEBUG -multi uC++Promise.cc" //
// End: //
