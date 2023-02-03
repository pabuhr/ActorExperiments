import akka.actor.typed.scaladsl.Behaviors
import akka.actor.typed.scaladsl.LoggerOps
import akka.actor.typed.{ ActorRef, ActorSystem, Behavior }
import java.util.concurrent.atomic.AtomicInteger
import akka.event.Logging
import akka.actor.typed.DispatcherSelector
import java.util.concurrent.Semaphore

object DynamicActor {
	final case class Dummy( cnt: Int )

	def apply(): Behavior[Dummy] = Behaviors.receive { (context, message) =>
		if ( message.cnt >= DynamicMain.times ) {
			if ( DynamicMain.trials.get() > 0 ) {		// ignore trial 1
				println( s"${DynamicMain.times} ${(System.nanoTime() - DynamicMain.startTime) / DynamicMain.times}ns" )
			} // if
			if ( DynamicMain.trials.incrementAndGet() == 11 + 1 ) {
				DynamicMain.system ! DynamicMain.Stop()
			} else {
				DynamicMain.startTime = System.nanoTime() // reset for next trial
				context.self ! Dummy( 0 )
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

	val trials = new AtomicInteger(0)
    val sem = new Semaphore(0)

	var actor_cnt = 0;
	var system : ActorSystem[MainMessages] = null
	var startTime = System.nanoTime()
	var times = 10_000_000

	def apply(): Behavior[MainMessages] = Behaviors.setup { context =>
      	Behaviors.receiveMessage { message =>
			message match {
				case Start( cnt: Int ) =>
					val actor = context.spawn(DynamicActor(), "actor_" + actor_cnt , DispatcherSelector.fromConfig("akka.dispatcher"))
					actor_cnt += 1
					actor ! DynamicActor.Dummy( cnt )					
					Behaviors.same
				case Stop() =>
                    sem.release()
					Behaviors.stopped
			}
      	}
    }

	def main( args: Array[String] ): Unit = {
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

		system = ActorSystem( DynamicMain(), "Dynamic" )
		system ! Start( 0 )
        sem.acquire()
	}
}

// Local Variables: //
// tab-width: 4 //
// mode: java //
// compile-command: "sbt --warn -J-Xmx32g \"run\"" //
// End: //
