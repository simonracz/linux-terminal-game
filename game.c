#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <termios.h>
#include <signal.h>
#include <string.h>

#ifdef RUN_TESTS

#define MAX_X 5
#define MAX_Y 4

#else

#define MAX_X 60
#define MAX_Y 26

#endif

typedef struct {
    int key;
    int pos_x;
    int pos_y;
    unsigned int count; // unsigned, so overflow will be fine
    int gems_collected;
    int dead;
    int won;
    char old_screen[MAX_Y][MAX_X];
    char screen[MAX_Y][MAX_X];
} GameState;

static struct termios old_termios, new_termios;

void reset_terminal() {
    printf("\e[m"); // reset color changes
    printf("\e[?25h"); // show cursor
    printf("\e[%d;%dH\n", MAX_Y + 3, MAX_X); // move cursor after game board
    fflush(stdout);
    tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
}

void configure_terminal() {
    tcgetattr(STDIN_FILENO, &old_termios);
	new_termios = old_termios; // save it to be able to reset on exit
    
    new_termios.c_lflag &= ~(ICANON | ECHO); // turn off echo + non-canonical mode
    new_termios.c_cc[VMIN] = 0;
    new_termios.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

    printf("\e[?25l"); // hide cursor
    atexit(reset_terminal);
}

static int exit_loop;

void signal_handler(__attribute__((unused)) int signum) {
    exit_loop = 1;
}

int read_key(char* buf, int k) {
    if (buf[k] == '\033' && buf[k + 1] == '[') {
		switch (buf[k + 2]) {
			case 'A': return 1; // UP
			case 'B': return 2; // DOWN
			case 'C': return 3; // RIGHT
			case 'D': return 4; // LEFT
		}
	}
	return 0;
}

void read_input(GameState* state) {
   	char buf[4096]; // maximum input buffer
	int n = read(STDIN_FILENO, buf, sizeof(buf));
	int final_key = 0;
    // it's okay if we miss some keys
    // we will correct it on next frame
	for (int k = 0; k <= n - 3; k += 3) {
		int key = read_key(buf, k);
		if (key == 0) continue;
		final_key = key;
	}
    state->key = final_key;
}

void handle_player(GameState* state) {
	switch (state->key) {
	case 1:
        switch (state->screen[state->pos_y - 1][state->pos_x]) {
            case '$':
                state->gems_collected++; // fallthrough
            case ' ':
            case '.':
                state->screen[state->pos_y][state->pos_x] = ' ';
                state->screen[state->pos_y - 1][state->pos_x] = '@';
                --state->pos_y;
                break;
            case 'E':
                state->won = 1;
                break;
            case 'o':
            case 'S':
                state->dead = 1;
                break;
            default:
                break;
        }
        break;
    case 2:
        switch (state->screen[state->pos_y + 1][state->pos_x]) {
            case '$':
                state->gems_collected++; // fallthrough
            case ' ':
            case '.':
                state->screen[state->pos_y][state->pos_x] = ' ';
                state->screen[state->pos_y + 1][state->pos_x] = '@';
                ++state->pos_y;
                break;
            case 'E':
                state->won = 1;
                break;
            default:
                break;
        }
        break;
    case 3:
        switch (state->screen[state->pos_y][state->pos_x + 1]) {
            case '$':
                state->gems_collected++; // fallthrough
            case ' ':
            case '.':
                state->screen[state->pos_y][state->pos_x] = ' ';
                state->screen[state->pos_y][state->pos_x + 1] = '@';
                ++state->pos_x;
                break;
            case 'E':
                state->won = 1;
                break;
            case 'O':
                if (state->screen[state->pos_y][state->pos_x + 2] == ' ') {
                    state->screen[state->pos_y][state->pos_x + 2] = 'O';
                    state->screen[state->pos_y][state->pos_x] = ' ';
                    state->screen[state->pos_y][state->pos_x + 1] = '@';
                    ++state->pos_x;
                }
                break;
            default:
                break;
        }
        break;
    case 4:
        switch (state->screen[state->pos_y][state->pos_x - 1]) {
            case '$':
                state->gems_collected++; // fallthrough
            case ' ':
            case '.':
                state->screen[state->pos_y][state->pos_x] = ' ';
                state->screen[state->pos_y][state->pos_x - 1] = '@';
                --state->pos_x;
                break;
            case 'E':
                state->won = 1;
                break;
            case 'O':
                if (state->screen[state->pos_y][state->pos_x - 2] == ' ') {
                    state->screen[state->pos_y][state->pos_x - 2] = 'O';
                    state->screen[state->pos_y][state->pos_x] = ' ';
                    state->screen[state->pos_y][state->pos_x - 1] = '@';
                    --state->pos_x;
                }
                break;
            default:
                break;
        }
        break;
    default:
        break;
    }
}

