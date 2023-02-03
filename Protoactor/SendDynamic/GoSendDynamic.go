// https://github.com/asynkron/protoactor-go
// https://pkg.go.dev/github.com/asynkron/protoactor-go/actor
package main

import (
	"os"; "strconv"; "fmt"; "time"; "runtime"
	"github.com/asynkron/protoactor-go/actor"
)

var Times int = 10_000_000;
var start time.Time;
var shake = make( chan string )
var system * actor.ActorSystem

type Msg struct { cnt int }
type Send struct{}

func ( state * Send ) Receive( context actor.Context ) {
	switch msg := context.Message().(type) {
	  case * Msg:
		if msg.cnt >= Times  {
			fmt.Printf( "%v %vns\n", Times, int64(time.Since(start) / time.Duration(Times) / time.Nanosecond) );
			shake <- "hand"
			return;
		} // if
		//fmt.Printf( "Send %v\n", msg.cnt );
		props := actor.PropsFromProducer( func() actor.Actor { return &Send{} } )
		pid := system.Root.Spawn( props )
		system.Root.Send( pid, &Msg{ msg.cnt + 1 } );
		system.Root.Stop( context.Self() );
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
	props := actor.PropsFromProducer( func() actor.Actor { return &Send{} } );
	pid := system.Root.Spawn(props);
	start = time.Now();
	system.Root.Send( pid, &Msg{ 0 } );
	<- shake
}

// /usr/bin/time -f "%Uu %Ss %Er %Mkb" GoSendDynamic

// Local Variables: //
// tab-width: 4 //
// compile-command: "go build" //
// End: //
