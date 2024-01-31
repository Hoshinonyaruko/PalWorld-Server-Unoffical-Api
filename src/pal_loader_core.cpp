#include "common_entry.h"
#include "spdlog/spdlog.h"
#include "sdk.hpp"

#include <cstdio>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <string>
#include <windows.h>

typedef void (*ForceGarbageCollectionType)(SDK::UEngine *engine, bool bForcePurge);

namespace net   = boost::asio;  // from <boost/asio.hpp>
namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http  = beast::http;  // from <boost/beast/http.hpp>
using tcp       = net::ip::tcp; // from <boost/asio/ip/tcp.hpp>


struct SDKContext {
        SDK::UEngine              *engine;
        SDK::UWorld               *world;
        SDK::UPalUtility          *utility;
        SDK::APalGameStateInGame  *stateInGame;
        ForceGarbageCollectionType forceGarbageCollection;

        SDKContext(SDK::UEngine *eng, SDK::UWorld *wrld, SDK::UPalUtility *util, SDK::APalGameStateInGame *state, ForceGarbageCollectionType fgc)
            : engine(eng), world(wrld), utility(util), stateInGame(state), forceGarbageCollection(fgc) {}
};

std::wstring utf8_to_utf16(const std::string &utf8) {
    int          size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    std::wstring utf16(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &utf16[0], size);
    return utf16;
}

std::string wide_to_narrow(const std::wstring &wide) {
    // 使用CP_ACP进行转换，将宽字符串转换为ANSI编码的窄字符串
    int         narrow_size = WideCharToMultiByte(CP_ACP, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string narrow(narrow_size, 0);
    WideCharToMultiByte(CP_ACP, 0, wide.c_str(), -1, &narrow[0], narrow_size, nullptr, nullptr);
    return narrow;
}

std::string get_query_parameter(const std::string &query, const std::string &key) {
    std::string key_with_equal = key + "=";
    size_t      start_pos      = query.find(key_with_equal);

    if (start_pos == std::string::npos)
        return ""; // 没找到参数

    // 跳过key和等号
    start_pos += key_with_equal.length();
    size_t end_pos = query.find("&", start_pos);

    if (end_pos == std::string::npos) {
        return query.substr(start_pos);
    }

    return query.substr(start_pos, end_pos - start_pos);
}

std::string url_decode(const std::string &input) {
    std::string result;
    result.reserve(input.size());

    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == '+') {
            result += ' ';
        } else if (input[i] == '%' && i + 2 < input.size()) {
            std::string hex          = input.substr(i + 1, 2);
            char        decoded_char = static_cast<char>(std::stoi(hex, nullptr, 16));
            result += decoded_char;
            i += 2; // Skip next two characters
        } else {
            result += input[i];
        }
    }

    return result;
}

void handle_request(http::request<http::string_body> &&req, http::response<http::string_body> &res, std::shared_ptr<SDKContext> sdkContext) {

     spdlog::info("Handling request for target: {}", std::string(req.target()));
    if (req.method() != http::verb::get) {
        res = { http::status::method_not_allowed, req.version() };
        res.set(http::field::content_type, "text/plain");
        res.keep_alive(req.keep_alive());
        res.body() = "Method Not Allowed";
        res.prepare_payload();
        return;
    }

    // 确定请求的路由
    if (req.target().starts_with("/rcon?")) {
        // 解析请求的URL参数
        auto target         = req.target();
        auto params         = target.substr(target.find("?") + 1);
        auto decoded_params = url_decode(params);

        // 提取并验证`text`参数
        auto text_param = get_query_parameter(decoded_params, "text"); 
        if (text_param.empty()) {
            res = { http::status::bad_request, req.version() };
            res.set(http::field::content_type, "text/plain");
            res.keep_alive(req.keep_alive());
            res.body() = "Bad Request: missing 'text' parameter";
            res.prepare_payload();
            return;
        }

        // 这里是处理命令的地方。将命令作为输出显示。
        std::string command_result = "Received command: " + text_param; 

        if (text_param == "state") {
            spdlog::info("[CMD::State] WorldName               = {}", sdkContext->stateInGame->GetWorldName().ToString());
            spdlog::info("[CMD::State] World Save Directory    = {}", sdkContext->stateInGame->WorldSaveDirectoryName.ToString());
            spdlog::info("[CMD::State] Server Frame Time       = {}", sdkContext->stateInGame->GetServerFrameTime());
            spdlog::info("[CMD::State] Max Player              = {}", sdkContext->stateInGame->GetMaxPlayerNum());
        } else if (text_param.starts_with("broadcast")) {
            auto message_utf8 = text_param.substr(10);

            // 将UTF-8字符串转换为UTF-16
            std::wstring message_utf16 = utf8_to_utf16(message_utf8);

            // 使用转换后的UTF-16字符串
            sdkContext->utility->SendSystemAnnounce(sdkContext->world, SDK::FString(message_utf16.c_str()));

            // 将宽字符串转换为窄字符串用于日志
            std::string message_narrow = wide_to_narrow(message_utf16);
            spdlog::info("[CMD::BroadcastChatMessage] {}", message_narrow);
        } else if (text_param == "gc") {
            sdkContext->forceGarbageCollection(sdkContext->engine, true);

            spdlog::info("[CMD::ForceGarbageCollection] done");
        } else {
            spdlog::info("[CMD::???] Unknown command");
        }


        // 设置HTTP响应
        res = { http::status::ok, req.version() };
        res.set(http::field::server, "Boost.Beast");
        res.set(http::field::content_type, "text/plain");
        res.keep_alive(req.keep_alive());
        res.body() = command_result;
        res.prepare_payload();
    } else {
        // 处理未知路由
        res = { http::status::not_found, req.version() };
        res.set(http::field::content_type, "text/plain");
        res.keep_alive(req.keep_alive());
        res.body() = "Not Found";
        res.prepare_payload();
    }
}


