import akka.actor.{ Actor, ActorSystem, Props, PoisonPill, ActorRef }
import java.util.concurrent.atomic.AtomicInteger

class Executor( system : ActorSystem, actors : Array[ActorRef], Actors: Int, Set : Int, Rounds : Int, Batch : Int, id : Int ) extends Actor {
	val rounds = Set * Rounds
	val group = id / Set * Set
	var recs = 0
	var sends = 0

	def receive = {
	  case dummy : Int =>
		if ( recs == rounds ) {
			if ( Executor.actorCnt.incrementAndGet() == Actors ) {
				if ( Executor.trails.get() > 0 ) { 		// ignore trial 1
					println( "Akka Executor " + f"${(System.nanoTime() - Executor.startTime) / 1_000_000_000.0}%1.2f" + "s" )
				} // if
				if ( Executor.trails.incrementAndGet() == 5 + 1 ) {
					system.terminate()
				} else {
					Executor.actorCnt.set( 0 )
					Executor.startTime = System.nanoTime()
					for ( r <- 0 until Actors ) {		// start actors again
						actors(r) ! 0
					} // for
				} // if
			} // if
			recs = 0									// reset for next trial
			sends = 0
		} else {
			if ( recs % Batch == 0 ) {
				for ( r <- 0 until Batch ) {			// start actors again
					actors( group + sends % Set ) ! 0	// cycle through group
					sends += 1
				} // for
			} // if
			recs += 1
		} // if
	} // receive
}

object Executor {
	val actorCnt = new AtomicInteger(0)
	val trails = new AtomicInteger(0)
	var startTime = System.nanoTime()					// ignore trial 1 as initialized too early

	def props( system : ActorSystem, actors : Array[ActorRef], Actors : Int, Set : Int, Rounds : Int, Batch : Int, id : Int ) : Props
		= Props( new Executor( system, actors, Actors, Set, Rounds, Batch, id ) )
}

object Main extends App {
	var Actors = 40_000; var Set = 100; var Rounds = 100; var Processors = 1; var Batch = 1 // default values

	def usage() = {
		println( "Usage: " +
			s"[ actors (> 0 && > set && actors % set == 0 ) | 'd' (default ${Actors}) ] " +
			s"[ set (> 0) | 'd' (default ${Set}) ] " +
			s"[ rounds (> 0) | 'd' (default ${Rounds}) ] " +
			s"[ processors (> 0) | 'd' (default ${Processors}) ] " +
			s"[ batch (> 0) | 'd' (default ${Batch}) ]"
		)
		System.exit( 1 )
	}

	if ( args.length > 5 ) usage()						// process command-line arguments
	if ( args.length == 5 ) {
		if ( args(4) != "d" ) {							// default ?
			Batch = args(4).toInt
			if ( Batch < 1 ) usage()
		} // if
	} // if
	if ( args.length == 4 ) {
		if ( args(3) != "d" ) {							// default ?
			Processors = args(3).toInt
			if ( Processors < 1 ) usage()
		} // if
	} // if
	if ( args.length >= 3 ) {							// fall through
		if ( args(2) != "d" ) {							// default ?
			Rounds = args(2).toInt
			if ( Rounds < 1 ) usage()
		} // if
	} // if
	if ( args.length >= 2 ) {							// fall through
		if ( args(1) != "d" ) {							// default ?
			Set = args(1).toInt
			if ( Set < 1 ) usage()
		} // if
	} // if
	if ( args.length >= 1 ) {							// fall through
		if ( args(0) != "d" ) {							// default ?
			Actors = args(0).toInt
			if ( Actors < 1 || Actors <= Set || Actors % Set != 0 ) usage()
		} // if
	} // if

	//println( "Actors " + Actors + " Set " + Set + " Rounds " + Rounds + " Processors " + Processors + " Batch " + Batch )

	val system = ActorSystem( "Executor" );
	val actors = new Array[ActorRef](Actors);
	for ( id <- 0 until Actors ) {						// create actors
		actors(id) = system.actorOf(
			Executor.props( system, actors, Actors, Set, Rounds, Batch, id ).withDispatcher("akka.dispatcher"), s"Executor${id}" )
	} // for
	for ( id <- 0 until Actors ) {						// start actors
		actors(id) ! 0
	} // for
}

// Local Variables: //
// tab-width: 4 //
// mode: java //
// compile-command: "sbt --warn -J-Xmx32g \"run\"" //
// End: //
