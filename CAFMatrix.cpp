#include <iostream>
using namespace std;
#include <chrono>
using namespace chrono;

#include "caf/actor_ostream.hpp"
#include "caf/caf_main.hpp"
#include "caf/event_based_actor.hpp"
#include "caf/all.hpp"
using namespace caf;

struct WorkMsg;

CAF_BEGIN_TYPE_ID_BLOCK(custom_types_1, first_custom_type_id)
CAF_ADD_TYPE_ID(custom_types_1, (WorkMsg))
CAF_END_TYPE_ID_BLOCK(custom_types_1)

// --(rst-foo-begin)--
struct WorkMsg {
	unsigned int b;
};

template <class Inspector>
bool inspect(Inspector& f, WorkMsg& x) {
	return f.object(x).fields(f.field("b", x.b));
}

unsigned int xr = 3'072, xc = 3'072, yc = 3'072, Processors = 1; // default values

time_point<steady_clock> starttime;
actor * actors;
WorkMsg ** work_msgs;
unsigned int actorCnt = 0;

int wCount = 0;

int ** w_Z, ** w_X, ** w_Y;

class MatrixMult : public event_based_actor {
	behavior make_behavior() override {
		return {
			[=]( const WorkMsg & val ) -> void {
				unsigned int w = val.b;
				for ( unsigned int i = 0; i < yc; i += 1 ) { // multiply X_row by Y_col and sum products
					w_Z[w][i] = 0;
					for ( unsigned int j = 0; j < xc; j += 1 ) {
						w_Z[w][i] += w_X[w][j] * w_Y[j][i];
					} // for
				} // for
				
				if ( __atomic_add_fetch( &actorCnt, 1, __ATOMIC_SEQ_CST ) == xr ) {
					aout(this) << Processors << " " << (steady_clock::now() - starttime).count() / 1'000'000'000.0 << "s" << endl;
				} // if
				this->quit();
				return;
			}
		};
	}
  public:
	MatrixMult( caf::actor_config & cfg ) : event_based_actor( cfg ) {}
}; // MatrixMult

void caf_main( actor_system & sys ) {
	starttime = steady_clock::now();

	for ( unsigned int i = 0; i < xr; i += 1 ) {		// create actors
		actors[i] = sys.spawn<MatrixMult>();
	} // for

	caf::scoped_actor self{sys};
	for ( unsigned int i = 0; i < xr; i += 1 ) {		// start actors
		self->send( actors[i], WorkMsg{i} );
	} // for
} // caf_main

int main( int argc, char * argv[] ) {
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

	//cout << Actors << " " << Set << " " << Rounds << " " << Processors << endl;
	actors = new actor[xr];

	caf::core::init_global_meta_objects();
	caf::exec_main_init_meta_objects<id_block::custom_types_1>();

	caf::actor_system_config cfg;
	cfg.set( "caf.scheduler.max-threads", Processors );
	caf::actor_system system { cfg };

	unsigned int r, c;
	int * Z[xr], * X[xr], * Y[xc];

	w_Z = Z;
	w_Y = Y;
	w_X = X;

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

	caf::exec_main<>(caf_main, argc, argv);
	
	for ( r = 0; r < xr; r += 1 ) {						// deallocate X and Z matrices
		delete [] X[r];
		delete [] Z[r];
	} // for
	for ( r = 0; r < xc; r += 1 ) {						// deallocate Y matrix
		delete [] Y[r];
	} // for
} // main
//CAF_MAIN()

// /usr/bin/time -f "%Uu %Ss %Er %Mkb" a.out

// Local Variables: //
// compile-command: "g++-10 -Wall -O3 -std=c++17 -ICAF/actor-framework/libcaf_core -ICAF/actor-framework/libcaf_core/caf -ICAF/actor-framework/build/libcaf_core -LCAF/actor-framework/build/libcaf_core -LCAF/actor-framework/build/libcaf_io CAFMatrix.cpp -lcaf_io -lcaf_core -Wl,-rpath=CAF/actor-framework/build/libcaf_core" //
// End: //
