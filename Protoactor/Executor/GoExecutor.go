// https://github.com/asynkron/protoactor-go
// https://pkg.go.dev/github.com/asynkron/protoactor-go/actor
package main

import (
	"os"; "strconv"; "fmt"; "time"; "runtime"; "sync/atomic"
	"github.com/asynkron/protoactor-go/actor"
)

var Actors, Set, Rounds, Processors int = 40_000, 100, 100, 1 // default values
var Batch int = 1
var starttime time.Time;
var shake = make( chan string )
var actorCnt int64 = 0

type Msg struct {}
var msg Msg
type Executor struct {
	actors [] * actor.PID;
	id, rounds, group, recs, sends int;
}
var system * actor.ActorSystem

func ( state * Executor ) Receive( context actor.Context ) {
	switch context.Message().(type) {
	  case * Msg:
		if state.recs == state.rounds {
			if ( atomic.AddInt64( &actorCnt, 1 ) == int64(Actors) ) {
				fmt.Printf( "Protoactor Executor %.2fs\n", time.Since( starttime ).Seconds() );
				shake <- "hand"
			} // if
			return;
		} // if
		if state.recs % Batch == 0 {
			for i := 0; i < Batch; i += 1 {
				system.Root.Send( state.actors[state.group + state.sends % Set], &msg ); // cycle through group
				state.sends += 1;
			}
		}
	  	state.recs += 1;
		//fmt.Printf( "%v %v %v %v %v\n", state.id, state.group, state.recs, state.sends, state.group + state.sends % Set );
	  default:
		// ignore actor.Started message
	}
}

func usage() {
	fmt.Printf( "Usage: %v " +
		"[ actors (> 0 && > set && actors % set == 0 ) | 'd' (default %v ) ] " +
		"[ set (> 0) | 'd' (default %v) ] " +
		"[ rounds (> 0) | 'd' (default %v) ] " +
		"[ processors (> 0) | 'd' (default %v) ] " +
		"[ batch (> 0) | 'd' (default %v) ]\n",
		os.Args[0], Actors, Set, Rounds, Processors );
	os.Exit( 1 );
}

func main() {
	switch len( os.Args ) {
	  case 6:
		if os.Args[5] != "d" {							// default ?
			Batch, _ = strconv.Atoi( os.Args[5] )
			if Batch < 1 { usage(); }
		} // if
		fallthrough
	  case 5:
		if os.Args[4] != "d" {							// default ?
			Processors, _ = strconv.Atoi( os.Args[4] )
			if Processors < 1 { usage(); }
		} // if
		fallthrough
	  case 4:
		if os.Args[3] != "d" {							// default ?
			Rounds, _ = strconv.Atoi( os.Args[3] )
			if Rounds < 1 { usage(); }
		} // if
		fallthrough
	  case 3:
		if os.Args[2] != "d" {							// default ?
			Set, _ = strconv.Atoi( os.Args[2] )
			if Set < 1 { usage(); }
		} // if
		fallthrough
	  case 2:
		if os.Args[1] != "d" {							// default ?
			Actors, _ = strconv.Atoi( os.Args[1] )
			if Actors < 1 || Actors <= Set || Actors % Set != 0 { usage(); }
		} // if
	  case 1:											// use defaults
	  default:
		usage();
	} // switch

	runtime.GOMAXPROCS( Processors );
	system = actor.NewActorSystem();

	actors := make( [] *actor.PID, Actors, Actors );	// create actors
	for id := 0; id < Actors; id += 1 {
		props := actor.PropsFromProducer( func() actor.Actor {
			return &Executor{ actors, id, Set * Rounds, id / Set * Set, 0, 0 }
		} );
		actors[id] = system.Root.Spawn( props );
	} // for
	starttime = time.Now();
	for id := 0; id < Actors; id += 1 {					// start actors
		system.Root.Send( actors[id], &msg );
	} // for
	<- shake
} // main

// /usr/bin/time -f "%Uu %Ss %Er %Mkb" GoExecutor

// Local Variables: //
// tab-width: 4 //
// compile-command: "go build" //
// End: //
