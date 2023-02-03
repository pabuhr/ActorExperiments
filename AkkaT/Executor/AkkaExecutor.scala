import akka.actor.typed.scaladsl.Behaviors
import akka.actor.typed.scaladsl.LoggerOps
import akka.actor.typed.{ ActorRef, ActorSystem, Behavior }
import java.util.concurrent.atomic.AtomicInteger
import akka.event.Logging
import akka.actor.typed.DispatcherSelector
import java.util.concurrent.Semaphore

object ExecutorActor {
	sealed trait MessageTypes
	final case class Dummy( id: Int ) extends MessageTypes
	final case class StartMsg( id: Int ) extends MessageTypes

	val rounds = ExecutorMain.Set * ExecutorMain.Rounds

	def apply(): Behavior[MessageTypes] = Behaviors.receive { (context, message) =>
		message match {
			case StartMsg( id ) =>
				ExecutorMain.groups(id) = id / ExecutorMain.Set * ExecutorMain.Set
				if (id == ExecutorMain.Actors - 1) {
					ExecutorMain.system ! ExecutorMain.Continue()
				} // if
			case Dummy( id ) =>
				if ( ExecutorMain.recs(id) >= rounds ) {
					if ( ExecutorMain.actorCnt.incrementAndGet() == ExecutorMain.Actors ) {
						if ( ExecutorMain.trials.get() > 0 ) { // ignore trial 1
							println( s"${ExecutorMain.Processors}" + " " + f"${(System.nanoTime() - ExecutorMain.startTime) / 1_000_000_000.0}%1.2f" + "s" )
						} // if
						if ( ExecutorMain.trials.incrementAndGet() == 11 + 1 ) {
							ExecutorMain.system ! ExecutorMain.Stop()
						} else {
							ExecutorMain.actorCnt.set( 0 )
							ExecutorMain.startTime = System.nanoTime()
							for ( r <- 0 until ExecutorMain.Actors ) { // start actors again
								ExecutorMain.actors(r) ! Dummy( r )
							} // for
						} // if
					} // if
					ExecutorMain.recs(id) = 0
					ExecutorMain.sends(id) = 0			// reset for next trial
				} else {
					if ( ExecutorMain.recs(id) % ExecutorMain.Batch == 0 ) {
						for ( r <- 0 until ExecutorMain.Batch ) {  // start actors again
							val sendId = ExecutorMain.groups(id) + ExecutorMain.sends(id) % ExecutorMain.Set
							ExecutorMain.actors( sendId ) ! Dummy( sendId ) // cycle through group
							ExecutorMain.sends(id) += 1
						} // for
					} // if
					ExecutorMain.recs(id) += 1
				} // if
		} // message
		Behaviors.same
	}
}

object ExecutorMain {
	sealed trait MainMessages
	final case class Start() extends MainMessages
	final case class Stop() extends MainMessages
	final case class Continue() extends MainMessages

	var Actors = 40_000; var Set = 100; var Rounds = 100; var Processors = 1; var Batch = 1 // default values

	val actors = new Array[ActorRef[ExecutorActor.MessageTypes]](Actors)
	val recs = new Array[Int](Actors)
	val sends = new Array[Int](Actors)
	val groups = new Array[Int](Actors)

	val actorCnt = new AtomicInteger(0)
	val trials = new AtomicInteger(0)
    val sem = new Semaphore(0)

	var system : ActorSystem[ExecutorMain.MainMessages] = null
	var startTime = System.nanoTime()

	def apply(): Behavior[MainMessages] = Behaviors.setup { context =>
		for ( id <- 0 until Actors ) {						// create actors
			recs(id) = 0
			sends(id) = 0
			actors(id) = context.spawn(ExecutorActor(), "actor_" + id, DispatcherSelector.fromConfig("akka.dispatcher"))
		} // for

      	Behaviors.receiveMessage { message =>
			message match {
				case Start() =>
					for ( id <- 0 until Actors ) {						// start actors
						actors(id) ! ExecutorActor.StartMsg( id )
					} // for
					Behaviors.same
				case Continue() =>
					for ( id <- 0 until Actors ) {						// start actors
						actors(id) ! ExecutorActor.Dummy( id )
					} // for
					Behaviors.same
				case Stop() =>
                    sem.release()
					Behaviors.stopped
			} // match
      	}
    }

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

	def main(args: Array[String]): Unit = {
		if ( args.length > 5 ) usage()						// process command-line arguments
		if ( args.length == 5 ) {
			if ( args(4) != "d" ) {							// default ?
				Batch = args(4).toInt
				if ( Batch < 1 ) usage()
			} // if
		} // if
		if ( args.length >= 4 ) {
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

		system = ActorSystem( ExecutorMain(), "Executor" )
		system ! Start()
        sem.acquire()
	}
}

// Local Variables: //
// tab-width: 4 //
// mode: java //
// compile-command: "sbt --warn -J-Xmx32g \"run\"" //
// End: //
