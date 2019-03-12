#include <assert.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <random>
#include <vector>

class NonblockTerm {
public:
    NonblockTerm() {
        // Backup default terminal
        struct termios term;
        tcgetattr(fileno(stdin), &term);
        m_default_term = term;

        // Set nonblocking terminal
        term.c_lflag &= ~(ECHO | ICANON);  // no echo back, non cannonical
        term.c_cc[VTIME] = 0;
        term.c_cc[VMIN] = 1;
        tcsetattr(fileno(stdin), TCSANOW, &term);
        fcntl(0, F_SETFL, O_NONBLOCK);
    }

    ~NonblockTerm() {
        // Restore terminal
        tcsetattr(fileno(stdin), TCSANOW, &m_default_term);
    }

    bool getKey(char& key) {
        // Return true when `c` is valid.
        key = getchar();
        return (key != EOF);
    }

private:
    struct termios m_default_term;
};

class FpsStabler {
public:
    FpsStabler(double fps)
        : INTERVAL_US(1000.0 * 1000.0 / fps),
          m_prev_time(std ::chrono::system_clock::now()) {}

    void sleep() {
        const auto curr_time = std::chrono::system_clock::now();
        const double elapsed_us =
            std::chrono::duration_cast<std::chrono::microseconds>(curr_time -
                                                                  m_prev_time)
                .count();
        m_prev_time = curr_time;
        const double sleep_us = INTERVAL_US - elapsed_us;
        if (0.0 < sleep_us) {
            usleep(sleep_us);
        }
    }

private:
    const double INTERVAL_US;
    std::chrono::system_clock::time_point m_prev_time;
};

class EventClock {
public:
    EventClock(double fps)
        : INTERVAL_US(1000.0 * 1000.0 / fps),
          m_prev_time(std ::chrono::system_clock::now()),
          m_prev_rest(0.0) {}

    bool shouldHappen() {
        const auto curr_time = std::chrono::system_clock::now();
        const double elapsed_us =
            std::chrono::duration_cast<std::chrono::microseconds>(curr_time -
                                                                  m_prev_time)
                .count();
        const double curr_rest = (elapsed_us + m_prev_rest) - INTERVAL_US;
        if (0.0 < curr_rest) {
            m_prev_time = curr_time;
            m_prev_rest = curr_rest;
            return true;
        } else {
            return false;
        }
    }

private:
    const double INTERVAL_US;
    std::chrono::system_clock::time_point m_prev_time;
    double m_prev_rest;
};

enum class Color : char {
    BLACK = 0,
    RED,
    GREEN,
    ORANGE,
    BLUE,
    PURPLE,
    LIGHT_BLUE,
    WHITE
};

class Block {
public:
    Block() {}
    virtual ~Block() {}
    virtual std::shared_ptr<Block> clone() const = 0;

    void move(int dx, int dy) { setPos(m_x + dx, m_y + dy); }
    void setPos(int x, int y) {
        m_x = x;
        m_y = y;
    }
    void rotate() {
        m_rot = static_cast<Rot>((static_cast<int>(m_rot) + 1) % 4);
    }

    virtual void getRange(int* sx, int* sy, int* ex, int* ey) const = 0;
    virtual bool exist(int x, int y) const = 0;
    virtual Color getColor() const = 0;

protected:
    enum Rot { ROT0, ROT90, ROT180, ROT270 };

    int m_x, m_y;
    Rot m_rot = ROT0;
};

struct BlockShape0 {
    constexpr static int W = 4;
    constexpr static int H = 4;
    constexpr static char SHAPE[H][W] = {
        {0, 0, 0, 0},
        {1, 1, 1, 1},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
    };
    constexpr static Color COL = Color::LIGHT_BLUE;
};

struct BlockShape1 {
    constexpr static int W = 3;
    constexpr static int H = 3;
    constexpr static char SHAPE[H][W] = {
        {1, 0, 0},
        {1, 1, 1},
        {0, 0, 0},
    };
    constexpr static Color COL = Color::BLUE;
};

struct BlockShape2 {
    constexpr static int W = 3;
    constexpr static int H = 3;
    constexpr static char SHAPE[H][W] = {
        {0, 0, 1},
        {1, 1, 1},
        {0, 0, 0},
    };
    constexpr static Color COL = Color::ORANGE;
};

struct BlockShape3 {
    constexpr static int W = 2;
    constexpr static int H = 2;
    constexpr static char SHAPE[H][W] = {
        {1, 1},
        {1, 1},
    };
    constexpr static Color COL = Color::WHITE;
};

struct BlockShape4 {
    constexpr static int W = 3;
    constexpr static int H = 3;
    constexpr static char SHAPE[H][W] = {
        {0, 1, 1},
        {1, 1, 0},
        {0, 0, 0},
    };
    constexpr static Color COL = Color::GREEN;
};