void handle_rocks_gems(GameState* state, int x, int y) {
    int gem = state->screen[y][x] == '$';
    if (state->screen[y + 1][x] == ' ') { // start to fall
        if (gem) {
            state->screen[y][x] = 'S';
        } else {
            state->screen[y][x] = 'o';
        }
        return;
    }
    if (state->screen[y + 1][x] == 'O' || state->screen[y + 1][x] == '$') {
        // check left
        if (state->screen[y][x - 1] == ' ' && state->screen[y + 1][x - 1] == ' ') {
            if (gem) {
                state->screen[y][x] = 'S';
            } else {
                state->screen[y][x] = 'o';
            }
        }
        // check right
        if (state->screen[y][x + 1] == ' ' && state->screen[y + 1][x + 1] == ' ') {
            if (gem) {
                state->screen[y][x] = 'S';
            } else {
                state->screen[y][x] = 'o';
            }
        }
    }
}

void handle_falling_rocks_gems(GameState* state, int x, int y) {
    int gem = state->screen[y][x] == 'S';
    if (state->screen[y + 1][x] == ' ') {
        state->screen[y][x] = ' ';
        if (gem) {
            state->screen[y + 1][x] = 'S';
        } else {
            state->screen[y + 1][x] = 'o';
        }
        return;
    }
    if (state->screen[y + 1][x] == 'O' || state->screen[y + 1][x] == '$') {
        // check left
        if (state->screen[y][x - 1] == ' ' && state->screen[y + 1][x - 1] == ' ') {
            state->screen[y][x] = ' ';
            if (gem) {
                state->screen[y][x - 1] = 'p';
            } else {
                state->screen[y][x - 1] = 'i';
            }
            return;
        }
        // check right
        if (state->screen[y][x + 1] == ' ' && state->screen[y + 1][x + 1] == ' ') {
            state->screen[y][x] = ' ';
            if (gem) {
                state->screen[y][x + 1] = 'S';
            } else {
                state->screen[y][x + 1] = 'o';
            }
            return;
        }
    }
    if (state->screen[y + 1][x] == 'o' || state->screen[y + 1][x] == 'S') return;
    if (state->screen[y + 1][x] == '@') {
        state->dead = 1;
    }
    if (gem) {
        state->screen[y][x] = '$';
    } else {
        state->screen[y][x] = 'O';
    }
}

void update_all_elements(GameState* state) {
    // We iterate over the screen from bottom to top from right to left
    for (int j = MAX_Y - 1; j != 0; --j) {
        for (int i = MAX_X - 2; i != 0; --i) {
            switch (state->screen[j][i]) {
                case 'p':
                    state->screen[j][i] = 'S';
                    break;
                case 'i':
                    state->screen[j][i] = 'o';
                    break;
                case 'O':
                case '$':
                    handle_rocks_gems(state, i, j);
                    break;
                case 'o':
                case 'S':
                    handle_falling_rocks_gems(state, i, j);
                    break;
                default:
                    break;
            }
        }
    }
}

void update(GameState* state) {
    memcpy(state->screen, state->old_screen, sizeof(state->screen));
    handle_player(state);
    update_all_elements(state);
    ++state->count;
}

void render(GameState* state) {
    for (int j = 0; j < MAX_Y; ++j) {
        for (int i = 0; i < MAX_X; ++i) {
            char c = state->screen[j][i];
            if (c == '$' || c == 'S') {
                int l = ((state->count + j + 3 * i) % 16) / 8;
                printf("\e[%d;%dH", j + 1, i + 1); // move cursor
                if (l == 0) {
                    printf("\e[48;2;10;10;40m\e[38;2;153;51;255m$");
                } else {
                    printf("\e[48;2;10;10;40m\e[38;12;33;61;255m$");
                }
                continue;
            }
            if (state->old_screen[j][i] != state->screen[j][i]) {
                printf("\e[%d;%dH", j + 1, i + 1); // move cursor
                switch (c) {
                case '\n':
                    printf("\n");
                    break;
                case 'X':
                    printf("\e[48;2;51;51;81m\e[38;2;91;91;91mX");
                    break;
                case '.':
                    printf("\e[48;2;80;76;60m\e[38;2;51;0;25m ");
                    break;
                case ' ':
                    printf("\e[48;2;10;10;40m ");
                    break;
                case 'O':
                    printf("\e[48;2;10;10;40m\e[38;2;202;198;194mO");
                    break;
                case 'o':
                    printf("\e[48;2;10;10;40m\e[38;2;202;198;194mo");
                    break;
                case '@':
                    printf("\e[48;2;10;10;40m\e[38;2;235;51;51m@");
                    break;
                case 'E':
                    printf("\e[48;2;10;10;40m\e[38;2;251;251;15mE");
                default:
                    break;
                }
            }
        }
    }
    fflush(stdout);
}

