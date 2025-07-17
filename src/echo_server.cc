#include <array>
#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <vector>

using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::ip::tcp;
namespace this_coro = boost::asio::this_coro;

// 单个会话协程
awaitable<void> session(tcp::socket socket) {
  try {
    // 获取当前 executor（可能是绑定了 strand 的 executor）
    auto executor = co_await this_coro::executor;
    std::array<char, 1024> buffer;
    for (;;) {
      // 异步读取
      std::size_t n = co_await socket.async_read_some(
          boost::asio::buffer(buffer), boost::asio::use_awaitable);
      // 异步写入
      co_await boost::asio::async_write(socket, boost::asio::buffer(buffer, n),
                                        boost::asio::use_awaitable);
    }
  } catch (const boost::system::system_error &e) {
    // 区分正常的连接关闭和真正的错误
    if (e.code() == boost::asio::error::eof) {
      std::cout << "Client disconnected normally\n";
    } else {
      std::cerr << "Session error: " << e.what() << "\n";
    }
  } catch (const std::exception &e) {
    std::cerr << "Session ended with exception: " << e.what() << "\n";
  }
}

// 监听并接受新连接
awaitable<void> listener(boost::asio::io_context &io, unsigned short port) {
  // 在 listener 级别绑定一个 strand，保证 accept 后再分配给 session
  auto executor = boost::asio::make_strand(io.get_executor());
  tcp::acceptor acceptor(executor, {tcp::v4(), port});
  for (;;) {
    tcp::socket sock =
        co_await acceptor.async_accept(boost::asio::use_awaitable);
    auto per_session_executor =
        boost::asio::make_strand(acceptor.get_executor());
    // 每个会话在自己的 strand 上启动协程，保证其内部操作串行
    co_spawn(per_session_executor, session(std::move(sock)), detached);
  }
}

int main() {
  boost::asio::io_context io_context;

  // 启动 listener 协程
  co_spawn(io_context, listener(io_context, 12345), detached);

  // 启动线程池运行 io_context
  std::vector<std::thread> threads;
  for (unsigned i = 0; i < std::thread::hardware_concurrency(); ++i) {
    threads.emplace_back([&] { io_context.run(); });
  }
  for (auto &t : threads)
    t.join();

  return 0;
}