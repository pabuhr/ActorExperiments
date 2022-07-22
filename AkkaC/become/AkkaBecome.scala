import akka.actor.{ Actor, ActorSystem, Props }
import java.util.concurrent.atomic.AtomicInteger

class Become( system : ActorSystem, times : Int, var startTime : Long ) extends Actor {
	def receive = {
		case cnt : Int =>
			if ( cnt >= times ) {
				if ( Become.trials.get() > 0 ) {		// ignore trial 1
					println( s"Akka Become ${times} ${(System.nanoTime() - startTime) / times}ns" )
				} // if
				if ( Become.trials.incrementAndGet() == 5 + 1 ) {
					system.terminate();
				} else {
					startTime = System.nanoTime()		// reset for next trial
					self ! 0
				} // if
			} else {
				//println( "receiver " + cnt );
				self ! cnt + 1							// send to self
				context.become( receive2 );				// alternate message receive
			} // if
	} // receive

	def receive2 : Receive = {							// mutually recursive references
		case cnt : Int =>
			//println( "receiver2 " + cnt );
			self ! cnt + 1								// send to self
			context.become( receive );					// alternate message receive
	} // receive2
}

object Become {
	val trials = new AtomicInteger(0)

	def props( system : ActorSystem, times : Int, startTime : Long ) : Props
		= Props( new Become( system, times, startTime ) )
} // Become

object Main extends App {
	def usage() = {
		println( "Usage: [ times (> 0) ]" );
		System.exit( 0 );
	}
	var times = 100_000_000
	args.length match {
	  case 1 =>
		times = args(0).toInt
		if ( times < 1 ) usage()
	  case 0 =>
	  case _ =>
		usage()
	} // match

	val system = ActorSystem( "Become" );
	system.actorOf( Become.props( system, times, System.nanoTime() ).withDispatcher("akka.dispatcher"), "Become0" ) ! 0
}

// Local Variables: //
// tab-width: 4 //
// mode: java //
// compile-command: "sbt --warn -J-Xmx32g \"run\"" //
// End: //
