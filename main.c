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

typedef struct {
    char username[MAX_USERNAME_LEN];
    char password[MAX_PASSWORD_LEN];
    char email[MAX_EMAIL_LEN];
} User;
typedef struct {
    char username[50];  // نام کاربری
    int difficulty;     // سطح دشواری
    int music;          // وضعیت موزیک (فعال/غیرفعال)
    int score;          // امتیاز
    int gold;           // طلا
    int theme;          // رنگ تم
} UserSettings;
void save_settings(UserSettings *user) {
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
char current_user[MAX_USERNAME_LEN];  // نام کاربری کاربر فعلی
void pre_game_menu(const char *username);
void start_new_game();
void continue_previous_game();
void show_leaderboard(const char *current_user);
void settings_menu();
void show_profile();
void settings_menu();
void show_profile();
void change_difficulty();
int user_count = 0;
UserSettings load_settings(const char *username) {
    UserSettings user = {"", 1, 1, 0, 0, 0};  // مقادیر پیش‌فرض

    // ایجاد نام فایل با استفاده از نام کاربری
    char filename[100];
    snprintf(filename, sizeof(filename), "%s_settings.txt", username);

    FILE *file = fopen(filename, "r");  // باز کردن فایل برای خواندن
    if (file == NULL) {
        perror("Unable to open settings file");
        return user;  // اگر فایل وجود ندارد، تنظیمات پیش‌فرض باز می‌گردد
    }

    // بارگذاری تنظیمات از فایل
    if (fscanf(file, "%s %d %d %d %d %d", 
               user.username, 
               &user.difficulty, 
               &user.music, 
               &user.score, 
               &user.gold, 
               &user.theme) != EOF) {
        fclose(file);
        return user;  // بازگرداندن تنظیمات بارگذاری شده
    }

    fclose(file);
    return user;  // اگر تنظیمات یافت نشد، تنظیمات پیش‌فرض باز می‌گردد
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
            break;
        }
    }

    if (login_successful) {
        attron(COLOR_PAIR(3));
        mvprintw(8, 10, "Login successful! Welcome, %s.", username);
        pre_game_menu(username);  // رفتن به منوی پیش از بازی
    
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

void pre_game_menu(const char *username) {
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
        mvprintw(2, 10, "Welcome, %s!", username);
        attroff(COLOR_PAIR(3) | A_BOLD);

        for (int i = 0; i < num_options; i++) {
            highlight_option(4 + i * 2, options[i], (i + 1 == choice));
        }

        refresh();
        ch = getch();

        if (ch == KEY_DOWN) {
            choice = (choice % num_options) + 1;
        } else if (ch == KEY_UP) {
            choice = (choice == 1) ? num_options : choice - 1;
        } else if (ch == 10) {
            switch (choice) {
                case 1:
                    start_new_game();
                    break;
                case 2:
                    continue_previous_game();
                    break;
                case 3:
                    show_leaderboard(username);
                    break;
                case 4:
                    settings_menu();
                    break;
                case 5:
                    show_profile();
                    break;
                case 6:
                    return;
            }
        }
    }
}

