async function main() {
    let c = null;
    let handles = [];

    let makeHandle = (obj) => handles.push(obj) - 1;

    function cstr(ptr) {
        let mem = new Uint8Array(c.memory.buffer);
        let len = 0;
        while (mem[ptr + len] != 0) {
            len++;
        }

        return new TextDecoder().decode(new Uint8Array(c.memory.buffer, ptr, len));
    }

    let wasm = await WebAssembly.instantiateStreaming(fetch('index.wasm'), {
        env: {
            write: (_, ptr, len) => console.log(new TextDecoder().decode(new Uint8Array(c.memory.buffer, ptr, len))),
            assert_here: (file, len, line, cond) => { if (!cond) throw new Error(`${cstr(file, len)}:${line}: Assertion Fail`); },
            document_get_element_by_id: (id) => makeHandle(document.getElementById(cstr(id))),
            canvas_get_width: (handle) => handles[handle].width,
            canvas_get_height: (handle) => handles[handle].height,
            canvas_set_width: (handle, width) => handles[handle].width = width,
            canvas_set_height: (handle, height) => handles[handle].height = height,
            canvas_set_background_color: (handle, style) => handles[handle].style.backgroundColor = cstr(style),
            canvas_get_context: (handle, ctx) => makeHandle(handles[handle].getContext(cstr(ctx))),
            context_clear: (handle) => handles[handle].clearRect(0, 0, handles[handle].canvas.width, handles[handle].canvas.height),
            context_set_stroke_style: (handle, style) => handles[handle].strokeStyle = cstr(style),
            context_set_stroke_hue: (handle, hue) => handles[handle].strokeStyle = `hsl(${hue*360} 70 40)`,
            context_stroke_rect: (handle, x, y, w, h) => handles[handle].strokeRect(x, y, w, h),
            context_stroke_line: (handle, x1, y1, x2, y2) => {
                let ctx = handles[handle];

                ctx.beginPath();
                ctx.moveTo(x1, y1);
                ctx.lineTo(x2, y2);
                ctx.stroke();
            },
            web_socket_client: (url) => {
                let ws = new WebSocket(cstr(url));
                let handle = makeHandle(ws);

                ws.onopen = (event) => c.on_ws_open();
                ws.onclose = (event) => c.on_ws_close();
                ws.onmessage = async (event) => {
                    let msg = new Uint8Array(await event.data.arrayBuffer());
                    let len = msg.byteLength;
                    let addr = c.allocate_message(len);
                    let buffer = new Uint8Array(c.memory.buffer, addr, len);
                    buffer.set(msg);
                };

                return handle;
            },
            web_socket_close: (handle) => handles[handle].close(),
            web_socket_send_bin: (handle, obj, len) => handles[handle].send(new Uint8Array(c.memory.buffer, obj, len)),
        }
    });

    c = wasm.instance.exports;

    c.init();

    let loop = timestamp => {
        c.update(timestamp);
        c.render();

        window.requestAnimationFrame(loop);
    };

    Object.entries({
        keydown: e => c.key_down(e.key.charCodeAt()),
        keyup: e => c.key_up(e.key.charCodeAt()),
        mousedown: e => c.mouse_down(e.x, e.y),
        mouseup: e => c.mouse_up(e.x, e.y)
    }).forEach(([event, fn]) => {
        document.addEventListener(event, fn);
    });

    window.requestAnimationFrame(loop);
}

main();
