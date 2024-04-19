#include <iostream>
#include <boost/asio.hpp>
#include <string>
#include <memory>

using namespace boost::asio;
using namespace boost::asio::ip;

const char response[] =
"HTTP/1.1 200 OK\r\n"
"Server: My Cool Server\r\n"
"Content-Type: text/html; charset=utf-8\r\n"
"Content-Length: 233\r\n"
"Connection: close\r\n"
"\r\n"
"<html><head><title>My Cool Server</title></head><body><h1>Hello</h1><p>Welcome to my server</p></body></html>";

struct MyServer : std::enable_shared_from_this<MyServer>
{
    io_context& context;
    tcp::socket socket;
    boost::asio::streambuf buf;

    MyServer(io_context& c, tcp::socket&& s)
        : context(c), socket(std::move(s)) {}

    void waitForRequest()
    {
        auto self = shared_from_this();
        async_read_until(socket, buf, "\r\n\r\n",
            [self](boost::system::error_code ec, size_t size) {
                if (!ec)
                {
                    self->socket.async_send(boost::asio::buffer(response, 233),
                        [self](boost::system::error_code ec, size_t size) {
                            self->socket.close();
                        });
                }
            });
    }
};

void handle_accept(tcp::acceptor& acceptor, io_context& context)
{
    acceptor.async_accept(
        [&acceptor, &context](boost::system::error_code ec, tcp::socket socket)
        {
            if (!ec)
            {
                std::make_shared<MyServer>(context, std::move(socket))->waitForRequest();
            }
            handle_accept(acceptor, context);
        });
}

int main()
{
    try
    {
        io_context context;
        tcp::endpoint ep(address::from_string("0.0.0.0"), 80);
        tcp::acceptor acceptor(context, ep);
        handle_accept(acceptor, context);
        context.run();
    }
    catch (const std::exception& e)
    {
        std::cout << "Error: " << e.what() << std::endl;
    }
}
