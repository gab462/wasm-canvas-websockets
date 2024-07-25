static constexpr usize canvas_width = 800;
static constexpr usize canvas_height = 600;

struct State {
    enum Moving: u8 { none, up, down, left, right };

    bool active;
    u8 id;
    Moving direction;
    u8 size;
    f32 x;
    f32 y;
    f32 hue;

    static auto create(u8 id, f32 x, f32 y, f32 hue) -> State {
        return { true, id, Moving::none, 50, x, y, hue };
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
    enum Type: u32 { join, joined, left, moving };

    u32 magic;
    Type type;
    u32 length;

    static auto get(ptr<imm<u8>> unaligned) -> Message {
        return get_unaligned<Message>(unaligned);
    }
};

struct Join_Message {
    Message metadata;

    static auto create() -> Join_Message {
        return { { 0xC0FFEE, Message::Type::join, sizeof(Join_Message) } };
    }

    static auto get(ptr<imm<u8>> unaligned) -> Join_Message {
        return get_unaligned<Join_Message>(unaligned);
    }
};

struct Joined_Message {
    Message metadata;
    State player;
    bool is_new;

    static auto create(State player, bool is_new) -> Joined_Message {
        return { { 0xC0FFEE, Message::Type::joined, sizeof(Joined_Message) }, player, is_new };
    }

    static auto get(ptr<imm<u8>> unaligned) -> Joined_Message {
        return get_unaligned<Joined_Message>(unaligned);
    }
};

struct Left_Message {
    Message metadata;
    u32 id;

    static auto create(u32 id) -> Left_Message {
        return { { 0xC0FFEE, Message::Type::left, sizeof(Left_Message) }, id };
    }

    static auto get(ptr<imm<u8>> unaligned) -> Left_Message {
        return get_unaligned<Left_Message>(unaligned);
    }
};

struct Moving_Message {
    Message metadata;
    u32 id;
    State::Moving direction;

    static auto create(u32 id, State::Moving direction) -> Moving_Message {
        return { { 0xC0FFEE, Message::Type::moving, sizeof(Moving_Message) }, id, direction };
    }

    static auto get(ptr<imm<u8>> unaligned) -> Moving_Message {
        return get_unaligned<Moving_Message>(unaligned);
    }
};

static constexpr usize player_cap = 16;
