package framework

import(
	"signaling/src/glog"
)

func Init() error { 
	glog.SetLogDir("./log")
	glog.SetLogFileName("signaling")
	glog.SetLogLevel("DEBUG")
	glog.SetLogToStderr(true)
	return nil
}