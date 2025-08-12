package main

import (
	"signaling/src/action"
	"signaling/src/framework"
)

func init() {
	framework.GActionRouter["/signaling/push"] = action.NewPushAction()

	framework.GActionRouter["/xrtcclient/push"] = action.NewXrtcClientPushAction()
}
