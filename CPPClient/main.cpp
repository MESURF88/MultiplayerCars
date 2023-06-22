#ifndef _WIN32
#include "json.hpp" //nlohmann::json cannot build in MSVC
#endif
#include "raylib-cpp.hpp"
#include "postRequest.hpp"
#include "websocketConnect.hpp"
#include "windowContext.hpp"
#include <functional>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>

using namespace std;
std::mutex _lock;

//----------------------------------------------------------------------------------

std::condition_variable_any _cond;
bool connect_finish = false;

class connection_listener
{

public:
    // use maybe?

    void on_connected()
    {
        _lock.lock();
        _cond.notify_all();
        connect_finish = true;
        _lock.unlock();
    }
    void on_close()
    {
        std::cout<<"socket closed "<<std::endl;
        exit(0);
    }
    
    void on_fail()
    {
        std::cout<<"socket failed "<<std::endl;
        exit(0);
    }
};

//globals
string timestamp;

int main() {
    SetTargetFPS(60);
    //--------------------------------------------------------------------------------------

    

    //h.connect("http://127.0.0.1:3000");
    //h.connect("https://safe-depths-24899.herokuapp.com");
    //h.connect("http://safe-depths-24899.herokuapp.com");
    //h.connect("ws://127.0.0.1:3000/ws");
    //h.connect("wss://127.0.0.1:3000/debug" ); // testing http
    //login
    int statusCode;
    //get status code by reference
    std::string otp = PostRequestPassword("https://127.0.0.1:3000/login", statusCode);
    if ((statusCode == 200) && (otp != ""))
    {
        // boost websockets
        //wss://localhost:3000/ws
        WebsocketConn("127.0.0.1", "3000");
        
        //TODO: h.connect("wss://127.0.0.1:3000/ws", std::map<std::string,std::string>{{"otp", otp}});
        /*_lock.lock();
        if(!connect_finish)
        {
            _cond.wait(_lock);
        }
        _lock.unlock();*/


        // Main game loop
        while (windowShouldCloseWrapper()) {   // Detect window close button or ESC key
            // Update
            //----------------------------------------------------------------------------------
            // Update your variables here
            //----------------------------------------------------------------------------------

            // Draw
            //----------------------------------------------------------------------------------
            drawTextTestBox(timestamp);
            //----------------------------------------------------------------------------------

        }
    }
    else 
    {
        std::cout << "Error: couldn't login, http status code: " << statusCode << std::endl;
    }


    return 0;
}
