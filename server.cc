#include <unistd.h>
#include <ws.h>
#include "basic/basic.cc"

auto on_ws_open(ptr<ws_cli_conn_t> client) -> void {
    auto cli = ws_getaddress(client);

    println("Connection opened, addr: %s", cli);
}

auto on_ws_close(ptr<ws_cli_conn_t> client) -> void {
    auto cli = ws_getaddress(client);

    println("Connection closed, addr: %s", cli);
}

auto on_ws_message(ptr<ws_cli_conn_t> client, ptr<imm<u8>> msg, u64 size, i32 type) -> void {
    auto cli = ws_getaddress(client);

    println("I receive a message: %s (%zu), from: %s", msg, size, cli);

    ws_sendframe_bin(client, "Hello from Server", 18);
}

auto main() -> int {
    ws_server server{ "localhost", 8080, 0, 1000, { on_ws_open, on_ws_close, on_ws_message } };
    ws_socket(&server);

    return 0;
}
