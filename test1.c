#include <ncurses.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>

#define MAX_USERS 100
#define MAX_USERNAME_LEN 50
#define MAX_PASSWORD_LEN 50
#define MAX_EMAIL_LEN 100
#define MAX_MSG_LENGTH 100
#define MSG_AREA_HEIGHT 3

#define MAX_MAP_SIZE 20
#define MAX_ROOMS 10

typedef struct {
    char type;  // 'W' for wall, '.' for floor, '+' for door, '#' for corridor, etc.
    int visible;  // 0 for not visible, 1 for visible
} Cell;

typedef struct {
    int x, y;  // Position of the room
    int width, height;  // Size of the room
} Room;

typedef struct {
    Cell map[MAX_MAP_SIZE][MAX_MAP_SIZE];  // نقشه بازی
    Room rooms[MAX_ROOMS];  // اطلاعات اتاق‌ها
    int num_rooms;  // تعداد اتاق‌ها
    int player_x, player_y;  // موقعیت بازیکن
} GameMap;


typedef struct {
    char username[MAX_USERNAME_LEN];   // نام کاربری
    char password[MAX_PASSWORD_LEN];   // رمز عبور
    char email[MAX_EMAIL_LEN];         // ایمیل
    int score;                         // امتیاز
    int gold;                          // طلا
    int games;                         // تعداد بازی‌های انجام‌شده
    time_t experience_days;            // تاریخ ثبت‌نام (زمانی که کاربر ثبت‌نام کرده است)
    int difficulty;                    // سطح دشواری
    int music;                         // وضعیت موزیک
    int theme;                         // رنگ تم
    GameMap game_map;                  // نقشه بازی
} User;

void pre_game_menu(User *user);  // منوی پیش از بازی
void start_new_game(User *user);  // شروع یک بازی جدید با اطلاعات کاربر
void continue_previous_game(User *user);  // ادامه بازی قبلی با اطلاعات کاربر
void show_leaderboard(const char *current_user);  // نمایش صفحه رتبه‌بندی
void settings_menu(User *user);  // منوی تنظیمات
void show_profile(User *user);  // نمایش پروفایل کاربر
void change_difficulty(User *user);  // تغییر سطح دشواری برای کاربر
 void move_player(User *user, int dx, int dy);  // حرکت بازیکن در نقشه
void save_game(User *user);  // ذخیره وضعیت بازی
 void load_game(User *user);  // بارگذاری وضعیت بازی
void cleanup_game();  // تمیز کردن و خاتمه بازی
void save_user_game_state(User *user);  // ذخیره وضعیت بازی کاربر
void load_user_game_state(User *user); 
void draw_map(User *user);
void connect_rooms_with_corridors(GameMap *game_map);
void add_doors_and_windows(GameMap *game_map);
void connect_rooms_with_corridors(GameMap *game_map) {
    for (int i = 1; i < game_map->num_rooms; i++) {
        Room *prev = &(game_map->rooms[i - 1]);
        Room *curr = &(game_map->rooms[i]);

        // نقطه شروع و پایان راهرو
        int start_x = prev->x + prev->width / 2;
        int start_y = prev->y + prev->height / 2;
        int end_x = curr->x + curr->width / 2;
        int end_y = curr->y + curr->height / 2;

        // ساخت راهرو افقی
        for (int x = (start_x < end_x ? start_x : end_x); x <= (start_x > end_x ? start_x : end_x); x++) {
            game_map->map[x][start_y].type = '#';
        }

        // ساخت راهرو عمودی
        for (int y = (start_y < end_y ? start_y : end_y); y <= (start_y > end_y ? start_y : end_y); y++) {
            game_map->map[end_x][y].type = '#';
        }
    }
}
void add_doors_and_windows(GameMap *game_map) {
    for (int i = 0; i < game_map->num_rooms; i++) {
        Room *room = &(game_map->rooms[i]);

        // افزودن یک در به دیوار پایین
        int door_x = room->x + (rand() % room->width);
        int door_y = room->y + room->height - 1;
        game_map->map[door_x][door_y].type = '+';

        // بررسی مجاورت برای پنجره
        for (int j = 0; j < game_map->num_rooms; j++) {
            if (i == j) continue;  // اتاق به خودش متصل نباشد
            Room *neighbor = &(game_map->rooms[j]);

            // بررسی مجاورت در جهت‌های مختلف
            if (abs(room->x - neighbor->x) <= 1 || abs(room->y - neighbor->y) <= 1) {
                // افزودن پنجره در دیوار مجاور
                for (int x = room->x; x < room->x + room->width; x++) {
                    for (int y = room->y; y < room->y + room->height; y++) {
                        if (x == neighbor->x || y == neighbor->y) {
                            game_map->map[x][y].type = '=';  // پنجره
                        }
                    }
                }
            }
        }
    }
}

