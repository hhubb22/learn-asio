# 创建可执行文件
add_executable(echo_server echo_server.cc)

# 设置目标属性
set_target_properties(echo_server PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# 链接库
target_link_libraries(echo_server PRIVATE 
    fmt::fmt
    spdlog::spdlog
    asio::asio
)
