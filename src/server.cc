#include <unistd.h>
#include <pthread.h>
#include <ws.h>
#include <basic.cc>
#include <sys/time.h>
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

struct Timer {
    timeval tv_start;

    auto start() -> void {
        gettimeofday(&tv_start, NULL);
    }

    auto end() -> f64 {
        timeval tv_end;

        gettimeofday(&tv_end, NULL);

        u64 start = tv_start.tv_sec * 1000000 + tv_start.tv_usec;
        u64 end = tv_end.tv_sec * 1000000 + tv_end.tv_usec;

        return end - start;
    }
};

Array<Client, player_cap> players{};
Mutex id_mutex = Mutex::create();

auto make_id() -> u32 {
    id_mutex.lock();
    defer unlock = [](){ id_mutex.unlock(); };

    for (auto& p: players) {
        if (!p.state.active) {
            return p.state.id;
        }
    }

    return players.tail++;
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
        if (client.state.active)
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

    players[id].state.active = false;

    ws_send_all(Left_Message::create(id));

    println("Connection closed, addr: %s, id: %d", cli, id);
}

auto on_ws_message(ptr<ws_cli_conn_t> client, ptr<imm<u8>> msg, u64 size, i32 type) -> void {
    if (type != WS_FR_OP_BIN || size < sizeof(Message)) {
        ws_close_client(client);
        return;
    }

    auto metadata = Message::get(msg);

    if (metadata.length != size  || metadata.magic != 0xC0FFEE) {
        ws_close_client(client);
        return;
    }

    switch (metadata.type) {
        case Message::Type::join: {
            //auto join = Join_Message::get(msg);

            // TODO:
            //if (inactive_players == 0 && players.tail == player_cap) {
            //    ws_close_client(client); // No space left for new player
            //    return;
            //}

            // TODO: batch
            for (auto& p: players) {
                ws_send(client, Joined_Message::create(p.state, false));
            }

            auto player = State::create(make_id(), random_float() * canvas_width, random_float() * canvas_height, random_float());

            players[player.id] = Client::create(client, player);

            ws_send_all(Joined_Message::create(player, true));
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

    defer cleanup = [](){ id_mutex.destroy(); };

    Timer timer;

    while (1) {
        static constexpr double fps = 60.0;
        static constexpr double dt = 1000000.0 / fps;

        timer.start();

        for (auto& p: players) {
            if(p.state.active)
                p.state.update(dt/1000.0); // msec
        }

        usleep(dt - timer.end());
    }

    return 0;
}
