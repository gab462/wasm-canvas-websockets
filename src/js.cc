namespace js {

    extern "C" {
        auto document_get_element_by_id(ptr<imm<char>> id) -> usize;
        auto canvas_get_width(usize handle) -> u32;
        auto canvas_get_height(usize handle) -> u32;
        auto canvas_set_width(usize handle, u32 width) -> void;
        auto canvas_set_height(usize handle, u32 height) -> void;
        auto canvas_set_background_color(usize handle, ptr<imm<char>> context) -> void;
        auto canvas_get_context(usize handle, ptr<imm<char>> context) -> usize;
        auto context_clear(usize handle) -> void;
        auto context_set_stroke_style(usize handle, ptr<imm<char>> style) -> void;
        auto context_stroke_rect(usize handle, u32 x, u32 y, u32 w, u32 h) -> void;
        auto context_stroke_line(usize handle, u32 x1, u32 y1, u32 x2, u32 y2) -> void;
        auto web_socket_client(ptr<imm<char>> url) -> usize;
        auto web_socket_close(usize handle) -> void;
        auto web_socket_send_bin(usize handle, ptr<imm<char>> msg, usize length) -> void;
    }

    struct Context {
        usize handle;

        auto clear() -> void {
            context_clear(handle);
        }

        auto set_stroke_style(ptr<imm<char>> style) -> void {
            context_set_stroke_style(handle, style);
        }

        auto stroke_rect(u32 x, u32 y, u32 w, u32 h) -> void {
            context_stroke_rect(handle, x, y, w, h);
        }

        auto stroke_line(u32 x1, u32 y1, u32 x2, u32 y2) -> void {
            context_stroke_line(handle, x1, y1, x2, y2);
        }
    };

    struct Canvas {
        usize handle;

        static auto create(ptr<imm<char>> id) -> Canvas {
            return { document_get_element_by_id(id) };
        }

        auto get_width() -> u32 {
            return canvas_get_width(handle);
        }

        auto get_height() -> u32 {
            return canvas_get_height(handle);
        }

        auto set_width(u32 width) -> void {
            canvas_set_width(handle, width);
        }

        auto set_height(u32 height) -> void {
            canvas_set_height(handle, height);
        }

        auto set_background_color(ptr<imm<char>> color) -> void {
            canvas_set_background_color(handle, color);
        }

        auto get_context(ptr<imm<char>> context) -> Context {
            return { canvas_get_context(handle, context) };
        }
    };

    struct Web_Socket {
        usize handle;

        static auto create(ptr<imm<char>> url) -> Web_Socket {
            return { web_socket_client(url) };
        }

        auto close() -> void {
            web_socket_close(handle);
        }

        template <typename T>
        auto send(T obj) -> void {
            web_socket_send_bin(handle, reinterpret_cast<ptr<imm<char>>>(&obj), sizeof(T));
        }

        auto send_text(ptr<imm<char>> text) -> void {
            web_socket_send_bin(handle, text, strlen(text) + 1);
        }
    };
}
