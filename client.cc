#include "basic/basic.cc"
#include "js.cc"

struct State {
    u32 x;
    u32 y;
    u32 size;
};

State state{0, 0, 100};
js::Canvas canvas;
js::Context ctx;
js::Web_Socket ws;
static constexpr usize message_cap = 128;
alignas(u64) char message_buffer[message_cap];

extern "C" {

    auto init() -> void {
        canvas = js::Canvas::create("app");
        canvas.set_width(800);
        canvas.set_height(600);
        canvas.set_background_color("black");

        ctx = canvas.get_context("2d");

        ws = js::Web_Socket::create("ws://localhost:8080");

        println("ok");
    }

    auto update(int timestamp) -> void {
        (void) timestamp;
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
            state.y -= state.size;
            break;
        case 'a':
            state.x -= state.size;
            break;
        case 's':
            state.y += state.size;
            break;
        case 'd':
            state.x += state.size;
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
        ws.send_text("Hello from client");
    }

    auto on_ws_message() -> void {
        println("Received message from server:");
        println(static_cast<ptr<imm<char>>>(message_buffer));
    }

}