void start_new_game() {
    clear();
    draw_frame("New Game");

    attron(COLOR_PAIR(3));  // رنگ سبز برای پیام موفقیت
    mvprintw(6, (COLS - strlen("Starting a new game...")) / 2, "Starting a new game...");
    attroff(COLOR_PAIR(3));

    attron(COLOR_PAIR(2));  // رنگ زرد برای بازگشت
    mvprintw(8, (COLS - strlen("Press any key to return to Pre-Game Menu")) / 2, "Press any key to return to Pre-Game Menu");
    attroff(COLOR_PAIR(2));

    refresh();
    getch();  // صبر برای ورودی کاربر
}
void continue_previous_game() {
    char saved_games[10][50];  // بازی‌های ذخیره‌شده
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

        attron(COLOR_PAIR(4));  // رنگ قرمز برای خطا
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

        highlight_option(6 + saved_game_count * 2, "Back to Pre-Game Menu", (choice == saved_game_count + 1));  // گزینه بازگشت

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

                refresh();
                getch();
                break;  // ادامه بازی انتخاب‌شده
            }
        }
    }
}
void show_leaderboard(const char *current_user) {
    const int users_per_page = 10;  // تعداد کاربران در هر صفحه
    int total_users = user_count;  // تعداد کل کاربران از user_count گرفته می‌شود
    int page = 0;  // شماره صفحه جاری
    int ch;

    // بررسی تعداد کاربران (در صورت نبود کاربر)
    if (total_users == 0) {
        clear();
        draw_frame("Leaderboard");
        attron(COLOR_PAIR(4));  // رنگ قرمز برای خطا
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

        // محاسبه کاربران در صفحه جاری
        int start = page * users_per_page;
        int end = (start + users_per_page < total_users) ? start + users_per_page : total_users;
        setlocale(LC_CTYPE, "");
        for (int i = start; i < end; i++) {
            int rank = i + 1;  // رتبه کاربر
            int y = 4 + (i - start);  // موقعیت Y برای هر کاربر

            // انتخاب رنگ و افکت برای کاربران خاص
            if (rank == 1) {
                attron(COLOR_PAIR(3) | A_BOLD);  // طلایی برای رتبه اول
                mvprintw(y, 4, "🏆 Legend");
            } else if (rank == 2) {
                attron(COLOR_PAIR(2) | A_BOLD);  // نقره‌ای برای رتبه دوم
                mvprintw(y, 4, "🥈 Pro");
            } else if (rank == 3) {
                attron(COLOR_PAIR(1) | A_BOLD);  // برنزی برای رتبه سوم
                mvprintw(y, 4, "🥉 Goat");
            } else if (strcmp(users[i].username, current_user) == 0) {
                attron(A_BOLD | COLOR_PAIR(5));  // Bold برای کاربر جاری
                mvprintw(y, 4, "➡️  ");
            }

            // نمایش اطلاعات کاربر
            mvprintw(y, 8, "%2d. %-10s | Score: %4d | Gold: %3d | Games: %2d | Experience: %ld days",
                     rank, users[i].username, rand() % 1000, rand() % 500,
                     rand() % 20, (time(NULL) - (rand() % 31536000)) / 86400);

            // خاموش کردن افکت
            attroff(COLOR_PAIR(3) | A_BOLD | COLOR_PAIR(2) | COLOR_PAIR(1) | COLOR_PAIR(5));
        }

        // نمایش کنترل‌های صفحه‌بندی
        mvprintw(LINES - 3, 4, "Page %d of %d", page + 1, (total_users + users_per_page - 1) / users_per_page);
        mvprintw(LINES - 2, 4, "Use LEFT/RIGHT to navigate pages, or press 'q' to return.");

        refresh();

        // ورودی کاربر برای صفحه‌بندی
        ch = getch();
        if (ch == 'q') {
            break;  // بازگشت به منوی قبلی
        } else if (ch == KEY_RIGHT && end < total_users) {
            page++;  // رفتن به صفحه بعد
        } else if (ch == KEY_LEFT && page > 0) {
            page--;  // برگشت به صفحه قبل
        }
    }
}
void guest_login() {
    strncpy(current_user, "Guest", MAX_USERNAME_LEN); // تنظیم نام کاربری مهمان
    clear();
    draw_frame("Guest Login");

    attron(COLOR_PAIR(3));
    mvprintw(6, (COLS - strlen("Welcome, Guest! Enjoy the game.")) / 2, "Welcome, Guest! Enjoy the game.");
    attroff(COLOR_PAIR(3));

    mvprintw(8, (COLS - strlen("Press any key to go to the pre_game menu.")) / 2, "Press any key to go to the pre_game menu.");
    refresh();
    getch();  // انتظار برای ورودی کاربر

    // بعد از دریافت ورودی، منوی پیش از بازی را نمایش بدهید
    pre_game_menu(current_user);  // فراخوانی منوی پیش از بازی
}
void change_difficulty() {
    const char *difficulty[] = {"Easy", "Medium", "Hard"};
    int choice = 1;
    int ch;
    int n_choices = 3;
    
    // رسم قاب صفحه تنظیمات
    draw_frame("Change Difficulty");

    while (1) {
        // نمایش گزینه‌های انتخاب سختی
        for (int i = 0; i < n_choices; i++) {
            highlight_option(4 + i * 2, difficulty[i], (i + 1 == choice));  // هر گزینه در وسط صفحه قرار می‌گیرد
        }

        // نمایش دستورالعمل برای کاربر
        mvprintw(LINES - 3, 4, "Use UP/DOWN to select, ENTER to confirm.");
        refresh();

        // دریافت ورودی کاربر
        ch = getch();

        if (ch == KEY_DOWN) {
            choice = (choice % n_choices) + 1;
        } else if (ch == KEY_UP) {
            choice = (choice == 1) ? n_choices : choice - 1;
        } else if (ch == 10) {  // کلید ENTER
            // نمایش پیام انتخاب شده
            mvprintw(LINES - 5, 4, "Difficulty set to: %s", difficulty[choice - 1]);
            refresh();
            getch();  // منتظر ورودی کاربر برای برگشت
            return;   // بازگشت به منوی قبلی
        }
    }
}

