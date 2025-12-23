#include "main.h"
#include "board.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

uint32_t random_seed = 0xdeadc0de;

void set_random_seed(uint32_t seed) {
    random_seed = seed;
}

static inline uint32_t get_nth_bit(uint32_t num, uint8_t n) {
    return (num >> n) & 1;
}

uint32_t gen_rand(void) {
    uint32_t feedback = get_nth_bit(random_seed, 31) ^ get_nth_bit(random_seed, 21) ^ get_nth_bit(random_seed, 1) ^ get_nth_bit(random_seed, 0);
    random_seed >>= 1;
    random_seed |= feedback<<31;
    return random_seed;
}

void write_stdout(uint8_t data) {
    REG_UART_0_DATA = data;
}

uint8_t read_stdin(void) {
    int32_t c = -1;
    while (c == -1) {
        c = REG_UART_0_DATA;
    }
    REG_UART_0_DATA = c;
    return c;
}

void write_str(char *str) {
    for (int i=0; str[i]; ++i) {
        write_stdout((uint8_t)str[i]);
    }
}

uint32_t read_decimal_number(void) {
    uint32_t num = 0;
    uint8_t digit = 0;
    while (digit<'0' || digit>'9') {
        digit = read_stdin();
    }
    while (digit>='0' && digit<='9') {
        num *= 10;
        num += digit - '0';
        digit = read_stdin();
    }
    return num;
}

void write_decimal_number(uint32_t n, unsigned width) {
    int num_of_digits = 1;
    uint32_t denominator = 1;
    while (n >= denominator*10) {
        num_of_digits += 1;
        denominator *= 10;
    }
    for (int i=0; i<(int)width-num_of_digits; ++i) {
        write_stdout(' ');
    }
    while (denominator > 0) {
        write_stdout(n/denominator + '0');
        n %= denominator;
        denominator /= 10;
    }
}

typedef struct block_t {
    bool is_mine;
    bool is_opened; 
    bool is_marked;
    uint8_t nearby_mines;
} block_t;

typedef struct minefield_t {
    uint16_t height;
    uint16_t width;
    uint32_t num_of_mines;
    block_t *blocks;
    uint32_t opened_blocks;
    bool is_lost;
} minefield_t;


block_t *get_block(minefield_t *minefield, uint16_t row, uint16_t col) {
    // in C, overflow behavior of unsigned values are defined
    if (row>=minefield->height || col>=minefield->width) {
        return NULL;
    }
    uint32_t offset = row*minefield->width + col;
    return minefield->blocks + offset;
}

void init_minefield(minefield_t *minefield) {
    // make it empty
    uint32_t num_of_blocks = minefield->width * minefield->height;
    minefield->opened_blocks = 0;
    minefield->is_lost = false;
    for (uint32_t i=0; i<num_of_blocks; ++i) {
        minefield->blocks[i].is_mine = false;
        minefield->blocks[i].is_opened = false;
        minefield->blocks[i].is_marked = false;
        minefield->blocks[i].nearby_mines = 0;
    }

    // genetate mines
    uint32_t current_num_of_mines = 0;
    while (current_num_of_mines < minefield->num_of_mines) {
        block_t *block = minefield->blocks + gen_rand()%num_of_blocks;
        if (!block->is_mine) {
            block->is_mine = true;
            current_num_of_mines += 1;
        }
    }

    // count the nearby mines of all blocks
    for (int r=0; r<minefield->height; ++r) {
        for (int c=0; c<minefield->width; ++c) {
            block_t *current_block = get_block(minefield, r, c);
            for (int i=-1; i<=1; ++i) {
                for (int j=-1; j<=1; ++j) {
                    block_t *neighbor = get_block(minefield, r+i, c+j);
                    if (neighbor != NULL) {
                        if (neighbor->is_mine) {
                            current_block->nearby_mines += 1;
                        }
                    }
                }
            }
        }
    }
}

void open_block(minefield_t *minefield, uint16_t row, uint16_t col) {
    block_t *block = get_block(minefield, row, col);
    if (block == NULL) {
        return;
    }
    if (block->is_opened) {
        return;
    }
    block->is_opened = true;
    if (block->nearby_mines == 0) {
        for (int i=-1; i<=1; ++i) {
            for (int j=-1; j<=1; ++j) {
                open_block(minefield, row+i, col+j);
            }
        }
    }
    if (block->is_mine) {
        minefield->is_lost = true;
    }
    minefield->opened_blocks += 1;
}

void mark_block(minefield_t *minefield, uint16_t row, uint16_t col) {
    block_t *block = get_block(minefield, row, col);
    if (block != NULL) {
        block->is_marked = true;
    }
}

