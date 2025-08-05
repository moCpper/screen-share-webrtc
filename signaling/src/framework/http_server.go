package framework

import "fmt"
import "net/http"

func init(){
	http.HandleFunc("/",entry)
}

type ActionInterface interface{
	Execute(w http.ResponseWriter, r *http.Request)
}

var GActionRouter map[string]ActionInterface = make(map[string]ActionInterface)

func responseError(w http.ResponseWriter,r *http.Request, code int, message string) {
	w.WriteHeader(code)
	w.Write([]byte(fmt.Sprintf("%d - %s", code, message)))
}

func entry(w http.ResponseWriter, r *http.Request) {
	fmt.Println("=============",r.URL.Path)

	if action, ok := GActionRouter[r.URL.Path]; ok {
		r.ParseForm()
		if action != nil{
			action.Execute(w, r)
		} else {
			responseError(w, r, http.StatusInternalServerError, "500 Internal Server Error")
		}
	} else {
		responseError(w,r,http.StatusNotFound, "404 Not Found")
	}
}

func StartHttp() error{
	fmt.Println("start http")
	return http.ListenAndServe(":8080", nil)
}