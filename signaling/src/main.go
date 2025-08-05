package main

import (
	"flag"
	"signaling/src/framework"
	"signaling/src/glog"
)

func main(){
	//Init the command-line flags.
	flag.Parse()

	err := framework.Init()
	if err != nil {
		panic(err)
	}

	glog.Info("hello")
	glog.Flush()

	err = framework.StartHttp()
	if err != nil {
		panic(err)
	}
}