struct BlockShape5 {
    constexpr static int W = 3;
    constexpr static int H = 3;
    constexpr static char SHAPE[H][W] = {
        {0, 1, 0},
        {1, 1, 1},
        {0, 0, 0},
    };
    constexpr static Color COL = Color::PURPLE;
};

struct BlockShape6 {
    constexpr static int W = 3;
    constexpr static int H = 3;
    constexpr static char SHAPE[H][W] = {
        {1, 1, 0},
        {0, 1, 1},
        {0, 0, 0},
    };
    constexpr static Color COL = Color::RED;
};

template <typename T>
class BlockImpl : public Block {
public:
    using Shape = T;

    BlockImpl() {}
    virtual ~BlockImpl() {}
    virtual std::shared_ptr<Block> clone() const {
        return std::make_shared<BlockImpl<T> >(*this);
    }

    virtual void getRange(int* sx, int* sy, int* ex, int* ey) const {
        constexpr int sx0 = -W_L;
        constexpr int sy0 = -H_L;
        constexpr int ex0 = W_R - 1;
        constexpr int ey0 = H_R - 1;
        if (m_rot == ROT0 || m_rot == ROT180) {
            (*sx) = m_x + sx0;
            (*sy) = m_y + sy0;
            (*ex) = m_x + ex0;
            (*ey) = m_y + ey0;
        } else {
            (*sx) = m_x + sy0;
            (*sy) = m_y + sx0;
            (*ex) = m_x + ey0;
            (*ey) = m_y + ex0;
        }
    }

    virtual bool exist(int x, int y) const {
        const int x0_idx = x - (m_x - W_L);
        const int y0_idx = y - (m_y - H_L);
        if (m_rot == ROT0) {
            return Shape::SHAPE[y0_idx][x0_idx];
        } else if (m_rot == ROT90) {
            return Shape::SHAPE[x0_idx][Shape::H - y0_idx - 1];
        } else if (m_rot == ROT180) {
            return Shape::SHAPE[Shape::H - y0_idx - 1][Shape::W - x0_idx - 1];
        } else if (m_rot == ROT270) {
            return Shape::SHAPE[Shape::W - x0_idx - 1][y0_idx];
        }
        assert(false);
        return false;
    }

    virtual Color getColor() const { return Shape::COL; }

protected:
    constexpr static int W_L = Shape::W / 2;
    constexpr static int W_R = Shape::W - W_L;
    constexpr static int H_L = Shape::H / 2;
    constexpr static int H_R = Shape::H - H_L;
};

class RandomBlockGenerator {
public:
    RandomBlockGenerator(int x, int y) : m_x(x), m_y(y), m_mt(m_rnd()) {}
    std::shared_ptr<Block> operator()() {
        const int idx = m_mt() % 7;
        std::shared_ptr<Block> block;
        if (idx == 0) {
            block = std::make_shared<BlockImpl<BlockShape0> >();
        } else if (idx == 1) {
            block = std::make_shared<BlockImpl<BlockShape1> >();
        } else if (idx == 2) {
            block = std::make_shared<BlockImpl<BlockShape2> >();
        } else if (idx == 3) {
            block = std::make_shared<BlockImpl<BlockShape3> >();
        } else if (idx == 4) {
            block = std::make_shared<BlockImpl<BlockShape4> >();
        } else if (idx == 5) {
            block = std::make_shared<BlockImpl<BlockShape5> >();
        } else if (idx == 6) {
            block = std::make_shared<BlockImpl<BlockShape6> >();
        } else {
            assert(false);
        }
        // Set default position
        block->setPos(m_x, m_y);
        return block;
    }

private:
    int m_x, m_y;
    std::random_device m_rnd;
    std::mt19937 m_mt;
};

void ClearScreen() {
    std::cout << "\e[2J";    // Clear all
    std::cout << "\e[0;0H";  // Go to top left
}

void PrintColorCode(Color col) {
    std::cout << "\e[4" << static_cast<int>(col) << "m";
}

void ResetColorCode() { std::cout << "\e[0m"; }

class BlockMap {
public:
    BlockMap() {}
    BlockMap(size_t w, size_t h)
        : m_w(w), m_h(h), m_blocks(w * h, Color::BLACK) {}

    bool exist(int x, int y) const { return static_cast<bool>(get(x, y)); }

    bool isPuttable(const Block& block) const {
        // Existing range
        int sx, sy, ex, ey;
        block.getRange(&sx, &sy, &ex, &ey);

        // Check field range and block's overlapping
        for (int y = sy; y <= ey; y++) {
            for (int x = sx; x <= ex; x++) {
                if (!block.exist(x, y)) {
                    continue;
                }
                // Ignore -Y
                if (y < 0) {
                    continue;
                }
                // Check
                if (x < 0 || m_w <= x || m_h <= y || this->exist(x, y)) {
                    return false;
                }
            }
        }
        return true;
    }

