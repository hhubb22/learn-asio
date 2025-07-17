#include <asio.hpp>
#include <exception>
#include <fmt/printf.h>
#include <iostream>

namespace net = asio;
using tcp = net::ip::tcp;

// 每个客户端会话
net::awaitable<void> echo_session(tcp::socket socket) {
  try {
    std::array<char, 4096> data;
    for (;;) {
      std::size_t n = co_await socket.async_read_some(net::buffer(data),
                                                      net::use_awaitable);
      co_await net::async_write(socket, net::buffer(data, n),
                                net::use_awaitable);
    }
  } catch (std::exception const &e) {
    std::cerr << "session error: " << e.what() << "\n";
    // 自动关闭 socket (RAII)
  }
}

// 接受循环
net::awaitable<void> listener(uint16_t port) {
  auto exec = co_await net::this_coro::executor;
  tcp::acceptor acc(exec, {tcp::v4(), port});
  acc.set_option(net::socket_base::reuse_address(true));
  for (;;) {
    try {
      tcp::socket s = co_await acc.async_accept(net::use_awaitable);
      s.set_option(tcp::no_delay(true));
      net::co_spawn(exec, echo_session(std::move(s)), net::detached);
    } catch (std::exception const &e) {
      std::cerr << "accept error: " << e.what() << "\n";
    }
  }
}

int main() {
  try {
    net::io_context io;

    net::signal_set signals(io, SIGINT, SIGTERM);
    signals.async_wait([&](auto ec, int /* sig */) {
      if (!ec)
        io.stop();
    });

    net::co_spawn(io, listener(8080), net::detached);

    std::vector<std::thread> threads;
    for (auto i = 0u; i < std::thread::hardware_concurrency(); ++i)
      threads.emplace_back([&] { io.run(); });

    for (auto &t : threads)
      t.join();
  } catch (std::exception const &e) {
    std::cerr << "fatal: " << e.what() << "\n";
  }
}