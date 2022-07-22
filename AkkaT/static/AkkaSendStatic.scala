import akka.actor.typed.scaladsl.Behaviors
import akka.actor.typed.scaladsl.LoggerOps
import akka.actor.typed.{ ActorRef, ActorSystem, Behavior }
import java.util.concurrent.atomic.AtomicInteger
import akka.event.Logging
import akka.actor.typed.DispatcherSelector

object StaticActor {
	final case class Dummy( cnt: Int )

	def apply(): Behavior[Dummy] = Behaviors.receive { (context, message) =>
		if ( message.cnt >= StaticMain.times ) {
			if ( StaticMain.trials.get() > 0 ) {		// ignore trial 1
				println( s"Akka Send Static ${StaticMain.times} ${(System.nanoTime() - StaticMain.startTime) / StaticMain.times}ns" )
			} // if
			if ( StaticMain.trials.incrementAndGet() == 5 + 1 ) {
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

	val times = 100_000_000

	val trials = new AtomicInteger(0)
	var startTime = System.nanoTime()

	var system : ActorSystem[MainMessages] = null

	def apply(): Behavior[MainMessages] = Behaviors.setup { context =>
		val actor = context.spawn(StaticActor(), "actor", DispatcherSelector.fromConfig("akka.dispatcher"))

      	Behaviors.receiveMessage { message =>
			message match {
				case Start() =>
					actor ! StaticActor.Dummy( 0 )					
					Behaviors.same
				case Stop() =>
					Behaviors.stopped
			}
      	}
	}

	def main(args: Array[String]): Unit = {
		system = ActorSystem( StaticMain(), "Static" )
		system ! Start()
	}
}

// Local Variables: //
// tab-width: 4 //
// mode: java //
// compile-command: "sbt --warn -J-Xmx32g \"run\"" //
// End: //
