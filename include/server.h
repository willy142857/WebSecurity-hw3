#ifndef SERVER_H
#define SERVER_H

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

/// A request received from a client.
class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(boost::asio::ip::tcp::socket, boost::asio::ssl::context&);

    void start();

private:
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket_;
    boost::asio::streambuf buf;
    std::string statusStr = "HTTP/1.1 200 OK\n";
    std::string foo = "foo";
    std::unordered_map<std::string, std::string> headers;
    std::string payload;
    std::string REQUEST_METHOD;
    std::string REQUEST_URI;
    std::string QUERY_STRING;
    std::string SERVER_PROTOCOL;
    char HTTP_HOST[100];
    std::string SCRIPT_NAME = "./";
    char blackhole[100];
    char responseBuf[8192];
    std::vector<boost::asio::const_buffer> buffers;
    void doHandshake();

    void readFirstLine();

    void readNextLine();

    void readBody(int length);

    void doWrite(std::size_t length);

    void setEnv();

    void response();
};

class WebServer
{
public:
    WebServer(boost::asio::io_context& ioContext, short port);

private:
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::ssl::context context_;

    void accept();
};

#endif
