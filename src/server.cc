#include <unistd.h>
#include <pthread.h>
#include <ws.h>
#include <basic.cc>
#include "common.cc"

struct Mutex {
    pthread_mutex_t mutex;

    static auto create() -> Mutex {
        pthread_mutex_t mutex;
        assert(pthread_mutex_init(&mutex, NULL) == 0);
        return { mutex };
    }

    auto destroy() -> void {
        pthread_mutex_destroy(&mutex);
    }

    auto lock() -> void {
        pthread_mutex_lock(&mutex);
    }

    auto unlock() -> void {
        pthread_mutex_unlock(&mutex);
    }
};

struct Client {
    ptr<ws_cli_conn_t> connection;
    State state;

    static auto create(ptr<ws_cli_conn_t> connection, State state) -> Client {
        return { connection, state };
    }
};

Array<Client, player_cap> players{};
Mutex id_mutex;

auto make_id() -> u32 {
    id_mutex.lock();
    defer unlock = [](){ id_mutex.unlock(); };

    if (inactive_players.size() > 0) {
        return inactive_players.pop();
    } else {
        return players.tail++;
    }
}

auto connection_id(ptr<ws_cli_conn_t> connection) -> u32 {
    for (auto& client: players) {
        if (connection == client.connection) {
            return client.state.id;
        }
    }

    return -1;
}

auto random_float() -> f32 {
    return rand() / static_cast<f32>(RAND_MAX);
}

template <typename T>
auto ws_send(ptr<ws_cli_conn_t> client, T obj) -> void {
    ws_sendframe_bin(client, reinterpret_cast<ptr<char>>(&obj), sizeof(T));
}

template <typename T>
auto ws_send_all(T obj) -> void {
    for (auto& client: players) {
        if (is_active(client.state.id))
            ws_sendframe_bin(client.connection, reinterpret_cast<ptr<char>>(&obj), sizeof(T));
    }
}

auto on_ws_open(ptr<ws_cli_conn_t> client) -> void {
    auto cli = ws_getaddress(client);

    println("Connection opened, addr: %s", cli);
}

auto on_ws_close(ptr<ws_cli_conn_t> client) -> void {
    auto cli = ws_getaddress(client);

    auto id = connection_id(client);

    inactive_players.push(id);

    ws_send_all(Left_Message::create(id));

    println("Connection closed, addr: %s, id: %d", cli, id);
}

auto on_ws_message(ptr<ws_cli_conn_t> client, ptr<imm<u8>> msg, u64 size, i32 type) -> void {
    if (type != WS_FR_OP_BIN)
        ws_close_client(client);

    auto metadata = Message::get(msg);

    if (size != metadata.length)
        ws_close_client(client);

    switch (metadata.type) {
        case Message::Type::join: {
            //auto join = Join_Message::get(msg);

            if (inactive_players.tail == 0 && players.tail == player_cap)
                ws_close_client(client); // No space left for new player

            // TODO: batch
            for (auto& p: players) {
                ws_send(client, Joined_Message::create(p.state, false));
            }

            auto player = State::create(make_id(), random_float() * canvas_width, random_float() * canvas_height, random_float());

            players[player.id] = Client::create(client, player);

            println("joined message id: %d", Joined_Message::create(player, true).player.id);
            ws_send_all(Joined_Message::create(player, true));

            println("Player joined, id: %d", player.id);
        } break;
        case Message::Type::moving: {
            auto moving = Moving_Message::get(msg);

            players[moving.id].state.direction = moving.direction;

            ws_send_all(Moving_Message::create(moving.id, moving.direction));
        } break;
        default:
            assert(false);
    }
}

auto main() -> int {
    ws_server server{ "localhost", 8080, 1, 1000, { on_ws_open, on_ws_close, on_ws_message } };
    ws_socket(&server);

    while (1) {
        double fps = 60.0;
        double dt = 1000.0 / fps; // TODO: measure

        for (auto& p: players) {
            p.state.update(dt);
        }

        usleep(dt * 1000.0);
    }

    return 0;
}
