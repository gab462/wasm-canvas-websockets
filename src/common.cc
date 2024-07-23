static constexpr usize canvas_width = 800;
static constexpr usize canvas_height = 600;

struct State {
    enum Moving: u32 { none, up, down, left, right };

    f32 x;
    f32 y;
    u32 size;
    Moving direction;
    f32 hue;

    static auto create(f32 x, f32 y, f32 hue) -> State {
        return { x, y, 50, Moving::none, hue };
    }

    static auto wrap(f32 val, f32 max) -> f32 {
        if (val < 0)
            return max + val;
        else if (val > max)
            return wrap(val - max, max);
        else
            return val;
    }

    auto update(f32 dt) -> void {
        static constexpr f32 speed = 0.5f;

        switch (direction) {
            case Moving::up:
                y = wrap(y - dt * speed, canvas_height);
                break;
            case Moving::left:
                x = wrap(x - dt * speed, canvas_width);
                break;
            case Moving::down:
                y = wrap(y + dt * speed, canvas_height);
                break;
            case Moving::right:
                x = wrap(x + dt * speed, canvas_width);
                break;
            default:
                break;
        }
    }
};

template <typename T>
auto get_unaligned(ptr<imm<u8>> unaligned) -> T {
    T out;

    memcpy(reinterpret_cast<ptr<char>>(&out), reinterpret_cast<ptr<imm<char>>>(unaligned), sizeof(T)); // FIXME

    return out;
}

struct Message {
    enum Type: u32 { join, init, moving };

    Type type;
    u32 length;

    static auto get(ptr<imm<u8>> unaligned) -> Message {
        return get_unaligned<Message>(unaligned);
    }
};

struct Joined_Message {
    Message metadata;

    static auto create() -> Joined_Message {
        return { { Message::Type::join, sizeof(Joined_Message) } };
    }

    static auto get(ptr<imm<u8>> unaligned) -> Joined_Message {
        return get_unaligned<Joined_Message>(unaligned);
    }
};

struct Init_Message {
    Message metadata;
    u32 x;
    u32 y;
    f32 hue;

    static auto create(u32 x, u32 y, f32 hue) -> Init_Message {
        return { { Message::Type::init, sizeof(Init_Message) }, x, y, hue };
    }

    static auto get(ptr<imm<u8>> unaligned) -> Init_Message {
        return get_unaligned<Init_Message>(unaligned);
    }
};

struct Moving_Message {
    Message metadata;
    State::Moving direction;

    static auto create(State::Moving direction) -> Moving_Message {
        return { { Message::Type::moving, sizeof(Moving_Message) }, direction };
    }

    static auto get(ptr<imm<u8>> unaligned) -> Moving_Message {
        return get_unaligned<Moving_Message>(unaligned);
    }
};