void handle_game_input(User *user) {
    int ch = getch();

    switch (ch) {
        case KEY_UP:
            move_player(user, -1, 0);  // Move up
            break;
        case KEY_DOWN:
            move_player(user, 1, 0);  // Move down
            break;
        case KEY_LEFT:
            move_player(user, 0, -1);  // Move left
            break;
        case KEY_RIGHT:
            move_player(user, 0, 1);  // Move right
            break;
        case 's':
            save_game(user);  // Save game state
            break;
        case 'l':
            load_game(user);  // Load game state
            break;
        case 'q':
            cleanup_game();  // Quit game
            exit(0);
            break;
    }
}

void save_settings(User *user) {
    // ایجاد نام فایل با استفاده از نام کاربری
    char filename[100];
    snprintf(filename, sizeof(filename), "%s_settings.txt", user->username);

    FILE *file = fopen(filename, "w");  // باز کردن فایل برای نوشتن
    if (file == NULL) {
        perror("Unable to open settings file");
        return;
    }

    // ذخیره تنظیمات کاربر در فایل
    fprintf(file, "%s %d %d %d %d %d\n", 
            user->username, 
            user->difficulty, 
            user->music, 
            user->score, 
            user->gold, 
            user->theme);
    
    fclose(file);
}

const char *music_options[] = {
    "Music 1",
    "Music 2",
    "Music 3",
    "No Music"
};
int current_music = 0;  // شاخص موزیک فعلی

User users[MAX_USERS];
User current_user;  // ساختار کاربر فعلی

int user_count = 0;
void generate_map(GameMap *game_map) {
    // مقداردهی اولیه نقشه
    for (int i = 0; i < MAX_MAP_SIZE; i++) {
        for (int j = 0; j < MAX_MAP_SIZE; j++) {
            game_map->map[i][j].type = 'W';  // دیوار
            game_map->map[i][j].visible = 0;  // نامرئی
        }
    }

    // تولید اتاق‌ها
    game_map->num_rooms = 6 + rand() % (MAX_ROOMS - 6);  // حداقل ۶ اتاق
    for (int r = 0; r < game_map->num_rooms; r++) {
        Room *room = &(game_map->rooms[r]);
        room->x = rand() % (MAX_MAP_SIZE - 6) + 1;  // محدودیت به نقشه
        room->y = rand() % (MAX_MAP_SIZE - 6) + 1;
        room->width = 4 + rand() % 4;  // اندازه اتاق‌ها
        room->height = 4 + rand() % 4;

        for (int i = room->x; i < room->x + room->width; i++) {
            for (int j = room->y; j < room->y + room->height; j++) {
                if (i == room->x || i == room->x + room->width - 1 ||
                    j == room->y || j == room->y + room->height - 1) {
                    game_map->map[i][j].type = 'W';  // دیوار
                } else {
                    game_map->map[i][j].type = '.';  // کف
                }
            }
        }
    }

    // افزودن درها و پنجره‌ها
    add_doors_and_windows(game_map);

    // اتصال اتاق‌ها با راهروها
    connect_rooms_with_corridors(game_map);
}

void update_visibility(GameMap *game_map, int player_x, int player_y) {
    for (int i = 0; i < game_map->num_rooms; i++) {
        Room room = game_map->rooms[i];

        // اگر بازیکن داخل اتاق باشد
        if (player_x >= room.x && player_x < room.x + room.width &&
            player_y >= room.y && player_y < room.y + room.height) {

            // نمایش کاشی‌های اتاق جاری
            for (int x = room.x; x < room.x + room.width; x++) {
                for (int y = room.y; y < room.y + room.height; y++) {
                    game_map->map[x][y].visible = 1;
                }
            }

            // بررسی اتاق‌های مجاور برای پنجره‌ها
            for (int j = 0; j < game_map->num_rooms; j++) {
                if (i == j) continue;  // از بررسی خود اتاق صرف‌نظر کنید

                Room neighbor = game_map->rooms[j];
                if (abs(room.x - neighbor.x) <= 1 || abs(room.y - neighbor.y) <= 1) {
                    // نمایش پنجره‌ها در اتاق مجاور
                    for (int x = neighbor.x; x < neighbor.x + neighbor.width; x++) {
                        for (int y = neighbor.y; y < neighbor.y + neighbor.height; y++) {
                            if (game_map->map[x][y].type == '=') {
                                game_map->map[x][y].visible = 1;  // نمایش پنجره
                            }
                        }
                    }
                }
            }
        }
    }
}


