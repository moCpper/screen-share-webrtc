package framework

import(
	"signaling/src/glog"
)

func Init() error { 
	glog.SetLogDir("./log")
	glog.SetLogFileName("signaling")
	return nil
}