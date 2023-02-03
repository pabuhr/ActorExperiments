import akka.actor.{ Actor, ActorSystem, Props, PoisonPill }
import java.util.concurrent.atomic.AtomicInteger
import java.util.concurrent.Semaphore

class Send( system : ActorSystem, times : Int, var startTime : Long ) extends Actor {
	def receive = {
		case cnt : Int =>
			if ( cnt >= times ) {
				if ( Send.trials.get() > 0 ) {			// ignore trial 1
					println( s"${times} ${(System.nanoTime() - startTime) / times}ns" )
				} // if
				if ( Send.trials.incrementAndGet() == 11 + 1 ) {
                    Main.sem.release()
					system.terminate();
				} else {
					startTime = System.nanoTime()		// reset for next trial
					self ! 0
				} // if
			} else {
				//println( cnt );
				val next = system.actorOf( Send.props( system, times, startTime ).withDispatcher("akka.dispatcher") )
				next ! cnt + 1
				self ! PoisonPill						// terminate actor
			} // if
	} // receive
} // Send

object Send {
	val trials = new AtomicInteger(0)

	def props( system : ActorSystem, times : Int, startTime : Long ) : Props
		= Props( new Send( system, times, startTime ) )
} // Send

object Main extends App {
    val sem = new Semaphore(0)

	var times = 10_000_000

	def usage() = {
		println( "Usage: [ times (> 0) ]" );
		System.exit( 0 );
	}

	args.length match {
	  case 1 =>
		if ( args(0) != "d" ) { times = args(0).toInt }
		if ( times < 1 ) usage()
	  case 0 =>
	  case _ =>
		usage()
	} // match

	val system = ActorSystem( "Send" );
	system.actorOf( Send.props( system, times, System.nanoTime() ).withDispatcher("akka.dispatcher") ) ! 0
    sem.acquire()
}

// Local Variables: //
// tab-width: 4 //
// mode: java //
// compile-command: "sbt --warn -J-Xmx32g \"run\"" //
// End: //
