#include <iostream>
#include <fstream>
#include <boost/asio.hpp>
#include <memory>

using boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket, const std::string& content)
        : socket_(std::move(socket)), content_(content) {}

    void start() {
        do_write();
    }

private:
    void do_write() {
        auto self(shared_from_this());
        boost::asio::async_write(socket_, boost::asio::buffer(content_),
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    socket_.close();
                }
            });
    }

    tcp::socket socket_;
    std::string content_;
};

class Server {
public:
    Server(boost::asio::io_context& io_context, short port, const std::string& file_path)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)), file_path_(file_path) {
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::string content = read_file(file_path_);
                    std::make_shared<Session>(std::move(socket), content)->start();
                }
                do_accept();
            });
    }

    std::string read_file(const std::string& file_path) {
        std::ifstream file(file_path, std::ios::in | std::ios::binary);
        if (!file) {
            return "Error: File not found.";
        }
        return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }

    tcp::acceptor acceptor_;
    std::string file_path_;
};

int main(int argc, char* argv[]) {
    try {
        if (argc != 3) {
            std::cerr << "Usage: server <port> <file_path>\n";
            return 1;
        }

        boost::asio::io_context io_context;
        Server s(io_context, std::atoi(argv[1]), argv[2]);
        io_context.run();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
