// https://github.com/asynkron/protoactor-go
// https://pkg.go.dev/github.com/asynkron/protoactor-go/actor
package main

import (
	"os"; "strconv"; "fmt"; "time"; "runtime"
	"github.com/asynkron/protoactor-go/actor"
)

var Times int = 100_000_000;
var start time.Time;
var shake = make( chan string )

type Msg struct { cnt int }
var msg Msg
type Become struct { behavior actor.Behavior }
var system * actor.ActorSystem

func ( state * Become ) Receive( context actor.Context ) {
	state.behavior.Receive( context )
}

func ( state * Become ) Receive1( context actor.Context ) {
	switch context.Message().(type) {
	  case * Msg:
		if msg.cnt >= Times {
			fmt.Printf( "%v %vns\n", Times, int64(time.Since(start) / time.Duration(Times) / time.Nanosecond) );
			shake <- "hand"
			return;
		} // if
		//fmt.Printf( "Protoactor Become Receive1 %v %v\n", Times, msg.cnt );
		msg.cnt += 1;
		system.Root.Send( context.Self(), &msg );
		state.behavior.Become( state.Receive2 );
	  default:
		// ignore actor.Started message
	}
}

func ( state * Become ) Receive2( context actor.Context ) {
	switch context.Message().(type) {
	  case * Msg:
		//fmt.Printf( "Protoactor Become Receive2 %v %v\n", Times, msg.cnt );
		msg.cnt += 1;
		system.Root.Send( context.Self(), &msg );
		state.behavior.Become( state.Receive1 );
	  default:
		// ignore actor.Started message
	}
}

func usage() {
	fmt.Printf( "Usage: %v [ times (> 0) ]\n", os.Args[0] );
	os.Exit( 1 );
}

func main() {
	switch len( os.Args ) {
	  case 2:
		if os.Args[1] != "d" { Times, _ = strconv.Atoi( os.Args[1] ) }
		if Times < 1 { usage(); }
	  case 1:											// use defaults
	  default:
		usage();
	} // switch

	runtime.GOMAXPROCS(1);
	system = actor.NewActorSystem();
	props := actor.PropsFromProducer( func() actor.Actor {
			state := &Become {
				behavior: actor.NewBehavior(),
			}
			state.behavior.Become( state.Receive1 )
			return state
	} );
	pid := system.Root.Spawn(props);
	start = time.Now();
	system.Root.Send( pid, &msg );
	<- shake
}

// /usr/bin/time -f "%Uu %Ss %Er %Mkb" GoBecome

// Local Variables: //
// tab-width: 4 //
// compile-command: "go build" //
// End: //