void load_settings(User *settings) {
    // بارگذاری نقشه از فایل
    char filename[100];
    snprintf(filename, sizeof(filename), "%s_settings.txt", settings->username);

    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Unable to open settings file");
        return;
    }

    // بارگذاری تنظیمات کاربر
    fscanf(file, "%s %d %d %d %d %d\n", 
           settings->username, 
           &settings->difficulty, 
           &settings->music, 
           &settings->score, 
           &settings->gold, 
           &settings->theme);

    // بارگذاری نقشه بازی
    for (int i = 0; i < MAX_MAP_SIZE; i++) {
        for (int j = 0; j < MAX_MAP_SIZE; j++) {
            settings->game_map.map[i][j].type = fgetc(file);
        }
        fgetc(file);  // خواندن کاراکتر جدید خط
    }

    // بارگذاری موقعیت بازیکن
    fscanf(file, "%d %d\n", &settings->game_map.player_x, &settings->game_map.player_y);

    fclose(file);
}

void draw_frame(const char *title) {
    clear();
    box(stdscr, 0, 0);  // Draw the border
    attron(COLOR_PAIR(5) | A_BOLD);  // Set title color and bold text
    mvprintw(0, (COLS - strlen(title)) / 2, "%s", title);  // Center the title
    attroff(COLOR_PAIR(5) | A_BOLD);
}
void highlight_option(int y, const char *text, int highlighted) {
    if (highlighted) {
        attron(COLOR_PAIR(2) | A_BOLD | A_REVERSE);  // Highlight with reverse video
        mvprintw(y, (COLS - strlen(text)) / 2, "%s", text);  // Center the option
        attroff(COLOR_PAIR(2) | A_BOLD | A_REVERSE);
    } else {
        mvprintw(y, (COLS - strlen(text)) / 2, "%s", text);  // Center the option
    }
}
void draw_menu(int choice) {
    draw_frame("Main Menu");

    const char *options[] = {
        "Create a New User",
        "Enter as Existing User",
        "Enter as a Guest",
        "Exit"
    };

    int num_options = sizeof(options) / sizeof(options[0]);
    int start_y = 4;  // Starting Y position for menu items

    for (int i = 0; i < num_options; i++) {
        highlight_option(start_y + i * 2, options[i], (i + 1 == choice));
    }

    refresh();
}


void setup_game() {
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    start_color();  // Enable color support

    // Define color pairs
    init_pair(1, COLOR_WHITE, COLOR_BLACK);   // Default
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);  // Highlighted options
    init_pair(3, COLOR_GREEN, COLOR_BLACK);   // Success messages
    init_pair(4, COLOR_RED, COLOR_BLACK);     // Error messages
    init_pair(5, COLOR_CYAN, COLOR_BLACK);    // Titles and frames
}

void cleanup_game() {
    
    endwin();
}


void load_users() {
    FILE *file = fopen("users.txt", "r");
    if (file) {
        user_count = 0;
        while (fscanf(file, "%s\n%s\n%s\n", users[user_count].username, users[user_count].password, users[user_count].email) == 3) {
            user_count++;
        }
        fclose(file);
    }
}


int handle_menu() {
    int choice = 1;
    int ch;

    while (1) {
        draw_menu(choice);  // Draw menu with the current choice
        ch = getch();

        if (ch == KEY_DOWN) {
            choice = (choice % 4) + 1;  // Cycle through options 1 -> 4 -> 1
        } else if (ch == KEY_UP) {
            choice = (choice == 1) ? 4 : choice - 1;  // Cycle through options 4 -> 1 -> 4
        } else if (ch == 10) {  // Enter key
            return choice;  // Return the selected choice
        }
    }
}


int username_exists(const char *username) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            return 1; // Username already exists
        }
    }
    return 0; // Username is available
}
void generate_random_password(char *password, int length) {
    const char *charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    int charset_size = strlen(charset);
    srand(time(NULL));  // Seed the random number generator

    for (int i = 0; i < length - 1; i++) {
        password[i] = charset[rand() % charset_size];
    }
    password[length - 1] = '\0';  // Null-terminate the password
}

int is_valid_password(const char *password) {
    if (strlen(password) < 7) return 0; // Password must be at least 7 characters
    int has_upper = 0, has_lower = 0, has_digit = 0;
    
    for (int i = 0; password[i] != '\0'; i++) {
        if (isupper(password[i])) has_upper = 1;
        if (islower(password[i])) has_lower = 1;
        if (isdigit(password[i])) has_digit = 1;
    }
    
    return has_upper && has_lower && has_digit;
}

int is_valid_email(const char *email) {
    // Simple check for xxx@yyy form
    char *at_sign = strchr(email, '@');
    return (at_sign != NULL && strchr(at_sign, '.') != NULL);
}
void save_user(User user) {
    FILE *file = fopen("users.txt", "a");
    if (file) {
        fprintf(file, "%s\n%s\n%s\n", user.username, user.password, user.email);
        fclose(file);
    }

    // ذخیره وضعیت بازی
    save_user_game_state(&user);
}

