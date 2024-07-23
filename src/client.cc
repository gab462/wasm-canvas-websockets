#include <basic.cc>
#include "js.cc"
#include "common.cc"

static constexpr usize message_cap = 1024;
Arena msgbuf;
State state;

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

        state = State{};

        msgbuf = Arena::create(message_cap);

        println("ok");
    }

    auto update(f32 timestamp) -> void {
        static f32 previous = 0;
        f32 dt = timestamp - previous;
        defer set_previous = [timestamp](){ previous = timestamp; };

        state.update(dt);

        usize processed = 0;
        while (processed < msgbuf.position) {
            auto data = static_cast<ptr<u8>>(msgbuf.memory) + processed;
            Message msg = Message::get(data);

            if (msg.type == Message::Type::moving) {
                auto moving = Moving_Message::get(data);

                state.direction = moving.direction;
            } else if (msg.type == Message::Type::init) {
                auto init = Init_Message::get(data);

                state = State::create(init.x, init.y, init.hue);
            } else {
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
        ctx.set_stroke_style("red");
        ctx.stroke_rect(state.x, state.y, state.size, state.size);
        ctx.stroke_line(state.x, state.y, state.x + state.size, state.y + state.size);
    }

    auto key_down(char c) -> void {
        switch (c) {
        case 'w':
            ws.send(Moving_Message::create(State::Moving::up));
            break;
        case 'a':
            ws.send(Moving_Message::create(State::Moving::left));
            break;
        case 's':
            ws.send(Moving_Message::create(State::Moving::down));
            break;
        case 'd':
            ws.send(Moving_Message::create(State::Moving::right));
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
        ws.send(Joined_Message::create());
    }

    auto allocate_message(usize n) -> ptr<u8> {
        return msgbuf.allocate<u8>(n);
    }

}
