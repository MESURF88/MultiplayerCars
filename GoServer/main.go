package main
 
import (
	"os"
    "fmt"
    "log"
    "net/http"
	"context"

)

 
func setupAPI(ctx context.Context) {
	// Create a Manager instance used to handle WebSocket Connections
	manager := NewManager(ctx)

    //http.Handle("/", http.FileServer(http.Dir("./frontend")))
	http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		fmt.Println("Home")
	})
	http.HandleFunc("/login", manager.loginHandler)
	http.HandleFunc("/ws", manager.serveWS)

	http.HandleFunc("/debug", func(w http.ResponseWriter, r *http.Request) {
		fmt.Println("Debug")
		fmt.Fprint(w, len(manager.clients))
	})
}
 
func main() {
	// Create a root ctx and a CancelFunc which can be used to cancel retentionMap goroutine
	rootCtx := context.Background()
	ctx, cancel := context.WithCancel(rootCtx)

	defer cancel()

	setupAPI(ctx)
	port := os.Getenv("PORT")
    if port == "" {
        port = "3000"
    }
	fmt.Println("Listening on port " + port)
    log.Fatal(http.ListenAndServeTLS(":"+port, "keys/server.crt", "keys/server.key", nil))
	//log.Fatal(http.ListenAndServe(":" + port, nil))
}