    void putBlock(const Block& block) {
        // Existing range
        int sx, sy, ex, ey;
        block.getRange(&sx, &sy, &ex, &ey);
        sx = std::max(0, sx);
        sy = std::max(0, sy);
        ex = std::min(static_cast<int>(m_w), ex);
        ey = std::min(static_cast<int>(m_h), ey);

        // Put
        const Color col = block.getColor();
        for (size_t y = sy; y <= ey; y++) {
            for (size_t x = sx; x <= ex; x++) {
                if (block.exist(x, y)) {
                    get(x, y) = col;
                }
            }
        }
    }

    int eraseFilledLines() {
        int n_erased_lines = 0;
        for (int y = m_h - 1; 0 <= y; y--) {
            // Check is the line filled
            bool is_filled = true;
            for (size_t x = 0; x < m_w; x++) {
                if (get(x, y) == Color::BLACK) {
                    is_filled = false;
                    break;
                }
            }
            // Erase the line
            if (is_filled) {
                n_erased_lines++;
                // Shift
                for (size_t yy = y; 1 <= yy; yy--) {
                    for (size_t x = 0; x < m_w; x++) {
                        get(x, yy) = get(x, yy - 1);
                    }
                }
                // Last line
                for (size_t x = 0; x < m_w; x++) {
                    get(x, 0) = Color::BLACK;
                }
            }
        }
        return n_erased_lines;
    }

    void draw() {
        ClearScreen();

        // Top wall
        printWall(m_w + 2);
        newline();

        // Print all blocks
        size_t idx = 0;
        for (size_t y = 0; y < m_h; y++) {
            // Left wall
            printWall(1);
            for (size_t x = 0; x < m_w; x++, idx++) {
                // Write one block
                PrintColorCode(m_blocks[idx]);
                std::cout << "　";
            }
            // Right wall
            printWall(1);
            newline();
        }

        // Button wall
        printWall(m_w + 2);
        newline();
    }

private:
    const Color& get(int x, int y) const { return m_blocks[y * m_w + x]; }

    Color& get(int x, int y) {
        return const_cast<Color&>(
            (static_cast<const BlockMap*>(this)->get(x, y)));
    }

    void printWall(size_t n) {
        // Print white wall
        PrintColorCode(Color::WHITE);
        for (size_t x = 0; x < n; x++) {
            std::cout << "　";
        }
    }

    void newline() {
        // Reset color for the rest of line
        ResetColorCode();
        std::cout << std::endl;
    }

    size_t m_w, m_h;
    std::vector<Color> m_blocks;
};

template <typename Action, typename... Args>
bool TryBlockAction(Block& block, const BlockMap& block_map, Action action,
                    Args&&... args) {
    // Create clone and try to act
    std::shared_ptr<Block> block_tmp = block.clone();
    ((*block_tmp).*action)(std::forward<Args>(args)...);
    // Check
    if (block_map.isPuttable(*block_tmp)) {
        // Overwrite the original one
        block = *block_tmp;
        return true;
    } else {
        return false;
    }
}

class TetrisApp {
public:
    TetrisApp(size_t w, size_t h, double fps = 15.f)
        : m_block_map(w, h),
          m_rand_block_gen(w / 2, 0),
          m_fps_stabler(fps),
          m_event_clock(1.f) {}

    void run() {
        int n_erased_lines = 0;
        bool next_block = false;
        std::shared_ptr<Block> block = m_rand_block_gen();
        while (true) {
            if (next_block) {
                next_block = false;
                block = m_rand_block_gen();
                // Check whether game over
                if (!m_block_map.isPuttable(*block)) {
                    std::cout << "Game Over" << std::endl;
                    break;
                }
            }

            if (m_event_clock.shouldHappen()) {
                // Go down
                next_block =
                    !TryBlockAction(*block, m_block_map, &Block::move, 0, 1);
                // Check filled lines
                n_erased_lines += m_block_map.eraseFilledLines();
            }

            // Draw screen
            auto block_map_tmp = m_block_map;
            block_map_tmp.putBlock(*block);
            block_map_tmp.draw();
            if (next_block) {
                // Restore screen
                m_block_map = block_map_tmp;
            }

            // Check key input
            char key;
            if (m_nonblock_term.getKey(key)) {
                if (key == ' ' || key == 'r') {
                    TryBlockAction(*block, m_block_map, &Block::rotate);
                }
                if (key == 'h') {
                    // Left
                    TryBlockAction(*block, m_block_map, &Block::move, -1, 0);
                }
                if (key == 'l') {
                    // Right
                    TryBlockAction(*block, m_block_map, &Block::move, 1, 0);
                }
                if (key == 'j') {
                    // Down
                    TryBlockAction(*block, m_block_map, &Block::move, 0, 1);
                }
                if (key == 'q') {
                    break;
                }
            }

            // Sleep
            m_fps_stabler.sleep();
        }
    }

private:
    NonblockTerm m_nonblock_term;
    BlockMap m_block_map;
    RandomBlockGenerator m_rand_block_gen;
    FpsStabler m_fps_stabler;
    EventClock m_event_clock;
};

int main() {
    TetrisApp app(11, 20);
    app.run();

    return 0;
}
