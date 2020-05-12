#include "include/server.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <fmt/color.h>
#include <fmt/core.h>
#include "include/base64.h"
#include "include/utility.h"

namespace fs = std::filesystem;
using namespace boost;
using boost::asio::ip::tcp;

/// A request received from a client.

Session::Session(tcp::socket socket, asio::ssl::context& context) 
    : socket_(std::move(socket), context) {}

void Session::start() 
{
    doHandshake(); 
}

void Session::doHandshake()
{
    auto self(shared_from_this());
    socket_.async_handshake(asio::ssl::stream_base::server, 
        [this, self](const system::error_code& error) {
            if (!error)
                readFirstLine();
            else
                fmt::print(fmt::fg(fmt::color::indian_red), "{}\n", error.message());
        });
}

void Session::readFirstLine()
{
    auto self(shared_from_this());
    asio::async_read_until(
        socket_, buf, '\r',
        [this, self](const system::error_code &ec, std::size_t s) {
            std::string line, ignore;
            std::istream stream{&buf};
            std::getline(stream, line, '\r');
            std::getline(stream, ignore, '\n');

            if(line.empty())
                return;

            std::stringstream{line} >> REQUEST_METHOD >> REQUEST_URI >> SERVER_PROTOCOL;
            const auto pos = REQUEST_URI.find('?');
            if (pos != std::string::npos) {
                QUERY_STRING = REQUEST_URI.substr(pos + 1);
                REQUEST_URI = REQUEST_URI.substr(0, pos);
            }
            SCRIPT_NAME = "." + REQUEST_URI;
            
            readNextLine();
        });
}

void Session::readNextLine()
{
    auto self(shared_from_this());
    asio::async_read_until(
        socket_, buf, '\r',
        [this, self](const system::error_code &ec, std::size_t s) {
            std::string line, ignore;
            std::istream stream{&buf};
            std::getline(stream, line, '\r');
            std::getline(stream, ignore, '\n');
            
            if (line.empty()) {
                if (headers.count("Content-Length") == 0 || headers["Content-Length"] == "0")
                    doWrite(55688);
                else {
                    readBody(std::stoi(headers["Content-Length"]) - buf.size());
                }
            }
            else {
                const auto pos = line.find(':');
                headers.emplace(line.substr(0, pos), line.substr(pos + 2));
                readNextLine();
            }
        });
}

void Session::readBody(int length)
{
    auto self(shared_from_this());
    asio::async_read(socket_, buf, asio::transfer_exactly(length), 
        [this, self, length](const system::error_code& ec, std::size_t s) {
            std::istream stream{&buf};
            std::stringstream ss;
            ss << stream.rdbuf();
            Base64::Encode(ss.str(), &payload);

            if (!ec)
                doWrite(55688);
        });
}   

void Session::doWrite(std::size_t length)
{
    fmt::print(fmt::fg(fmt::color::yellow), "[{}]\n", timestamp());
    fmt::print(fmt::fg(fmt::color::light_sea_green), "New Connection\n");
    fmt::print(fmt::fg(fmt::color::azure), "Method: {}\n", REQUEST_METHOD);
    fmt::print(fmt::fg(fmt::color::azure), "Uri: {}\n", REQUEST_URI);
    if (REQUEST_METHOD == "POST")
        fmt::print(fmt::fg(fmt::color::azure), "Content-Length: {}\n", headers["Content-Length"]);
    setEnv();
    response();

    auto self(shared_from_this());
    asio::async_write(
        socket_, buffers,
        [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                fmt::print("response to client successfully\n");
                //readFirstLine();
            }
            else
                fmt::print(fmt::fg(fmt::color::orange_red), "{}\n", ec.message());
        });
}

void Session::setEnv()
{
    setenv("REQUEST_METHOD", REQUEST_METHOD.c_str(), 1);
    setenv("REQUEST_URI", REQUEST_URI.c_str(), 1);
    setenv("QUERY_STRING", QUERY_STRING.c_str(), 1);
    setenv("SERVER_PROTOCOL", SERVER_PROTOCOL.c_str(), 1);
    setenv("HTTP_HOST", HTTP_HOST, 1);
    setenv("SCRIPT_NAME", SCRIPT_NAME.c_str(), 1);
    if (REQUEST_METHOD == "POST") {
        setenv("CONTENT_LENGTH", headers["Content-Length"].c_str(), 1);
    }
}

void Session::response()
{
    int cgiInput[2];
    int cgiOutput[2];
    if (pipe(cgiInput) < 0)
        fmt::print(stderr, fmt::fg(fmt::color::red), "pipe error\n");
    if (pipe(cgiOutput) < 0)
        fmt::print(stderr, fmt::fg(fmt::color::red), "pipe error\n");

    auto& ioContext = socket_.get_executor().context();
    ioContext.notify_fork(asio::io_context::fork_prepare);
    if (fork() != 0) {
        ioContext.notify_fork(asio::io_context::fork_parent);
        close(cgiInput[0]);
        close(cgiOutput[1]);

        if (headers.count("Content-Length"))
            write(cgiInput[1], payload.data(), payload.length());
        close(cgiInput[1]);

        buffers.push_back(asio::buffer(statusStr));
        int len = 0;
        while (len = read(cgiOutput[0], responseBuf, sizeof(responseBuf))) {
            buffers.push_back(asio::buffer(responseBuf, len));
        }

        close(cgiOutput[0]);
        //socket_.shutdown();
    }
    else {
        ioContext.notify_fork(asio::io_context::fork_child);
        close(cgiOutput[0]);
        close(cgiInput[1]);

        dup2(cgiOutput[1], STDOUT_FILENO);
        dup2(cgiInput[0], STDIN_FILENO);

        close(cgiOutput[1]);
        close(cgiInput[0]);
        
        fs::current_path("./build/cgi-bin");
        if (REQUEST_METHOD == "GET" && REQUEST_URI == "/view.cgi/" &&
            QUERY_STRING.find("file=") != std::string::npos)
            execlp("./viewstatic.cgi", "./viewstatic.cgi", NULL);
        else if (REQUEST_METHOD == "GET" && REQUEST_URI == "/")
            execlp("./view.cgi", "./view.cgi", NULL);
        else if (SCRIPT_NAME == "./view.cgi" || SCRIPT_NAME == "./insert.cgi")
            if (execlp(SCRIPT_NAME.c_str(), SCRIPT_NAME.c_str(), NULL) < 0)
                std::cout << "Content-type:text/html\r\n\r\n<h1>Not Found</h1>";
        exit(0);
    }
}


WebServer::WebServer(asio::io_context& ioContext, short port)
    : acceptor_(ioContext, tcp::endpoint(tcp::v4(), port)),
      context_(asio::ssl::context::sslv23)
{
    context_.set_options(
        asio::ssl::context::default_workarounds | asio::ssl::context::no_sslv2 | asio::ssl::context::single_dh_use);
    context_.use_certificate_chain_file(".key/host.crt");
    context_.use_private_key_file(".key/host.key", asio::ssl::context::pem);
    context_.set_verify_mode(asio::ssl::context::verify_none);
    accpet();
}

void WebServer::accpet()
{
    acceptor_.async_accept([this](const system::error_code& ec, tcp::socket socket) {
        if (!ec) {
            std::make_shared<Session>(std::move(socket), context_)->start();
        }
        accpet();
    });
}


