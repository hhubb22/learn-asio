#include <array>
#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <spdlog/spdlog.h>
#include <utility>

using tcp = boost::asio::ip::tcp;
using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::system::error_code;

const size_t SESSION_BUFFER_SIZE = 1024;

awaitable<void> session(tcp::socket sock);

awaitable<void> listener(tcp::acceptor acceptor) {
  auto ex = co_await boost::asio::this_coro::executor;
  for (;;) {
    auto sock = co_await acceptor.async_accept(boost::asio::use_awaitable);
    auto ep = sock.remote_endpoint();
    spdlog::info("Accepted connection from {}:{}", ep.address().to_string(),
                 ep.port());
    auto per_session_strand = boost::asio::make_strand(ex);

    co_spawn(per_session_strand, session(std::move(sock)),
             boost::asio::detached);
  }
}

awaitable<void> session(tcp::socket sock) {
  auto endpoint = sock.remote_endpoint();
  auto endpoint_addr = endpoint.address().to_string();
  auto endpoint_port = endpoint.port();
  std::array<char, SESSION_BUFFER_SIZE> buf{};
  error_code ec;

  for (;;) {
    spdlog::debug("Waiting for data from {}:{}", endpoint_addr, endpoint_port);
    size_t n = co_await sock.async_read_some(
        boost::asio::buffer(buf, SESSION_BUFFER_SIZE),
        boost::asio::redirect_error(boost::asio::use_awaitable, ec));
    if (ec == boost::asio::error::eof) {
      spdlog::info("Connection {}:{} closed by peer", endpoint_addr,
                   endpoint_port);
      co_return;
    } else if (ec) {
      spdlog::error("Read error from {}:{}: {}", endpoint_addr, endpoint_port,
                    ec.message());
    }
    spdlog::debug("Read {} bytes from {}:{}", n, endpoint_addr, endpoint_port);

    spdlog::debug("Echoing {} bytes to {}:{}", n, endpoint_addr, endpoint_port);
    co_await boost::asio::async_write(
        sock, boost::asio::buffer(buf, n),
        boost::asio::redirect_error(boost::asio::use_awaitable, ec));
    if (ec) {
      spdlog::error("Write error to {}:{}: {}", endpoint_addr, endpoint_port,
                    ec.message());
    }
  }
}

int main() {
  spdlog::info("Initializing I/O context");
  boost::asio::io_context io;

  spdlog::info("Starting listener on port {}", 12345);
  tcp::acceptor acceptor{io, tcp::endpoint{tcp::v4(), 12345}};

  spdlog::info("Server listening...");
  co_spawn(io, listener(std::move(acceptor)), boost::asio::detached);

  io.run();
  return 0;
}