void register_new_user() {
    char username[MAX_USERNAME_LEN], password[MAX_PASSWORD_LEN], email[MAX_EMAIL_LEN];
    char random_password[MAX_PASSWORD_LEN];
    int use_random_password = 0;

    draw_frame("Register New User");

    echo();
    mvprintw(4, 10, "Enter username: ");
    timeout(-1);
    getnstr(username, MAX_USERNAME_LEN - 1);

    if (username_exists(username)) {
        attron(COLOR_PAIR(4));
        mvprintw(6, 10, "Username already exists. Press any key to continue.");
        attroff(COLOR_PAIR(4));
        getch();
        return;
    }

    mvprintw(6, 10, "Do you want a random password? (y/n): ");
    char ch = getch();
    if (ch == 'y' || ch == 'Y') {
        use_random_password = 1;
        generate_random_password(random_password, 10);
        mvprintw(8, 10, "Generated Password: %s", random_password);
        strncpy(password, random_password, MAX_PASSWORD_LEN);
    } else {
        mvprintw(8, 10, "Enter password: ");
        getnstr(password, MAX_PASSWORD_LEN - 1);
        if (!is_valid_password(password)) {
            attron(COLOR_PAIR(4));
            mvprintw(10, 10, "Password must be at least 7 characters with an uppercase, lowercase, and number.");
            attroff(COLOR_PAIR(4));
            getch();
            return;
        }
    }

    mvprintw(10, 10, "Enter email: ");
    getnstr(email, MAX_EMAIL_LEN - 1);
    if (!is_valid_email(email)) {
        attron(COLOR_PAIR(4));
        mvprintw(12, 10, "Invalid email format.");
        attroff(COLOR_PAIR(4));
        getch();
        return;
    }

    User new_user;
    strncpy(new_user.username, username, MAX_USERNAME_LEN);
    strncpy(new_user.password, password, MAX_PASSWORD_LEN);
    strncpy(new_user.email, email, MAX_EMAIL_LEN);
    save_user(new_user);

    attron(COLOR_PAIR(3));
    mvprintw(14, 10, "User registered successfully. Press any key to continue.");
    attroff(COLOR_PAIR(3));
    getch();
}
void login_user() {
    char username[MAX_USERNAME_LEN], password[MAX_PASSWORD_LEN];
    int login_successful = 0;

    draw_frame("Login User");

    echo();
    mvprintw(4, 10, "Enter username: ");
    getnstr(username, MAX_USERNAME_LEN - 1);
    mvprintw(6, 10, "Enter password: ");
    getnstr(password, MAX_PASSWORD_LEN - 1);

    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0 && strcmp(users[i].password, password) == 0) {
            login_successful = 1;
            current_user = users[i];  // کاربر شناسایی شد
            break;
        }
    }

    if (login_successful) {
        attron(COLOR_PAIR(3));
        mvprintw(8, 10, "Login successful! Welcome, %s.", username);
        load_user_game_state(&current_user);  // بارگذاری وضعیت بازی
        pre_game_menu(&current_user);  // رفتن به منوی پیش از بازی
    
        attroff(COLOR_PAIR(3));
    } else {
        attron(COLOR_PAIR(4));
        mvprintw(8, 10, "Login failed. Invalid username or password.");
        attroff(COLOR_PAIR(4));
    }

    mvprintw(10, 10, "Press any key to continue.");
    getch();
}

void forgot_password() {
    char username[MAX_USERNAME_LEN], email[MAX_EMAIL_LEN];
    int found = 0;

    echo();  // Enable input echo
    clear();  // Clear the screen
    box(stdscr, 0, 0);  // Draw a frame around the screen

    mvprintw(2, 10, "Forgot Password:");
    mvprintw(4, 10, "Enter your username: ");
    getnstr(username, MAX_USERNAME_LEN - 1);  // Get username from user

    mvprintw(5, 10, "Enter your email: ");
    getnstr(email, MAX_EMAIL_LEN - 1);  // Get email from user

    // Search for the user in the users array
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0 && strcmp(users[i].email, email) == 0) {
            mvprintw(7, 10, "Your password is: %s", users[i].password);  // Show the password
            found = 1;
            break;
        }
    }

    if (!found) {
        mvprintw(7, 10, "No matching user found. Please try again.");  // Error message if user not found
    }

    mvprintw(9, 10, "Press any key to continue.");
    getch();  // Wait for user input before returning to the previous menu
}

