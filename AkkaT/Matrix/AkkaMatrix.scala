import akka.actor.typed.scaladsl.Behaviors
import akka.actor.typed.scaladsl.LoggerOps
import akka.actor.typed.{ ActorRef, ActorSystem, Behavior }
import java.util.concurrent.atomic.AtomicInteger
import akka.event.Logging
import akka.actor.typed.DispatcherSelector
import java.util.concurrent.Semaphore

object MatrixMult {
	final case class WorkMsg( var Z: Array[Int], val X: Array[Int], val Y: Array[Array[Int]] )

	def apply(): Behavior[WorkMsg] = Behaviors.receive { (context, message) =>
		// message match {
		// 	case WorkMsg( Z, X, Y )  =>
		for ( i <- 0 until MatrixMain.yc ) {
			message.Z(i) = 0
			for ( j <- 0 until MatrixMain.xc ) {
				message.Z(i) = message.Z(i) + message.X(j) * message.Y(j)(i)
			}
		}
		if ( MatrixMain.actorCnt.incrementAndGet() == MatrixMain.xr ) {
			MatrixMain.system ! MatrixMain.Stop()
		}
		// } // message
		Behaviors.same
	}
}

object MatrixMain {
	sealed trait MainMessages
	final case class Start() extends MainMessages
	final case class Stop() extends MainMessages

	var xr = 3_072; var xc = 3_072; var yc = 3_072; var Processors = 1 // default values

	val actors = new Array[ActorRef[MatrixMult.WorkMsg]](xr)
	val messages = new Array[MatrixMult.WorkMsg](xr);

	val actorCnt = new AtomicInteger(0)
	val sem = new Semaphore(0)

	var system : ActorSystem[MatrixMain.MainMessages] = null
	var startTime = System.nanoTime()

	var X = Array.ofDim[Int](0, 0)						// set type, set size below
	var Y = Array.ofDim[Int](0, 0)
	var Z = Array.ofDim[Int](0, 0)

	def apply(): Behavior[MainMessages] = Behaviors.setup { context =>
		for ( id <- 0 until xr ) {						// create actors
			actors(id) = context.spawn(MatrixMult(), "actor_" + id, DispatcherSelector.fromConfig("akka.dispatcher"))
			messages(id) = new MatrixMult.WorkMsg(Z(id), X(id), Y)
		} // for

	  	Behaviors.receiveMessage { message =>
			message match {
				case Start() =>
					for ( id <- 0 until xr ) {			// start actors
						actors(id) ! messages(id)
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
			s"[ yc (> 0) | 'd' (default ${xr}) ] " +
			s"[ xc (> 0) | 'd' (default ${xc}) ] " +
			s"[ xr (> 0) | 'd' (default ${yc}) ] " +
			s"[ processors (> 0) | 'd' (default ${Processors}) ]"
		)
		System.exit( 1 )
	}

	def main(args: Array[String]): Unit = {
		if ( args.length > 4 ) usage()					// process command-line arguments
		if ( args.length == 4 ) {
			if ( args(3) != "d" ) {						// default ?
				Processors = args(3).toInt
				if ( Processors < 1 ) usage()
			} // if
		} // if
		if ( args.length >= 3 ) {						// fall through
			if ( args(2) != "d" ) {						// default ?
				xr = args(2).toInt
				if ( xr < 1 ) usage()
			} // if
		} // if
		if ( args.length >= 2 ) {						// fall through
			if ( args(1) != "d" ) {						// default ?
				xc = args(1).toInt
				if ( xc < 1 ) usage()
			} // if
		} // if
		if ( args.length >= 1 ) {						// fall through
			if ( args(0) != "d" ) {						// default ?
				yc = args(0).toInt
				if ( yc < 1 ) usage()
			} // if
		} // if

		X = Array.ofDim[Int](xr, xc)
		Y = Array.ofDim[Int](xc, yc)
		Z = Array.ofDim[Int](xr, yc)

		for ( r <- 0 until xr ) {
			for ( c <- 0 until xc ) {
				X(r)(c) = r * c % 37
			}
		}

		for ( r <- 0 until xc ) {
			for ( c <- 0 until yc ) {
				Y(r)(c) = r * c % 37
			}
		}

		startTime = System.nanoTime()
		system = ActorSystem( MatrixMain(), "Matrix" )
		system ! Start()

		sem.acquire()

		println( s"${Processors} " + f"${(System.nanoTime() - startTime) / 1_000_000_000.0}%1.2f" + "s" )
	}
}

// Local Variables: //
// tab-width: 4 //
// mode: java //
// compile-command: "sbt --warn -J-Xmx32g \"run\"" //
// End: //
