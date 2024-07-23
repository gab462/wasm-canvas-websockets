#include <unistd.h>
#include <ws.h>
#include <basic.cc>
#include "common.cc"

template <typename T>
auto ws_send(ptr<ws_cli_conn_t> client, T obj) -> void {
    assert(ws_sendframe_bin(client, reinterpret_cast<ptr<char>>(&obj), sizeof(T)) > 0);
}

State state{};

auto on_ws_open(ptr<ws_cli_conn_t> client) -> void {
    auto cli = ws_getaddress(client);

    println("Connection opened, addr: %s", cli);
}

auto on_ws_close(ptr<ws_cli_conn_t> client) -> void {
    auto cli = ws_getaddress(client);

    println("Connection closed, addr: %s", cli);
}

auto on_ws_message(ptr<ws_cli_conn_t> client, ptr<imm<u8>> msg, u64 size, i32 type) -> void {
    //auto cli = ws_getaddress(client);

    auto metadata = Message::get(msg);

    assert(size == metadata.length && type == WS_FR_OP_BIN);

    if (metadata.type == Message::Type::join) {
        //auto join = Joined_Message::get(msg);

        state = State::create(0.f, 0.f, 1.f);

        ws_send(client, Init_Message::create(state.x, state.y, state.hue));
    } else if (metadata.type == Message::Type::moving) {
        auto moving = Moving_Message::get(msg);

        state.direction = moving.direction;

        ws_send(client, Moving_Message::create(moving.direction));
    }
}

auto main() -> int {
    ws_server server{ "localhost", 8080, 1, 1000, { on_ws_open, on_ws_close, on_ws_message } };
    ws_socket(&server);

    while (1) {
        double fps = 60.0;
        double dt = 1000.0 / fps; // TODO: measure

        state.update(dt);

        usleep(dt);
    }

    return 0;
}