void user_login_menu() {
    int choice = 1;  // Default choice
    int ch;

    while (1) {
        clear();  // Clear the screen
        box(stdscr, 0, 0);  // Draw a frame around the screen

        // Display the menu options
        mvprintw(2, 10, "Login Menu:");
        mvprintw(4, 10, "1. Login with username and password");
        mvprintw(5, 10, "2. Forgot password");
        mvprintw(6, 10, "3. Back to main menu");

        // Highlight the selected option
        switch (choice) {
            case 1:
                mvprintw(4, 8, ">");  // Highlight first option
                break;
            case 2:
                mvprintw(5, 8, ">");  // Highlight second option
                break;
            case 3:
                mvprintw(6, 8, ">");  // Highlight third option
                break;
        }

        refresh();  // Update the screen to show changes

        // Get user input
        ch = getch();

        // Handle key presses
        if (ch == KEY_DOWN) {
            choice = (choice % 3) + 1;  // Cycle through 1 -> 2 -> 3 -> 1
        } else if (ch == KEY_UP) {
            choice = (choice == 1) ? 3 : choice - 1;  // Cycle through 3 -> 2 -> 1 -> 3
        } else if (ch == 10) {  // Enter key
            switch (choice) {
                case 1:
                    login_user();  // Call the login_user function
                    return;  // Return to main menu after login
                case 2:
                    forgot_password();  // Call the forgot_password function
                    break;  // Stay in the login menu after recovering password
                case 3:
                    return;  // Back to the main menu
            }
        }
    }
}
void pre_game_menu(User *user) {
    int choice = 1;
    int ch;

    while (1) {
        clear();
        draw_frame("Pre-Game Menu");

        const char *options[] = {
            "New Game",
            "Continue Previous Game",
            "View Leaderboard",
            "Settings",
            "Profile",
            "Back to Main Menu"
        };

        int num_options = sizeof(options) / sizeof(options[0]);

        // نمایش نام کاربر جاری
        attron(COLOR_PAIR(3) | A_BOLD);
        mvprintw(2, 10, "Welcome, %s!", user->username);
        attroff(COLOR_PAIR(3) | A_BOLD);

        // نمایش گزینه‌ها
        for (int i = 0; i < num_options; i++) {
            highlight_option(4 + i * 2, options[i], (i + 1 == choice));
        }

        refresh();
        ch = getch();

        // پیمایش در منو با کلیدهای جهت‌دار
        if (ch == KEY_DOWN) {
            choice = (choice % num_options) + 1;
        } else if (ch == KEY_UP) {
            choice = (choice == 1) ? num_options : choice - 1;
        } else if (ch == 10) {  // Enter key
            switch (choice) {
                case 1:
                    start_new_game(user);  // شروع بازی جدید
                    break;
                case 2:
                    continue_previous_game(user);  // ادامه بازی قبلی
                    break;
                case 3:
                    show_leaderboard(user->username);  // نمایش لیدربورد
                    break;
                case 4:
                    settings_menu(user);  // تنظیمات
                    break;
                case 5:
                    show_profile(user);  // نمایش پروفایل
                    break;
                case 6:
                    return;  // بازگشت به منوی اصلی
            }
        }
    }
}
void start_new_game(User *user) {
    clear();
    draw_frame("New Game");

    // تولید نقشه جدید
    generate_map(&(user->game_map));

    // مقداردهی اولیه موقعیت بازیکن
    user->game_map.player_x = 5;
    user->game_map.player_y = 5;

    // به‌روزرسانی دید و رسم نقشه
    update_visibility(&(user->game_map), user->game_map.player_x, user->game_map.player_y);
    draw_map(user);

    // حلقه اصلی بازی
    while (1) {
        int ch = getch();
        switch (ch) {
            case KEY_UP: move_player(user, -1, 0); break;
            case KEY_DOWN: move_player(user, 1, 0); break;
            case KEY_LEFT: move_player(user, 0, -1); break;
            case KEY_RIGHT: move_player(user, 0, 1); break;
            case 'q': return;
        }

        update_visibility(&(user->game_map), user->game_map.player_x, user->game_map.player_y);
        draw_map(user);
    }
}


