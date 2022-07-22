import akka.actor.typed.scaladsl.Behaviors
import akka.actor.typed.scaladsl.LoggerOps
import akka.actor.typed.{ ActorRef, ActorSystem, Behavior }
import java.util.concurrent.atomic.AtomicInteger
import akka.event.Logging
import akka.actor.typed.DispatcherSelector

object DynamicActor {
	final case class Dummy( cnt: Int )

	def apply(): Behavior[Dummy] = Behaviors.receive { (context, message) =>
		if ( message.cnt >= DynamicMain.times ) {
			if ( DynamicMain.trials.get() > 0 ) {		// ignore trial 1
				println( s"Akka Send Dynamic ${DynamicMain.times} ${(System.nanoTime() - DynamicMain.startTime) / DynamicMain.times}ns" )
			} // if
			if ( DynamicMain.trials.incrementAndGet() == 5 + 1 ) {
				DynamicMain.system ! DynamicMain.Stop()
			} else {
				DynamicMain.startTime = System.nanoTime()		// reset for next trial
				context.self ! Dummy(0)
			} // if
			Behaviors.same
		} else {
			DynamicMain.system ! DynamicMain.Start( message.cnt + 1 )
			Behaviors.stopped
		}
	}
}

object DynamicMain {
	sealed trait MainMessages
	final case class Start( cnt: Int ) extends MainMessages
	final case class Stop() extends MainMessages

	val times = 10_000_000
	var actor_cnt = 0;

	val trials = new AtomicInteger(0)
	var startTime = System.nanoTime()

	var system : ActorSystem[DynamicMain.MainMessages] = null

	def apply(): Behavior[MainMessages] = Behaviors.setup { context =>
      	Behaviors.receiveMessage { message =>
			message match {
				case Start( cnt: Int ) =>
					val actor = context.spawn(DynamicActor(), "actor_" + actor_cnt , DispatcherSelector.fromConfig("akka.dispatcher"))
					actor_cnt += 1
					actor ! DynamicActor.Dummy( cnt )					
					Behaviors.same
				case Stop() =>
					Behaviors.stopped
			}
      	}
    }

	def main(args: Array[String]): Unit = {
		system = ActorSystem( DynamicMain(), "Dynamic" )
		system ! Start( 0 )
	}
}

// Local Variables: //
// tab-width: 4 //
// mode: java //
// compile-command: "sbt --warn -J-Xmx32g \"run\"" //
// End: //
