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
    auto per_session_strand = boost::asio::make_strand(ex);

    co_spawn(per_session_strand, session(std::move(sock)),
             boost::asio::detached);
  }
}

awaitable<void> session(tcp::socket sock) {
  std::array<char, SESSION_BUFFER_SIZE> buf{};
  error_code ec;

  for (;;) {
    spdlog::info("read from client");
    size_t n = co_await sock.async_read_some(
        boost::asio::buffer(buf, SESSION_BUFFER_SIZE),
        boost::asio::redirect_error(boost::asio::use_awaitable, ec));
    if (ec == boost::asio::error::eof) {
      spdlog::info("peer closed connection");
      co_return;
    } else if (ec) {
      spdlog::error("read error: {}", ec.message());
    }
    spdlog::info("read {} bytes", n);

    spdlog::info("write back");
    co_await boost::asio::async_write(
        sock, boost::asio::buffer(buf, n),
        boost::asio::redirect_error(boost::asio::use_awaitable, ec));
    if (ec) {
      spdlog::error("write error: {}", ec.message());
    }
  }
}

int main() {
  spdlog::info("initial the io context");
  boost::asio::io_context io;

  spdlog::info("initial listener");
  tcp::acceptor acceptor{io, tcp::endpoint{tcp::v4(), 12345}};

  spdlog::info("start listenning...");
  co_spawn(io, listener(std::move(acceptor)), boost::asio::detached);

  io.run();
  return 0;
}