void continue_previous_game(User *user) {
    char saved_games[10][50];  // لیست بازی‌های ذخیره‌شده
    int saved_game_count = 0;
    int choice = 1;
    int ch;

    // خواندن فایل بازی‌های ذخیره‌شده
    FILE *file = fopen("saved_games.txt", "r");
    if (file) {
        while (fgets(saved_games[saved_game_count], 50, file)) {
            saved_games[saved_game_count][strcspn(saved_games[saved_game_count], "\n")] = '\0';  // حذف کاراکتر newline
            saved_game_count++;
        }
        fclose(file);
    }

    if (saved_game_count == 0) {
        clear();
        draw_frame("Continue Previous Game");

        attron(COLOR_PAIR(4));  // رنگ قرمز برای پیام خطا
        mvprintw(6, (COLS - strlen("No saved games found.")) / 2, "No saved games found.");
        attroff(COLOR_PAIR(4));

        attron(COLOR_PAIR(2));  // رنگ زرد برای بازگشت
        mvprintw(8, (COLS - strlen("Press any key to return to Pre-Game Menu")) / 2, "Press any key to return to Pre-Game Menu");
        attroff(COLOR_PAIR(2));

        refresh();
        getch();
        return;
    }

    while (1) {
        clear();
        draw_frame("Continue Previous Game");

        for (int i = 0; i < saved_game_count; i++) {
            highlight_option(4 + i * 2, saved_games[i], (i + 1 == choice));
        }

        highlight_option(6 + saved_game_count * 2, "Back to Pre-Game Menu", (choice == saved_game_count + 1));

        refresh();
        ch = getch();

        if (ch == KEY_DOWN) {
            choice = (choice % (saved_game_count + 1)) + 1;
        } else if (ch == KEY_UP) {
            choice = (choice == 1) ? (saved_game_count + 1) : choice - 1;
        } else if (ch == 10) {  // Enter key
            if (choice == saved_game_count + 1) {
                return;  // بازگشت به منوی پیش از بازی
            } else {
                clear();
                draw_frame("Loading Game");

                attron(COLOR_PAIR(3));  // رنگ سبز برای موفقیت
                mvprintw(6, (COLS - strlen("Loading game...")) / 2, "Loading game...");
                attroff(COLOR_PAIR(3));

                // بارگذاری وضعیت بازی
                load_settings(user);

                refresh();
                getch();
                break;  // ادامه بازی انتخاب‌شده
            }
        }
    }
}

void show_leaderboard(const char *current_username) {
    const int users_per_page = 10;  // تعداد کاربران در هر صفحه
    int total_users = user_count;  // تعداد کل کاربران
    int page = 0;  // شماره صفحه جاری
    int ch;

    if (total_users == 0) {
        clear();
        draw_frame("Leaderboard");

        attron(COLOR_PAIR(4));  // رنگ قرمز برای پیام خطا
        mvprintw(6, (COLS - strlen("No users found.")) / 2, "No users found.");
        attroff(COLOR_PAIR(4));

        mvprintw(8, (COLS - strlen("Press any key to return to the menu.")) / 2, "Press any key to return to the menu.");
        refresh();
        getch();
        return;
    }

    while (1) {
        clear();
        draw_frame("Leaderboard");

        int start = page * users_per_page;
        int end = (start + users_per_page < total_users) ? start + users_per_page : total_users;

        for (int i = start; i < end; i++) {
            int rank = i + 1;
            int y = 4 + (i - start);

            if (rank == 1) {
                attron(COLOR_PAIR(3) | A_BOLD);
                mvprintw(y, 4, "🏆 ");
            } else if (rank == 2) {
                attron(COLOR_PAIR(2) | A_BOLD);
                mvprintw(y, 4, "🥈 ");
            } else if (rank == 3) {
                attron(COLOR_PAIR(1) | A_BOLD);
                mvprintw(y, 4, "🥉 ");
            } else if (strcmp(users[i].username, current_username) == 0) {
                attron(COLOR_PAIR(5) | A_BOLD);
                mvprintw(y, 4, "➡️ ");
            }

            mvprintw(y, 8, "%2d. %-10s | Score: %4d | Gold: %3d | Games: %2d",
                     rank, users[i].username, users[i].score, users[i].gold, users[i].games);

            attroff(COLOR_PAIR(3) | A_BOLD | COLOR_PAIR(2) | COLOR_PAIR(1) | COLOR_PAIR(5));
        }

        mvprintw(LINES - 3, 4, "Page %d of %d", page + 1, (total_users + users_per_page - 1) / users_per_page);
        mvprintw(LINES - 2, 4, "Use LEFT/RIGHT to navigate pages, or press 'q' to return.");

        refresh();
        ch = getch();

        if (ch == 'q') {
            break;
        } else if (ch == KEY_RIGHT && end < total_users) {
            page++;
        } else if (ch == KEY_LEFT && page > 0) {
            page--;
        }
    }
}
void guest_login() {
    strncpy(current_user.username, "Guest", MAX_USERNAME_LEN);
    clear();
    draw_frame("Guest Login");

    attron(COLOR_PAIR(3));
    mvprintw(6, (COLS - strlen("Welcome, Guest! Enjoy the game.")) / 2, "Welcome, Guest! Enjoy the game.");
    attroff(COLOR_PAIR(3));

    mvprintw(8, (COLS - strlen("Press any key to go to the pre-game menu.")) / 2, "Press any key to go to the pre-game menu.");
    refresh();
    getch();

    // نمایش منوی پیش از بازی
    pre_game_menu(&current_user);
}

