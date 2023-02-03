import akka.actor.{ Actor, ActorSystem, Props, PoisonPill, ActorRef }
import java.util.concurrent.atomic.AtomicInteger
import java.util.concurrent.Semaphore

class WorkMsg( var Z: Array[Int], val X: Array[Int], val Y: Array[Array[Int]] ) {}

class MatrixMult( system : ActorSystem, xc : Int, yc : Int, xr: Int, sem: Semaphore ) extends Actor {
	def receive = {
	  case msg : WorkMsg =>
		for ( i <- 0 until yc ) {
            msg.Z(i) = 0
            for ( j <- 0 until xc ) {
                msg.Z(i) = msg.Z(i) + msg.X(j) * msg.Y(j)(i)
            }
        }
        if ( MatrixMult.actorCnt.incrementAndGet() == xr ) {
            sem.release()
        }
	} // receive
}

object MatrixMult {
	val actorCnt = new AtomicInteger(0)

	def props( system : ActorSystem, xc : Int, yc : Int, xr: Int, sem: Semaphore ) : Props
		= Props( new MatrixMult( system, xc, yc, xr, sem ) )
}

object Main extends App {
	var xr = 3_072; var xc = 3_072; var yc = 3_072; var Processors = 1 // default values

	def usage() = {
		println( "Usage: " +
			s"[ yc (> 0) | 'd' (default ${xr}) ] " +
			s"[ xc (> 0) | 'd' (default ${xc}) ] " +
			s"[ xr (> 0) | 'd' (default ${yc}) ] " +
			s"[ processors (> 0) | 'd' (default ${Processors}) ]"
		)
		System.exit( 1 )
	}

	if ( args.length > 4 ) usage()						// process command-line arguments
	if ( args.length == 4 ) {
		if ( args(3) != "d" ) {							// default ?
			Processors = args(3).toInt
			if ( Processors < 1 ) usage()
		} // if
	} // if
	if ( args.length >= 3 ) {							// fall through
		if ( args(2) != "d" ) {							// default ?
			xr = args(2).toInt
			if ( xr < 1 ) usage()
		} // if
	} // if
	if ( args.length >= 2 ) {							// fall through
		if ( args(1) != "d" ) {							// default ?
			xc = args(1).toInt
			if ( xc < 1 ) usage()
		} // if
	} // if
	if ( args.length >= 1 ) {							// fall through
		if ( args(0) != "d" ) {							// default ?
			yc = args(0).toInt
			if ( yc < 1 ) usage()
		} // if
	} // if

    val X = Array.ofDim[Int](xr, xc)
    val Y = Array.ofDim[Int](xc, yc)
    val Z = Array.ofDim[Int](xr, yc)

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
    
    val sem = new Semaphore(0)
	val system = ActorSystem( "MatrixMult" );

    var startTime = System.nanoTime()

	val actors = new Array[ActorRef](xr);
    val messages = new Array[WorkMsg](xr);
	for ( id <- 0 until xr ) {						// create actors
		actors(id) = system.actorOf(
            MatrixMult.props( system, xc, yc, xr, sem ).withDispatcher("akka.dispatcher"), s"MatrixMult${id}" )

        messages(id) = new WorkMsg(Z(id), X(id), Y)
	} // for
	for ( id <- 0 until xr ) {						// start actors
		actors(id) ! messages(id)
	} // for

    sem.acquire()

    println( s"${Processors} " + f"${(System.nanoTime() - startTime) / 1_000_000_000.0}%1.2f" + "s" )
    
}

// Local Variables: //
// tab-width: 4 //
// mode: java //
// compile-command: "sbt --warn -J-Xmx32g \"run\"" //
// End: //
