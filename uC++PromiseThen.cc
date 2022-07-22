#include <string>
using namespace std;
#include <uActor.h>

struct IntMsg : public uActor::PromiseMsg< int > {
	int val;   IntMsg() {}   IntMsg( int val ) : PromiseMsg( uActor::Delete ), val( val ) {}
};
struct StrMsg : public uActor::PromiseMsg< string > {
	string val;   StrMsg() {}   StrMsg( string val ) : PromiseMsg( uActor::Delete ), val( val ) {}
};
int delay = 100;
_Actor Server {
	Allocation receive( uActor::Message & msg ) {
		Case( IntMsg, msg ) { msg_d->delivery( 7 ); }
		else Case( StrMsg, msg ) { msg_d->delivery( "XYZ" ); }
		else Case( StopMsg, msg ) { return Finished; }
		return Nodelete;							// reuse actor
	}
};
_Actor Client {
	enum { Messages = 100 };								// number of message kinds sent
	
	IntMsg intmsg[Messages];   uActor::Promise<int> pi[Messages];
	StrMsg strmsg[Messages];   uActor::Promise<string> ps[Messages];
	int results = 0;
	Server & server;

	#define ICB [this]( int ) { this->results += 1; if ( this->results == 2 * Messages ) { this->server | uActor::stopMsg; return Finished; } return Nodelete; }
	#define SCB [this]( string ) { this->results += 1; if ( this->results == 2 * Messages ) { this->server | uActor::stopMsg; return Finished; } return Nodelete; }

	Allocation receive( uActor::Message & ) {		// receive callback messages
		for ( unsigned int i = 0; i < Messages; i += 1 ) { // send out work
			pi[i] = server || intmsg[i];			// ask send
			ps[i] = server || strmsg[i];
		}
		// do some other work and then process already completed requests from server
		for ( unsigned int i = 0; i < Messages; i += 1 ) { // any promise fulfilled ?
			pi[i].then( ICB );
			ps[i].then( SCB );
		}
		if ( results == 2 * Messages ) { server | uActor::stopMsg; return Finished; }
		return Nodelete;							// reuse actor
	}
  public:
	Client( Server & server ) : server( server ) {}
};
int main() {
	uActor::start();								// start actor system
	Server server;
	Client client( server );
	client | uActor::startMsg;						// start actors
	uActor::stop();									// wait for all actors to terminate
}

// Local Variables: //
// compile-command: "u++-work -g -Wall -Wextra -O3 -nodebug -DNDEBUG -multi uC++Promise.cc" //
// End: //