void change_difficulty(User *user) {
    const char *difficulty_levels[] = {"Easy", "Medium", "Hard"};
    int choice = user->difficulty;
    int ch;

    draw_frame("Change Difficulty");

    while (1) {
        for (int i = 0; i < 3; i++) {
            highlight_option(4 + i * 2, difficulty_levels[i], (i == choice));
        }

        mvprintw(LINES - 3, 4, "Use UP/DOWN to select, ENTER to confirm.");
        refresh();

        ch = getch();

        if (ch == KEY_DOWN) {
            choice = (choice + 1) % 3;
        } else if (ch == KEY_UP) {
            choice = (choice == 0) ? 2 : choice - 1;
        } else if (ch == 10) {  // Enter key
            user->difficulty = choice;
            save_settings(user);
            mvprintw(LINES - 5, 4, "Difficulty set to: %s", difficulty_levels[choice]);
            refresh();
            getch();
            return;
        }
    }
}

void change_music(User *user) {
    int choice = user->music;
    int ch;

    draw_frame("Change Music");

    while (1) {
        for (int i = 0; i < 4; i++) {
            highlight_option(4 + i * 2, music_options[i], (i == choice));
        }

        mvprintw(LINES - 3, 4, "Use UP/DOWN to select, ENTER to confirm.");
        refresh();

        ch = getch();

        if (ch == KEY_DOWN) {
            choice = (choice + 1) % 4;
        } else if (ch == KEY_UP) {
            choice = (choice == 0) ? 3 : choice - 1;
        } else if (ch == 10) {  // Enter key
            user->music = choice;
            save_settings(user);
            mvprintw(LINES - 5, 4, "Music set to: %s", music_options[choice]);
            refresh();
            getch();
            return;
        }
    }
}

void settings_menu(User *user) {
    const char *options[] = {
        "Change Difficulty",
        "Change Music",
        "Back to Pre-Game Menu"
    };

    int choice = 0;
    int num_options = sizeof(options) / sizeof(options[0]);
    int ch;

    while (1) {
        clear();
        draw_frame("Settings");

        for (int i = 0; i < num_options; i++) {
            highlight_option(4 + i * 2, options[i], (i == choice));
        }

        mvprintw(LINES - 3, 4, "Use UP/DOWN to select, ENTER to confirm.");
        refresh();

        ch = getch();

        if (ch == KEY_DOWN) {
            choice = (choice + 1) % num_options;
        } else if (ch == KEY_UP) {
            choice = (choice == 0) ? (num_options - 1) : choice - 1;
        } else if (ch == 10) {  // Enter key
            switch (choice) {
                case 0:
                    change_difficulty(user);
                    break;
                case 1:
                    change_music(user);
                    break;
                case 2:
                    return;
            }
        }
    }
}

void show_profile(User *user) {
    clear();
    draw_frame("Profile");

    mvprintw(4, 10, "Username: %s", user->username);
    mvprintw(5, 10, "Email: %s", user->email);
    mvprintw(6, 10, "Score: %d", user->score);
    mvprintw(7, 10, "Gold: %d", user->gold);
    mvprintw(8, 10, "Games Played: %d", user->games);
    mvprintw(9, 10, "Difficulty: %s", 
             user->difficulty == 0 ? "Easy" : 
             user->difficulty == 1 ? "Medium" : "Hard");

    mvprintw(11, 10, "Press any key to return.");
    refresh();
    getch();
}

void draw_message(const char *message) {
    int len = strlen(message);
    int x = (COLS - len) / 2;
    int y = LINES - 2;

    attron(COLOR_PAIR(3) | A_BOLD);
    mvprintw(y, x, "%s", message);
    attroff(COLOR_PAIR(3) | A_BOLD);
    refresh();

}
void draw_map(User *user) {
    GameMap *game_map = &(user->game_map);
    clear();

    for (int i = 0; i < MAX_MAP_SIZE; i++) {
        for (int j = 0; j < MAX_MAP_SIZE; j++) {
            if (game_map->map[i][j].visible) {
                switch (game_map->map[i][j].type) {
                    case 'W': mvprintw(i, j, "|"); break;  // دیوار
                    case '.': mvprintw(i, j, "."); break;  // کف
                    case '+': mvprintw(i, j, "+"); break;  // در
                    case '#': mvprintw(i, j, "#"); break;  // راهرو
                    default: mvprintw(i, j, " "); break;   // فضای خالی
                }
            }
        }
    }

    mvprintw(game_map->player_x, game_map->player_y, "@");
    refresh();
}


