package action

import "fmt"
import "net/http"
import "html/template"
import "signaling/src/framework"

type xrtcClientPushAction struct{
	
}

func NewXrtcClientPushAction() *xrtcClientPushAction {
	return &xrtcClientPushAction{}
}

func writeHtmlErrorResonse(w http.ResponseWriter, code int, message string) {
	w.WriteHeader(code)
	w.Write([]byte(message))
}

func (*xrtcClientPushAction) Execute(w http.ResponseWriter, cr *framework.ComRequest) {
	r :=cr.R

	t,err := template.ParseFiles("./static/template/push.tpl")
	if err != nil {
		fmt.Println(err)
		writeHtmlErrorResonse(w, http.StatusNotFound, "404 Not Found")
		return
	}

	request := make(map[string]string)

	for k, v := range r.Form{
		request[k] = v[0]
	}

	err = t.Execute(w, request)
	if err != nil {
		fmt.Println(err)
		writeHtmlErrorResonse(w, http.StatusNotFound, "404 Not Found")
		return
	}
}