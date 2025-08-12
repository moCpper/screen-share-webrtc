package action

import (
	"fmt"
	"html/template"
	"net/http"

	"signaling/src/framework"
)

type xrtcClientPushAction struct{}

func NewXrtcClientPushAction() *xrtcClientPushAction {
	return &xrtcClientPushAction{}
}

func writeHtmlErrorResponse(w http.ResponseWriter, status int, err string) {
	w.WriteHeader(status)
	w.Write([]byte(err))
}

func (*xrtcClientPushAction) Execute(w http.ResponseWriter, cr *framework.ComRequest) {
	r := cr.R

	t, err := template.ParseFiles(framework.GetStaticDir() + "/template/push.tpl")
	if err != nil {
		fmt.Println(err)
		writeHtmlErrorResponse(w, http.StatusNotFound, "404 - Not found")
		return
	}

	request := make(map[string]string)

	for k, v := range r.Form {
		request[k] = v[0]
	}

	err = t.Execute(w, request)
	if err != nil {
		fmt.Println(err)
		writeHtmlErrorResponse(w, http.StatusNotFound, "404 - Not found")
		return
	}
}