void change_music() {
    int choice = 1;
    int ch;

    while (1) {
        clear();
        draw_frame("Change Music");

        // نمایش گزینه‌ها
        for (int i = 0; i < sizeof(music_options) / sizeof(music_options[0]); i++) {
            highlight_option(4 + i * 2, music_options[i], (i + 1 == choice));
        }

        mvprintw(LINES - 2, 4, "Press 'Enter' to select, 'q' to exit.");
        refresh();

        ch = getch();

        if (ch == KEY_DOWN) {
            choice = (choice % 4) + 1;
        } else if (ch == KEY_UP) {
            choice = (choice == 1) ? 4 : choice - 1;
        } else if (ch == 10) {  // Enter key
            current_music = choice - 1;
            mvprintw(6, 10, "Selected Music: %s", music_options[current_music]);
            refresh();
            getch();  // انتظار برای ورودی کاربر
            return;
        } else if (ch == 'q') {
            return;  // بازگشت به منوی قبلی
        }
    }
}
void settings_menu() {
    int choice = 1;
    int ch;

    while (1) {
        clear();
        draw_frame("Settings");

        // آرایه‌ای از گزینه‌ها
        const char *options[] = {
            "Change Difficulty",     // گزینه برای تغییر سطح دشواری
            "Change Music",          // گزینه برای تغییر موزیک
            "Back to Main Menu"      // گزینه برای برگشت به منوی اصلی
        };

        int num_options = sizeof(options) / sizeof(options[0]);

        // نمایش گزینه‌ها
        for (int i = 0; i < num_options; i++) {
            highlight_option(4 + i * 2, options[i], (i + 1 == choice));
        }

        refresh();
        ch = getch();

        if (ch == KEY_DOWN) {
            choice = (choice % num_options) + 1;
        } else if (ch == KEY_UP) {
            choice = (choice == 1) ? num_options : choice - 1;
        } else if (ch == 10) {  // Enter key
            switch (choice) {
                case 1:
                    change_difficulty();  // تابع تغییر دشواری
                    break;
                case 2:
                    change_music();  // تابع تغییر موزیک
                    break;
                case 3:
                    return;  // برگشت به منوی اصلی
            }
        } else if (ch == 'q') {  // کلید 'q' برای خروج
            return;
        }
    }
}


void show_profile() {
    clear();
    draw_frame("Profile");

    mvprintw(4, 10, "Profile feature is not implemented yet.");
    mvprintw(6, 10, "Press any key to return.");
    refresh();
    getch();
}
void draw_message(const char *message) {
    int message_len = strlen(message);
    int start_x = (COLS - message_len) / 2;  // وسط صفحه افقی
    int start_y = LINES - 2;  // پایین صفحه (قبل از خط پایانی)

    attron(COLOR_PAIR(4) | A_BOLD);  // رنگ و افکت برای پیام
    mvprintw(start_y, start_x, "%s", message);  // نمایش پیام در وسط پایین صفحه
    attroff(COLOR_PAIR(4) | A_BOLD);
    refresh();
}

// این تابع برای نمایش پیام‌ها در مواقع مختلف بازی فراخوانی خواهد شد.
// برای مثال، زمانی که دشمن کشته می‌شود یا سلاح جدیدی برداشته می‌شود.

void display_game_message(const char *message) {
    draw_message(message);  // فراخوانی تابع برای نمایش پیام
}

int main() {
    setup_game();
    load_users();  // Load users from the file at the beginning

    int choice;
    while (1) {
        choice = handle_menu();

        switch (choice) {
            case 1:
                register_new_user();  // Register new user
                break;
            case 2:
                user_login_menu();  // Open the user login menu
                break;
            case 3:
                guest_login();  // Guest login
                break;
            case 4:
                cleanup_game();  // Exit the program
                exit(0);
        }
    }

    cleanup_game();
    return 0;
}


