// https://github.com/AsynkronIT/protoactor-go
// https://pkg.go.dev/github.com/AsynkronIT/protoactor-go/actor
package main

import (
	"os"; "strconv"; "fmt"; "time"; "runtime"; "sync/atomic"
	"github.com/asynkron/protoactor-go/actor"
)

var xr, xc, yc uint64 = 3_072, 3_072, 3_072 // default values

var Processors int = 1

var starttime time.Time;
var shake = make( chan string )
var actorCnt uint64 = 0

type Msg struct {
	Z []int
	X []int
	Y [][]int
}

func NewMsg( Z_ []int, X_ []int, Y_ [][]int ) *Msg {
	return &Msg{ Z: Z_, X: X_, Y: Y_ }
}

var msg Msg
type MatrixMult struct {
	actors [] * actor.PID;
	id, rounds, group, recs, sends int;
}
var system * actor.ActorSystem

func ( state * MatrixMult ) Receive( context actor.Context ) {
	switch msg := context.Message().(type) {
	  case * Msg:
		for i := uint64(0); i < yc; i += 1 {
			msg.Z[i] = 0
			for j := uint64(0); j < xc; j += 1 {
				msg.Z[i] = msg.X[j] * msg.Y[j][i]
			}
		}

		if ( atomic.AddUint64( &actorCnt, 1 ) == xr ) {
			shake <- "hand"
		} // if
	  default:
		// ignore actor.Started message
	}
}

func usage() {
	fmt.Printf( "Usage: %v " +
		"[ yc (> 0) | 'd' (default %v) ] " +
		"[ xc (> 0) | 'd' (default %v) ] " +
		"[ xr (> 0) | 'd' (default %v) ]\n",
		"[ processors (> 0) | 'd' (default %v) ]\n",
		os.Args[0], yc, xc, xr, Processors );
	os.Exit( 1 );
}

func main() {
	switch len( os.Args ) {
	  case 5:
		if os.Args[4] != "d" {							// default ?
			Processors, _ = strconv.Atoi( os.Args[4] )
			if Processors < 1 { usage(); }
		} // if
		fallthrough
	  case 4:
		if os.Args[3] != "d" {							// default ?
			xr, _ = strconv.ParseUint( os.Args[3], 10, 64 )
			if xr < 1 { usage(); }
		} // if
		fallthrough
	  case 3:
		if os.Args[2] != "d" {							// default ?
			xc, _ = strconv.ParseUint( os.Args[2], 10, 64 )
			if xc < 1 { usage(); }
		} // if
		fallthrough
	  case 2:
		if os.Args[1] != "d" {							// default ?
			yc, _ = strconv.ParseUint( os.Args[1], 10, 64 )
			if yc < 1 { usage(); }
		} // if
	  case 1:											// use defaults
	  default:
		usage();
	} // switch

	runtime.GOMAXPROCS( Processors );
	system = actor.NewActorSystem();
	

	// set up matrices
	X := make( [][]int, xr )
	Y := make( [][]int, xc )
	Z := make( [][]int, xr )
	
	for r := uint64(0); r < xr; r += 1 { // set up X subarrays
		X[r] = make( []int, xc )
		for c := uint64(0); c < xc; c += 1 { // set up X values
			X[r][c] = int(r * c % 37)
		}
	}

	for r := uint64(0); r < xc; r += 1 { // set up Y subarrays
		Y[r] = make( []int, yc )
		for c := uint64(0); c < yc; c += 1 { // set up Y values
			Y[r][c] = int(r * c % 37)
		}
	}

	for r := uint64(0); r < xr; r += 1 { // set up Z subarrays
		Z[r] = make( []int, yc )
	}

	system = actor.NewActorSystem();

	starttime = time.Now();
	actors := make( [] * actor.PID, xr );
	messages := make( []*Msg, xr );
	for r := uint64(0); r < xr; r += 1 { // create messages and actors
		messages[r] = NewMsg(Z[r], X[r], Y);
		props := actor.PropsFromProducer( func() actor.Actor { return &MatrixMult{} })
		actors[r] = system.Root.Spawn(props)
	} // for
	for r := uint64(0); r < xr; r += 1 { // send messages to actors
		system.Root.Send( actors[r], messages[r] );
	} // for

	<- shake // wait for actors to finish

	fmt.Printf( "%d %.2fs\n", Processors, time.Since( starttime ).Seconds() );
} // main

// /usr/bin/time -f "%Uu %Ss %Er %Mkb" GoMatrix

// Local Variables: //
// tab-width: 4 //
// compile-command: "go build" //
// End: //
