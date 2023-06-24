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

    void on_message(std::string msg)
    {
        parseMessage(msg);
    }

private:
    int parseMessage(const std::string& msg)
    {

    }
};

//globals
string timestamp;

int main() {
    SetTargetFPS(60);
    //--------------------------------------------------------------------------------------
    connection_listener listener;
    std::function<void(std::string)> stdf_message = [](std::string s) { std::cout << s << std::endl; }; //TODO: listener.on_message(s); };
    //login
    //std::string host = "127.0.0.1:3000"; 
    //std::string port = "3000";
    std::string host = "onlinecarsimgame-ab2533447e53.herokuapp.com";
    std::string port = "443"; //http

    int statusCode;
    //get status code by reference
    //std::string otp = PostRequestPassword("https://" + host + ":" + port + "/login", statusCode);
    std::string otp = PostRequestPassword("https://" + host + "/login", statusCode);
    if ((statusCode == 200) && (otp != ""))
    {
        // boost websockets
        //WebsocketConn("127.0.0.1", "3000", otp);
        WebsocketConn(host, "443", otp, stdf_message);
        
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
