package framework

import "fmt"
import "net/http"
import "strconv"

func init(){
	http.HandleFunc("/",entry)
}

type ActionInterface interface{
	Execute(w http.ResponseWriter,cr *ComRequest)
}

var GActionRouter map[string]ActionInterface = make(map[string]ActionInterface)

type ComRequest struct{
	R *http.Request
	Logger *ComLog
	LogId uint32
}

func responseError(w http.ResponseWriter,r *http.Request, code int, message string) {
	w.WriteHeader(code)
	w.Write([]byte(fmt.Sprintf("%d - %s", code, message)))
}

func getRealClientIP(r *http.Request) string {
	ip := r.RemoteAddr

	if rip := r.Header.Get("X-Real-IP"); rip != "" {
		ip = rip
	}else if rip = r.Header.Get("X-Forwarded-IP"); rip != ""{
		ip = rip
	}

	return ip
}

func entry(w http.ResponseWriter, r *http.Request) {
	fmt.Println("=============",r.URL.Path)

	if action, ok := GActionRouter[r.URL.Path]; ok {
		if action != nil{
			cr := &ComRequest{
				R: r,
				Logger: &ComLog{},
				LogId: GetLogId32(),
			}

			cr.Logger.AddNotice("logId",strconv.Itoa(int(cr.LogId)))
			cr.Logger.AddNotice("url",r.URL.Path)
			cr.Logger.AddNotice("referer",r.Header.Get("Referer"))
			cr.Logger.AddNotice("ua",r.Header.Get("User-Agent"))
			cr.Logger.AddNotice("clientIP",r.RemoteAddr)
			cr.Logger.AddNotice("realClientIP",getRealClientIP(r))

			r.ParseForm()

			for k,v := range r.Form{
				cr.Logger.AddNotice(k,v[0])
			}

			cr.Logger.TimeBegin("totalCost")
			action.Execute(w, cr)
			cr.Logger.TimeEnd("totalCost")

			cr.Logger.Infof("")

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