#include "postRequest.hpp"
#include <iostream>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

std::string PostRequestPassword(std::string url, int &statusCode)
{
    std::string otpCapture;
    nlohmann::json authJson;
    std::ifstream envAuthJson; 
    envAuthJson.open(".env");
    if ( envAuthJson.is_open() ) { // always check whether the file is open
      authJson = nlohmann::json::parse(envAuthJson);
    }
    envAuthJson.close();
    cpr::Response r = cpr::Post(cpr::Url{url},
                      cpr::Body{authJson.dump()}, cpr::Header{{"Content-Type", "application/json"}}, cpr::VerifySsl(false));
    statusCode = r.status_code;
    nlohmann::json otpJson = nlohmann::json::parse(r.text);
    try
    {
      otpCapture = otpJson.at("otp");
    }
    catch (std::out_of_range& e)
    {
      std::cout << "out of range: " << e.what() << std::endl;
    }
    if (statusCode != 200)
    {
      std::cout << "post status code != 200" << std::endl;
    }
    return otpCapture;
}