import akka.actor.typed.scaladsl.Behaviors
import akka.actor.typed.scaladsl.LoggerOps
import akka.actor.typed.{ ActorRef, ActorSystem, Behavior }
import java.util.concurrent.atomic.AtomicInteger
import akka.event.Logging
import akka.actor.typed.DispatcherSelector
import java.util.concurrent.Semaphore

object StaticActor {
	final case class Dummy( cnt: Int )

	def apply(): Behavior[Dummy] = Behaviors.receive { (context, message) =>
		if ( message.cnt >= StaticMain.times ) {
			if ( StaticMain.trials.get() > 0 ) {		// ignore trial 1
				println( s"${StaticMain.times} ${(System.nanoTime() - StaticMain.startTime) / StaticMain.times}ns" )
			} // if
			if ( StaticMain.trials.incrementAndGet() == 11 + 1 ) {
				StaticMain.system ! StaticMain.Stop()
			} else {
				StaticMain.startTime = System.nanoTime() // reset for next trial
				context.self ! Dummy( 0 )
			} // if
		} else {
			context.self ! Dummy( message.cnt + 1 )		// send to self
		} // if
		Behaviors.same
	}
}

object StaticMain {
	sealed trait MainMessages
	final case class Start() extends MainMessages
	final case class Stop() extends MainMessages

	val trials = new AtomicInteger(0)
    val sem = new Semaphore(0)

	var system : ActorSystem[MainMessages] = null
	var startTime = System.nanoTime()
	var times = 100_000_000

	def apply(): Behavior[MainMessages] = Behaviors.setup { context =>
		val actor = context.spawn(StaticActor(), "actor", DispatcherSelector.fromConfig("akka.dispatcher"))

      	Behaviors.receiveMessage { message =>
			message match {
				case Start() =>
					actor ! StaticActor.Dummy( 0 )					
					Behaviors.same
				case Stop() =>
                    sem.release()
					Behaviors.stopped
			}
      	}
	}

	def main(args: Array[String]): Unit = {
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

		system = ActorSystem( StaticMain(), "Static" )
		system ! Start()
        sem.acquire()
	}
}

// Local Variables: //
// tab-width: 4 //
// mode: java //
// compile-command: "sbt --warn -J-Xmx32g \"run\"" //
// End: //
