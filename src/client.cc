#include <basic.cc>
#include "js.cc"
#include "common.cc"

static constexpr usize message_cap = 1024;
Arena msgbuf;
i32 id = -1;
Array<State, player_cap> players{};

js::Canvas canvas;
js::Context ctx;
js::Web_Socket ws;

extern "C" {

    auto init() -> void {
        canvas = js::Canvas::create("app");
        canvas.set_width(canvas_width);
        canvas.set_height(canvas_height);
        canvas.set_background_color("black");

        ctx = canvas.get_context("2d");

        ws = js::Web_Socket::create("ws://localhost:8080");

        msgbuf = Arena::create(message_cap);
    }

    auto update(f32 timestamp) -> void {
        static f32 previous = 0;
        f32 dt = timestamp - previous;
        defer set_previous = [timestamp](){ previous = timestamp; };

        for (auto& player: players) {
            if (is_active(player.id)) {
                player.update(dt);
            }
        }

        usize processed = 0;
        while (processed < msgbuf.position) {
            auto data = static_cast<ptr<u8>>(msgbuf.memory) + processed;
            Message msg = Message::get(data);

            switch (msg.type) {
                case Message::Type::moving: {
                    auto moving = Moving_Message::get(data);

                    players[moving.id].direction = moving.direction;
                } break;
                case Message::Type::joined: {
                    auto joined = Joined_Message::get(data);

                    if (players.tail > joined.player.id) {
                        players[joined.player.id] = joined.player;

                        if (id > 0 && joined.is_new == true)
                            inactive_players.pop();
                    } else {
                        players.append(joined.player);
                    }

                    if (joined.is_new && id == -1) {
                        id = joined.player.id;
                    }
                } break;
                case Message::Type::left: {
                    auto left = Left_Message::get(data);

                    inactive_players.push(left.id);
                } break;
                default:
                    assert(false);
            }

            processed += msg.length;
        }

        // Reset arena
        msgbuf.destroy();
        msgbuf = Arena::create(message_cap);
    }

    auto render() -> void {
        ctx.clear();

        for (auto& player: players) {
            if (is_active(player.id)) {
                ctx.set_stroke_hue(player.hue);
                ctx.stroke_rect(player.x, player.y, player.size, player.size);
                ctx.stroke_line(player.x, player.y, player.x + player.size, player.y + player.size);
            }
        }
    }

    auto key_down(char c) -> void {
        switch (c) {
        case 'w':
            ws.send(Moving_Message::create(id, State::Moving::up));
            break;
        case 'a':
            ws.send(Moving_Message::create(id, State::Moving::left));
            break;
        case 's':
            ws.send(Moving_Message::create(id, State::Moving::down));
            break;
        case 'd':
            ws.send(Moving_Message::create(id, State::Moving::right));
            break;
        }
    }

    auto key_up(char c) -> void {
        (void) c;
    }

    auto mouse_down(i32 x, i32 y) -> void {
        (void) x;
        (void) y;
    }

    auto mouse_up(i32 x, i32 y) -> void {
        (void) x;
        (void) y;
    }

    auto on_ws_open() -> void {
        ws.send(Join_Message::create());
        println("Connected to ws");
    }

    auto on_ws_close() -> void {
        println("Disconnected from ws");
    }

    auto allocate_message(usize n) -> ptr<u8> {
        return msgbuf.allocate<u8>(n);
    }

}
