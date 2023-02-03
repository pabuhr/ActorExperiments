#include <iostream>
#include <algorithm>
using namespace std;
#include <chrono>
using namespace chrono;
#include <uActor.h>

unsigned int xr = 3'072, xc = 3'072, yc = 3'072, Processors = 1; // default values

struct WorkMsg : public uActor::Message {				// derived message
	int * Z;
	const int * const X, * const * Y;
	WorkMsg( int Z[], const int X[], const int * const Y[] ) :
		Message( uActor::Finished ), Z( Z ), X( X ), Y( Y ) {} // one-shot
}; // WorkMsg

time_point<steady_clock> starttime;

_Actor MatrixMult {
	Allocation receive( Message & msg ) {
		Case ( WorkMsg, msg ) {
			int * z = msg_d->Z;							// optimizations
			const int * const x = msg_d->X, * const * y = msg_d->Y;
			for ( unsigned int i = 0; i < yc; i += 1 ) { // multiply X_row by Y_col and sum products
				z[i] = 0;
				for ( unsigned int j = 0; j < xc; j += 1 ) {
					z[i] += x[j] * y[j][i];
				} // for
			} // for
		} // Case

		return Finished;
	} // MatrixMult:::receive
}; // MatrixMult

int main( int argc, char * argv[] ) {
	locale loc( getenv("LANG") );
	cout.imbue( loc );

	switch ( argc ) {
	  case 5:
		if ( strcmp( argv[4], "d" ) != 0 ) {			// default ?
			Processors = stoi( argv[4] );
			if ( Processors < 1 ) goto Usage;
		} // if
	  case 4:
		if ( strcmp( argv[3], "d" ) != 0 ) {			// default ?
			xr = stoi( argv[3] );
			if ( xr < 1 ) goto Usage;
		} // if
	  case 3:
		if ( strcmp( argv[2], "d" ) != 0 ) {			// default ?
			xc = stoi( argv[2] );
			if ( xc < 1 ) goto Usage;
		} // if
	  case 2:
		if ( strcmp( argv[1], "d" ) != 0 ) {			// default ?
			yc = stoi( argv[1] );
			if ( yc < 1 ) goto Usage;
		} // if
	  case 1:											// use defaults
		break;
	  default:
	  Usage:
		cerr << "Usage: " << argv[0]
			 << " [ yc (> 0) | 'd' (default " << yc
			 << ") ] [ xc (> 0) | 'd' (default " << xc
			 << ") ] [ xr (> 0) | 'd' (default " << xr
			 << ") ] [ processors (> 0) | 'd' (default " << Processors
			 << ") ]" << endl;
		exit( EXIT_FAILURE );
	} // switch

	unsigned int r, c;
	int * Z[xr], * X[xr], * Y[xc];

	for ( r = 0; r < xr; r += 1 ) {						// create/initialize X matrix
		X[r] = new int[xc];
		for ( c = 0; c < xc; c += 1 ) {
			X[r][c] = r * c % 37;						// for timing
		} // for
	} // for
	for ( r = 0; r < xc; r += 1 ) {						// create/initialize Y matrix
		Y[r] = new int[yc];
		for ( c = 0; c < yc; c += 1 ) {
			Y[r][c] = r * c % 37;						// for timing
		} // for
	} // for
	for ( r = 0; r < xr; r += 1 ) {						// create Z matrix
		Z[r] = new int[yc];
	} // for

	uExecutor * executor = new uExecutor( Processors, Processors, Processors == 1 ? 1 : Processors * 32, true, -1 );
	uActor::start( executor );							// start actor system
	uNoCtor<MatrixMult> * multiply = new uNoCtor<MatrixMult>[xr];
	uNoCtor<WorkMsg> * workMsg = new uNoCtor<WorkMsg>[xr];	

	for ( unsigned int r = 0; r < xr; r += 1 ) {
		multiply[r].ctor();
		workMsg[r].ctor( Z[r], X[r], (const int * const *)Y );
	} // for

	starttime = steady_clock::now();
	for ( unsigned int r = 0; r < xr; r += 1 ) {
		*multiply[r] | *workMsg[r];
	} // for

	uActor::stop();										// wait for all actors to terminate

	cout << Processors << " " << (steady_clock::now() - starttime).count() / 1'000'000'000.0 << "s" << endl;
	
	for ( r = 0; r < xr; r += 1 ) {						// deallocate X and Z matrices
		delete [] X[r];
		delete [] Z[r];
	} // for
	for ( r = 0; r < xc; r += 1 ) {						// deallocate Y matrix
		delete [] Y[r];
	} // for

	// malloc_stats();
} // main

// /usr/bin/time -f "%Uu %Ss %Er %Mkb" a.out

// Local Variables: //
// compile-command: "u++-work -g -Wall -Wextra -O3 -nodebug -DNDEBUG -multi uC++Matrix.cc" //
// End: //