void unmark_block(minefield_t *minefield, uint16_t row, uint16_t col) {
    block_t *block = get_block(minefield, row, col);
    if (block != NULL) {
        block->is_marked = false;
    }
}

bool is_beaten(minefield_t *minefield) {
    uint32_t num_of_blockes = minefield->height*minefield->width;
    if (minefield->is_lost || num_of_blockes==0) {
        return false;
    } else {
        return minefield->opened_blocks + minefield->num_of_mines == num_of_blockes;
    }
}

bool is_lost(minefield_t *minefield) {
    return minefield->is_lost;
}

void print_help(void) {
    write_str("Welcome to minesweeper on ECOS SoC!\r\n");
    write_str("n <height> <width> <n_mines>: start a new game\r\n");
    write_str("o <row> <col>: open a block\r\n");
    write_str("m <row> <col>: mark a block\r\n");
    write_str("u <row> <col>: unmark a block\r\n");
    write_str("q: quite game\r\n");
}

void put_spaces(unsigned n) {
    for (unsigned i=0; i<n; ++i) {
        write_stdout(' ');
    }
}

void output_minefield(minefield_t *minefield, unsigned width) {
    write_str("\r\n");
    put_spaces(width);
    for (uint16_t i=0; i<minefield->width; ++i) {
        write_decimal_number(i, width);
    }
    write_str("\r\n");
    for (uint16_t r=0; r<minefield->height; ++r) {
        write_decimal_number(r, width);
        for (uint16_t c=0; c<minefield->width; ++c) {
            block_t *block = get_block(minefield, r, c);
            if (block->is_opened) {
                if (block->is_mine) {
                    put_spaces(width-1);
                    write_stdout('*');
                } else {
                    write_decimal_number(block->nearby_mines, width);
                }
            } else if (block->is_marked) {
                put_spaces(width-1);
                write_stdout('!');
            } else {
                put_spaces(width-1);
                write_stdout('-');
            }
        }
        write_str("\r\n");
    }

    if (is_beaten(minefield)) {
        write_str("You win!\r\n");
    }
    if (is_lost(minefield)) {
        write_str("You lost!\r\n");
    }
}

typedef enum state_t {NEW_GAME, IN_GAME, GAME_OVER} state_t;

#define MAX_BLOCKS 128u

int main(void) {
    sys_uart_init();
    state_t state = GAME_OVER;
    state_t next_state;
    // user input
    uint8_t command;
    bool new_game = false;
    unsigned height, width, num_of_mines;

    minefield_t minefield;
    block_t blocks[MAX_BLOCKS];

    print_help();

    while (true) {
        switch (state) {
            case NEW_GAME:
                minefield.height = height;
                minefield.width = width;
                minefield.num_of_mines = num_of_mines;
                minefield.blocks = blocks;
                init_minefield(&minefield);
                new_game = false;
                output_minefield(&minefield, 3);
                break;
            case IN_GAME:
                switch (command) {
                    case 'o':
                        open_block(&minefield, height, width);
                        break;
                    case 'm':
                        mark_block(&minefield, height, width);
                        break;
                    case 'u':
                        unmark_block(&minefield, height, width);
                        break;
                    default:
                        break;
                }
                output_minefield(&minefield, 3);
                break;
            case GAME_OVER:
                write_str("use command `n` to start a new game, or use command `q` to quite game.\r\n");
                break;
        }

        // parse user input
        write_str("> ");
        command = read_stdin();
        switch (command) {
            case 'n':
                height = read_decimal_number();
                width = read_decimal_number();
                num_of_mines = read_decimal_number();
                if (width*height > MAX_BLOCKS) {
                    write_str("Maximum number of blocks is ");
                    write_decimal_number(MAX_BLOCKS, 0);
                    write_str("\r\n");
                    new_game = false;
                } else if (num_of_mines > width*height/2) {
                    write_str("Number of mines cannot exceed half of the number of blocks.\r\n");
                    new_game = false;
                } else {
                    new_game = true;
                }
                break;
            case 'o':
            case 'm':
            case 'u':
                new_game = false;
                height = read_decimal_number();
                width = read_decimal_number();
                break;
            case 'q':
                return 0;
                break;
            default:
                break;
        }

        // decide the next state
        switch (state) {
            case NEW_GAME:
                next_state = IN_GAME;
                break;
            case IN_GAME:
                if (is_beaten(&minefield) || is_lost(&minefield)) {
                    next_state = GAME_OVER;
                } else if (new_game) {
                    next_state = NEW_GAME;
                } else {
                    next_state = IN_GAME;
                }
                break;
            case GAME_OVER:
                if (new_game) {
                    next_state = NEW_GAME;
                } else {
                    next_state = GAME_OVER;
                }
                break;
        }
        state = next_state;
    }

    return 0;
}