void http_server(tcp::acceptor &acceptor, std::shared_ptr<SDKContext> sdkContext) {
    // 创建新的socket
    auto socket = std::make_shared<tcp::socket>(acceptor.get_executor());

    acceptor.async_accept(*socket, [&acceptor, socket, sdkContext](boost::system::error_code ec) {
        if (!ec) {
            spdlog::info("New connection accepted");

            auto buffer = std::make_shared<beast::flat_buffer>();
            auto req    = std::make_shared<http::request<http::string_body>>();

            http::async_read(*socket, *buffer, *req, [socket, req, buffer, sdkContext](beast::error_code ec, std::size_t bytes_transferred) {
                if (!ec) {
                    spdlog::info("Received HTTP request: {}", std::string(req->target()));
                    http::response<http::string_body> res;
                    handle_request(std::move(*req), res, sdkContext);
                    http::write(*socket, res);
                    spdlog::info("HTTP response sent");
                } else {
                    spdlog::error("Error reading HTTP request: {}", ec.message());
                }
            });
        } else {
            spdlog::error("Error accepting connection: {}", ec.message());
        }

        // 准备下一个连接
        http_server(acceptor, sdkContext);
    });
}



void pal_loader_thread_start() {
    spdlog::info("loading ...");

    SDK::InitGObjects();

    SDK::UEngine              *engine                 = nullptr;
    SDK::UWorld               *world                  = nullptr;
    SDK::UPalUtility          *utility                = nullptr;
    SDK::APalGameStateInGame  *stateInGame            = nullptr;
    ForceGarbageCollectionType ForceGarbageCollection = nullptr;

    for (int i = 0; i < SDK::UObject::GObjects->Num(); i++) {
        SDK::UObject *object = SDK::UObject::GObjects->GetByIndex(i);

        if (!object)
            continue;

        if (object->IsA(SDK::UEngine::StaticClass()) && !object->IsDefaultObject()) {
            engine = static_cast<SDK::UEngine *>(object);
            break;
        }
    }

    world = *reinterpret_cast<SDK::UWorld **>(uintptr_t(GetImageBaseOffset()) + Offsets::GWorld);

    ForceGarbageCollection = reinterpret_cast<ForceGarbageCollectionType>(uintptr_t(GetImageBaseOffset()) + Offsets::ForceGarbageCollection);

    utility = SDK::UPalUtility::GetDefaultObj();

    stateInGame = utility->GetPalGameStateInGame(world);

    spdlog::info("Unreal::GObjects         = {:x}", uintptr_t(SDK::UObject::GObjects));
    spdlog::info("Unreal::GEngine          = {}", uintptr_t(engine));
    spdlog::info("Unreal::GWorld           = {}", uintptr_t(world));
    spdlog::info("UPalUtility              = {:x}", uintptr_t(utility));
    spdlog::info("PalGameStateInGame       = {:x}", uintptr_t(stateInGame));
    spdlog::info("IsDevelopmentBuild       = {}", utility->IsDevelopmentBuild());

    // Now wo can do some magic!

    // Hook code removed, it's unstable

    // 启动HTTP服务器

     try {
            auto const      address = net::ip::make_address("127.0.0.1");
            auto const      port    = static_cast<unsigned short>(53000);
            net::io_context ioc { 1 };

            tcp::acceptor acceptor { ioc, { address, port } };
            auto          sdkContext = std::make_shared<SDKContext>(engine, world, utility, stateInGame, ForceGarbageCollection);

            http_server(acceptor, sdkContext);

            spdlog::info("HTTP server is running at 127.0.0.1:53000 ...");
            ioc.run();
        } catch (std::exception const &e) {
            spdlog::error("Exception: {}", e.what());
        }
    


    while (true) {
        std::cout << "Pal Loader > ";
        std::string userInput;
        std::getline(std::cin, userInput);

        if (userInput == "state") {
            spdlog::info("[CMD::State] WorldName               = {}", stateInGame->GetWorldName().ToString());
            spdlog::info("[CMD::State] World Save Directory    = {}", stateInGame->WorldSaveDirectoryName.ToString());
            spdlog::info("[CMD::State] Server Frame Time       = {}", stateInGame->GetServerFrameTime());
            spdlog::info("[CMD::State] Max Player              = {}", stateInGame->GetMaxPlayerNum());
        } else if (userInput.starts_with("broadcast")) {
            auto message = userInput.substr(10);

            utility->SendSystemAnnounce(world, SDK::FString(std::wstring(message.begin(), message.end()).c_str()));

            spdlog::info("[CMD::BroadcastChatMessage] {}", message);
        } else if (userInput == "gc") {
            ForceGarbageCollection(engine, true);
            
            spdlog::info("[CMD::ForceGarbageCollection] done");
        } else {
            spdlog::info("[CMD::???] Unknown command");
        }
    }
}