void save_user_game_state(User *user) {
    char filename[100];
    snprintf(filename, sizeof(filename), "%s_game_state.txt", user->username);

    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Unable to open game state file");
        return;
    }

    // ذخیره اطلاعات کاربر
    fprintf(file, "%d %d\n", user->game_map.player_x, user->game_map.player_y);
    for (int i = 0; i < MAX_MAP_SIZE; i++) {
        for (int j = 0; j < MAX_MAP_SIZE; j++) {
            fputc(user->game_map.map[i][j].type, file);
        }
        fputc('\n', file);
    }

    fclose(file);
}
void load_user_game_state(User *user) {
    char filename[100];
    snprintf(filename, sizeof(filename), "%s_game_state.txt", user->username);

    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Unable to open game state file");
        return;
    }

    // بارگذاری اطلاعات کاربر
    fscanf(file, "%d %d\n", &user->game_map.player_x, &user->game_map.player_y);
    for (int i = 0; i < MAX_MAP_SIZE; i++) {
        for (int j = 0; j < MAX_MAP_SIZE; j++) {
            user->game_map.map[i][j].type = fgetc(file);
        }
        fgetc(file);  // خواندن کاراکتر جدید خط
    }

    fclose(file);
}
void load_game(User *user) {
    char filename[100];
    snprintf(filename, sizeof(filename), "%s_game.txt", user->username);

    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to load the game");
        return;
    }

    // بارگذاری نقشه بازی
    for (int i = 0; i < MAX_MAP_SIZE; i++) {
        for (int j = 0; j < MAX_MAP_SIZE; j++) {
            fscanf(file, " %c", &user->game_map.map[i][j].type);
        }
    }

    // بارگذاری موقعیت بازیکن
    fscanf(file, "%d %d", &user->game_map.player_x, &user->game_map.player_y);

    fclose(file);

    printf("Game successfully loaded for user: %s\n", user->username);
}
void save_game(User *user) {
    char filename[100];
    snprintf(filename, sizeof(filename), "%s_game.txt", user->username);

    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Failed to save the game");
        return;
    }

    // ذخیره نقشه بازی
    for (int i = 0; i < MAX_MAP_SIZE; i++) {
        for (int j = 0; j < MAX_MAP_SIZE; j++) {
            fprintf(file, "%c ", user->game_map.map[i][j].type);
        }
        fprintf(file, "\n");
    }

    // ذخیره موقعیت بازیکن
    fprintf(file, "%d %d\n", user->game_map.player_x, user->game_map.player_y);

    fclose(file);

    printf("Game successfully saved for user: %s\n", user->username);
}
void move_player(User *user, int dx, int dy) {
    int new_x = user->game_map.player_x + dx;
    int new_y = user->game_map.player_y + dy;

    // بررسی اینکه بازیکن از محدوده نقشه خارج نشود
    if (new_x < 0 || new_x >= MAX_MAP_SIZE || new_y < 0 || new_y >= MAX_MAP_SIZE) {
        attron(COLOR_PAIR(4));  // استفاده از رنگ قرمز برای پیام
        mvprintw(LINES - 2, 0, "Move out of bounds!");  // نمایش پیام در پایین صفحه
        attroff(COLOR_PAIR(4));
        refresh();
        return;
    }

    // بررسی برخورد با دیوار
    if (user->game_map.map[new_x][new_y].type == 'W') {
        attron(COLOR_PAIR(4));  // استفاده از رنگ قرمز برای پیام
        mvprintw(LINES - 2, 0, "Cannot move into a wall!");  // نمایش پیام در پایین صفحه
        attroff(COLOR_PAIR(4));
        refresh();
        return;
    }

    // به‌روزرسانی موقعیت بازیکن
    user->game_map.player_x = new_x;
    user->game_map.player_y = new_y;

    // به‌روزرسانی دید و نقشه
    update_visibility(&(user->game_map), new_x, new_y);
    draw_map(user);

    // نمایش پیام موفقیت حرکت
    attron(COLOR_PAIR(3));  // استفاده از رنگ سبز برای پیام
    mvprintw(LINES - 2, 0, "Player moved to position (%d, %d)      ", new_x, new_y);  // پیام حرکت بازیکن
    attroff(COLOR_PAIR(3));
    refresh();
}


int main() {
    setup_game();
    load_users();  // Load users from the file at the beginning

    int choice;

    while (1) {
        choice = handle_menu();  // Main menu for the user to choose an action

        switch (choice) {
            case 1:
                register_new_user();  // Register new user
                break;

            case 2:
                // Login as existing user
                login_user();  // Directly handle user login and proceed to pre-game menu
                break;

            case 3:
                guest_login();  // Guest login (no username required)
                break;

            case 4:
                clear();
                mvprintw(10, 10, "Thank you for playing! Exiting...");
                refresh();
                sleep(2); 
                cleanup_game();  // Exit the program
                exit(0);
        }
    }
}