void find_player_position(GameState* state) {
    for (int j = 0; j < MAX_Y; ++j) {
        for (int i = 0; i < MAX_X; ++i) {
            if (state->screen[j][i] == '@') {
                state->pos_x = i;
                state->pos_y = j;
                return;
            }
        }
    }
}

void load_level(GameState* state) {
    FILE* f = fopen("./level_1.txt", "r");
    if (!f) {
        fprintf(stderr, "Failed to open level_1.txt");
        exit(EXIT_FAILURE);
    }
    if (fread(state->screen, 1, MAX_X * MAX_Y, f) != MAX_X * MAX_Y) {
        fprintf(stderr, "Failed to read level_1.txt");
        fclose(f);
        exit(EXIT_FAILURE);
    }
    render(state); // To display the level
    find_player_position(state);
    memcpy(state->old_screen, state->screen, sizeof(state->screen));
    fclose(f);
}

void print_end_message(GameState* state) {
    printf("\e[%d;%dH", MAX_Y + 2, 1); // move cursor
    if (state->dead) {
        printf("You died! Better luck next time!");
    }
    if (state->won) {
        printf("You won! You collected %d gems!", state->gems_collected);
    }
}

// Lower is faster
#define SPEED 0.1

#ifndef RUN_TESTS

int main() {
    configure_terminal();

    signal(SIGINT, signal_handler);

   	struct timespec req = {};
	struct timespec rem = {};

    GameState state = {
        .pos_x = 5,
        .pos_y = 5
    };

    printf("\e[2J");
    load_level(&state);

    clock_t start, end;

    while (!exit_loop) {
        start = clock();

        read_input(&state);
        update(&state);
        if (state.won || state.dead) {
            print_end_message(&state);
            break;
        }

        render(&state);

        memcpy(state.old_screen, state.screen, sizeof(state.screen));

        end = clock();

        double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
        if (time_taken > SPEED) continue;

        req.tv_sec = 0;
        req.tv_nsec = (SPEED - time_taken) * 1000000000; // 0.1 seconds
        nanosleep(&req, &rem);
    }
}

#endif

#ifdef RUN_TESTS

int test_player_lives() {
    GameState state = {
        .old_screen = {
                {'X', 'X', 'X', 'X', '\n'},
                {'X', 'O', 'O', 'X', '\n'},
                {'X', '@', '.', 'X', '\n'},
                {'X', 'X', 'X', 'X', '\n'}
            },
        .key = 3,
        .pos_x = 1,
        .pos_y = 2,
    };

    char expected1[4][5] = {
        {'X', 'X', 'X', 'X', '\n'},
        {'X', 'o', 'O', 'X', '\n'},
        {'X', ' ', '@', 'X', '\n'},
        {'X', 'X', 'X', 'X', '\n'}
    };

    char expected2[4][5] = {
        {'X', 'X', 'X', 'X', '\n'},
        {'X', ' ', 'O', 'X', '\n'},
        {'X', 'o', '@', 'X', '\n'},
        {'X', 'X', 'X', 'X', '\n'}
    };

    char expected3[4][5] = {
        {'X', 'X', 'X', 'X', '\n'},
        {'X', ' ', 'O', 'X', '\n'},
        {'X', 'O', '@', 'X', '\n'},
        {'X', 'X', 'X', 'X', '\n'}
    };

    update(&state);
    int ret = memcmp(state.screen, expected1, sizeof(expected1));
    if (ret != 0) {
        printf("\e[38;2;250;10;10mState after update() is not equal to expected1\n");
        // memset(state.old_screen, '?', sizeof(state.old_screen));
        // render(&state);
        return 1;
    }
    memcpy(state.old_screen, state.screen, sizeof(state.screen));
    state.key = 0;

    update(&state);
    ret = memcmp(state.screen, expected2, sizeof(expected2));
    if (ret != 0) {
        printf("\e[38;2;250;10;10mState after update() is not equal to expected2\n");
        return 1;
    }
    memcpy(state.old_screen, state.screen, sizeof(state.screen));

    update(&state);
    ret = memcmp(state.screen, expected3, sizeof(expected3));
    if (ret != 0) {
        printf("\e[38;2;250;10;10mState after update() is not equal to expected3\n");
        return 1;
    }
    memcpy(state.old_screen, state.screen, sizeof(state.screen));

    return 0;
}

