name := "dummy"
version := "0.0"
scalaVersion := "2.13.1"
lazy val akkaVersion = "2.6.14"

libraryDependencies ++= Seq(
	"com.typesafe.akka" %% "akka-actor-typed" % akkaVersion
)
libraryDependencies += "ch.qos.logback" % "logback-classic" % "1.1.3" % Runtime
Compile / unmanagedResourceDirectories += baseDirectory.value


