#include <iostream>
using namespace std;
#include <chrono>
using namespace chrono;

int Times = 100'000'000;								// default values
time_point<steady_clock> starttime;

_Coroutine SusRes {
	void main() {
		for ( int i = 0; i < Times; i += 1 ) {
			suspend();
		} // for
	} // SusRes::main
  public:
	void next() {
		starttime = steady_clock::now();
		for ( int i = 0; i < Times; i += 1 ) {
			resume();
		} // for
		cout << Times << ' ' << (steady_clock::now() - starttime).count() / Times << "ns" << endl;
	} // SusRes::next
}; // SusRes

int main( int argc, char * argv[] ) {
	switch ( argc ) {
	  case 2:
		if ( strcmp( argv[1], "d" ) != 0 ) { Times = stoi( argv[1] ); }
		if ( Times < 1 ) goto Usage;
	  case 1:											// use defaults
		break;
	  default:
	  Usage:
		cerr << "Usage: " << argv[0] << " [ times (> 0) ]" << endl;
		exit( EXIT_FAILURE );
	} // switch

	SusRes susres;
	susres.next();
} // main

// /usr/bin/time -f "%Uu %Ss %Er %Mkb" a.out

// Local Variables: //
// compile-command: "u++-work -g -Wall -Wextra -O3 -nodebug -DNDEBUG -multi uC++Coroutine.cc" //
// End: //
