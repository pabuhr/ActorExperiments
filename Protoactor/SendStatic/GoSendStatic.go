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
var system * actor.ActorSystem

type Msg struct { cnt int }
var msg Msg
type Send struct{}

func ( state * Send ) Receive( context actor.Context ) {
	switch context.Message().(type) {
	  case * Msg:
		if msg.cnt >= Times  {
			fmt.Printf( "Protoactor Send Static %v %vns\n", Times, int64(time.Since(start) / time.Duration(Times) / time.Nanosecond) );
			shake <- "hand"
			return;
		} // if
		//fmt.Printf( "Send %v\n", msg.cnt );
		msg.cnt += 1;
		system.Root.Send( context.Self(), &msg );
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
		Times, _ = strconv.Atoi( os.Args[1] )
		if Times < 1 { usage(); }
	  case 1:											// use defaults
	  default:
		usage();
	} // switch

	runtime.GOMAXPROCS(1);
	system = actor.NewActorSystem();
	props := actor.PropsFromProducer( func() actor.Actor { return &Send{} } );
	pid := system.Root.Spawn(props);
	start = time.Now();
	system.Root.Send( pid, &msg );
	<- shake
}

// /usr/bin/time -f "%Uu %Ss %Er %Mkb" GoSendStatic

// Local Variables: //
// tab-width: 4 //
// compile-command: "go build" //
// End: //
