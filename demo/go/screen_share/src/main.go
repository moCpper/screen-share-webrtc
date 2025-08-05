package main

import "net/http"
import "fmt"

func startHttp(port string){
	fmt.Println("Starting HTTP server on port", port)
	err := http.ListenAndServe(port, nil)
	if err != nil {
		fmt.Println("Error starting server:", err)
	}
}

func startHttps(port,cert,key string){
	fmt.Println("Starting HTTPS server on port", port)
	err := http.ListenAndServeTLS(port, cert, key, nil)
	if err != nil {
		fmt.Println("Error starting server:", err)
	}
}

func main(){
	// 定义一个url前缀
	staticUrl := "/static/"

	// 定义一个fileserver
	fs := http.FileServer(http.Dir("./static"))

	// 绑定url和fileserver
	http.Handle(staticUrl, http.StripPrefix(staticUrl,fs))

	// 启动
	go startHttp(":8080")		// go协程

	startHttps(":8081", "./conf/ca.crt", "./conf/ca.key")
}
