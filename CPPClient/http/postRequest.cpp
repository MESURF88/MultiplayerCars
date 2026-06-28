#include "postRequest.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#if defined(_WIN32)
#include <windows.h>
#else
#include <limits.h>
#include <unistd.h>
#endif
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

namespace
{
    struct JsonLocation
    {
        std::size_t line;
        std::size_t column;
    };

    JsonLocation locationFromByte(const std::string& text, std::size_t byte)
    {
        JsonLocation location{ 1, 1 };
        if (byte == 0)
        {
            return location;
        }

        const std::size_t target = byte - 1;
        for (std::size_t i = 0; i < text.size() && i < target; ++i)
        {
            if (text[i] == '\n')
            {
                ++location.line;
                location.column = 1;
            }
            else
            {
                ++location.column;
            }
        }
        return location;
    }

    std::string lineTextAt(const std::string& text, std::size_t targetLine)
    {
        std::istringstream stream(text);
        std::string line;
        for (std::size_t lineNumber = 1; std::getline(stream, line); ++lineNumber)
        {
            if (lineNumber == targetLine)
            {
                return line;
            }
        }
        return "";
    }

    void logJsonParseError(const std::string& sourceName, const std::string& jsonText, const nlohmann::json::parse_error& error)
    {
        const JsonLocation location = locationFromByte(jsonText, error.byte);
        std::cout << "JSON parse error in " << sourceName << std::endl;
        std::cout << "  line: " << location.line << ", column: " << location.column << ", byte: " << error.byte << std::endl;
        std::cout << "  details: " << error.what() << std::endl;

        const std::string line = lineTextAt(jsonText, location.line);
        if (!line.empty())
        {
            std::cout << "  text: " << line << std::endl;
            std::cout << "        ";
            for (std::size_t i = 1; i < location.column; ++i)
            {
                std::cout << ' ';
            }
            std::cout << '^' << std::endl;
        }
    }

    bool canReadFile(const std::string& filePath)
    {
        std::ifstream input(filePath);
        return input.good();
    }

    std::string executableDirectory()
    {
#if defined(_WIN32)
        char path[MAX_PATH];
        DWORD length = GetModuleFileNameA(NULL, path, MAX_PATH);
        if (length == 0 || length == MAX_PATH)
        {
            return "";
        }
        std::string executablePath(path, length);
        const std::size_t separator = executablePath.find_last_of("\\/");
#else
        char path[PATH_MAX];
        ssize_t length = readlink("/proc/self/exe", path, sizeof(path) - 1);
        if (length <= 0)
        {
            return "";
        }
        path[length] = '\0';
        std::string executablePath(path);
        const std::size_t separator = executablePath.find_last_of('/');
#endif
        if (separator == std::string::npos)
        {
            return "";
        }
        return executablePath.substr(0, separator);
    }

    std::string findEnvPath()
    {
        const std::string exeDir = executableDirectory();
        std::vector<std::string> candidates = {
            ".env",
            "CPPClient/.env"
        };

        if (!exeDir.empty())
        {
#if defined(_WIN32)
            candidates.push_back(exeDir + "\\.env");
            candidates.push_back(exeDir + "\\..\\.env");
#else
            candidates.push_back(exeDir + "/.env");
            candidates.push_back(exeDir + "/../.env");
#endif
        }

        for (const std::string& candidate : candidates)
        {
            if (canReadFile(candidate))
            {
                return candidate;
            }
        }
        return "";
    }

    std::string readFileToString(const std::string& filePath)
    {
        std::ifstream input(filePath);
        if (!input.is_open())
        {
            std::cout << "error: could not open " << filePath << std::endl;
            return "";
        }

        std::ostringstream buffer;
        buffer << input.rdbuf();
        return buffer.str();
    }
}

std::string PostRequestPassword(std::string url, int &statusCode)
{
    std::string otpCapture;
    nlohmann::json authJson;

    const std::string envPath = findEnvPath();
    if (envPath.empty())
    {
        std::cout << "error: could not find .env. Checked .env, CPPClient/.env, and the executable directory." << std::endl;
        statusCode = 0;
        return otpCapture;
    }

    std::cout << "Using auth JSON from " << envPath << std::endl;
    const std::string envAuthJson = readFileToString(envPath);
    if (envAuthJson.empty())
    {
        statusCode = 0;
        return otpCapture;
    }

    try
    {
        authJson = nlohmann::json::parse(envAuthJson);
    }
    catch (const nlohmann::json::parse_error& e)
    {
        logJsonParseError(envPath, envAuthJson, e);
        statusCode = 0;
        return otpCapture;
    }
    catch (const nlohmann::json::exception& e)
    {
        std::cout << "JSON error in " << envPath << ": " << e.what() << std::endl;
        statusCode = 0;
        return otpCapture;
    }

    std::cout << "POST " << url << std::endl;
    cpr::Response r = cpr::Post(cpr::Url{url},
                      cpr::Body{authJson.dump()}, cpr::Header{{"Content-Type", "application/json"}}, cpr::VerifySsl(false));
    statusCode = r.status_code;
    try
    {
        if (r.status_code != 0)
        {
            nlohmann::json otpJson = nlohmann::json::parse(r.text);
            otpCapture = otpJson.at("otp");
        }
    }
    catch (const nlohmann::json::parse_error& e)
    {
      logJsonParseError("login response body", r.text, e);
      std::cout << "login response status code: " << r.status_code << std::endl;
      std::cout << "login response body: " << r.text << std::endl;
    }
    catch (const nlohmann::json::exception& e)
    {
      std::cout << "JSON error in login response body: " << e.what() << std::endl;
      std::cout << "login response status code: " << r.status_code << std::endl;
      std::cout << "login response body: " << r.text << std::endl;
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