int test_rock_rolls() {
    GameState state = {
        .old_screen = {
                {'X', 'X', 'X', 'X', '\n'},
                {'X', ' ', 'O', 'X', '\n'},
                {'X', ' ', 'O', 'X', '\n'},
                {'X', 'X', 'X', 'X', '\n'}
            },
        .key = 0,
        .pos_x = 0,
        .pos_y = 0,
    };

    char expected1[4][5] = {
        {'X', 'X', 'X', 'X', '\n'},
        {'X', ' ', 'o', 'X', '\n'},
        {'X', ' ', 'O', 'X', '\n'},
        {'X', 'X', 'X', 'X', '\n'}
    };

    char expected2[4][5] = {
        {'X', 'X', 'X', 'X', '\n'},
        {'X', 'o', ' ', 'X', '\n'},
        {'X', ' ', 'O', 'X', '\n'},
        {'X', 'X', 'X', 'X', '\n'}
    };

    char expected3[4][5] = {
        {'X', 'X', 'X', 'X', '\n'},
        {'X', ' ', ' ', 'X', '\n'},
        {'X', 'o', 'O', 'X', '\n'},
        {'X', 'X', 'X', 'X', '\n'}
    };

    char expected4[4][5] = {
        {'X', 'X', 'X', 'X', '\n'},
        {'X', ' ', ' ', 'X', '\n'},
        {'X', 'O', 'O', 'X', '\n'},
        {'X', 'X', 'X', 'X', '\n'}
    };

    update(&state);
    int ret = memcmp(state.screen, expected1, sizeof(expected1));
    if (ret != 0) {
        printf("\e[38;2;250;10;10mState after update() is not equal to expected1\n");
        // memset(state.old_screen, '?', sizeof(state.old_screen));
        // render(&state);
        return 1;
    }
    memcpy(state.old_screen, state.screen, sizeof(state.screen));

    update(&state);
    ret = memcmp(state.screen, expected2, sizeof(expected2));
    if (ret != 0) {
        printf("\e[38;2;250;10;10mState after update() is not equal to expected2\n");
        memset(state.old_screen, '?', sizeof(state.old_screen));
        render(&state);
        return 1;
    }
    memcpy(state.old_screen, state.screen, sizeof(state.screen));

    update(&state);
    ret = memcmp(state.screen, expected3, sizeof(expected3));
    if (ret != 0) {
        printf("\e[38;2;250;10;10mState after update() is not equal to expected3\n");
        return 1;
    }
    memcpy(state.old_screen, state.screen, sizeof(state.screen));

    update(&state);
    ret = memcmp(state.screen, expected4, sizeof(expected4));
    if (ret != 0) {
        printf("\e[38;2;250;10;10mState after update() is not equal to expected4\n");
        return 1;
    }

    return 0;
}

int main() {
    int tests = 0;
    int evaluation = 0;

    int ret = test_rock_rolls();
    if (ret != 0) {
        printf("\e[38;2;250;10;10mTest Rock Falls - Failed\n");
    } else {
        printf("\e[38;2;10;250;10mTest Rock Falls - Successful\n");
    }
    evaluation += ret;
    ++tests;

    ret = test_player_lives();
    if (ret != 0) {
        printf("\e[38;2;250;10;10mTest Player Lives - Failed\n");
    } else {
        printf("\e[38;2;10;250;10mTest Player Lives - Successful\n");
    }
    evaluation += ret;
    ++tests;

    if (evaluation == 0) {
        printf("\e[38;2;10;250;10mALL %d test were Successful\n", tests);
    } else {
        printf("\e[38;2;250;10;10mTests Failed\n");
    }

    printf("\e[m"); // reset changes
}

#endif


























