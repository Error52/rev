#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <time.h>

#define APP_NAME "Ы-Банк: Reverse Challenge"
#define APP_VERSION "1.0"
#define MAX_LOG 256
#define MAX_PHONE 32
#define MAX_INPUT 128

/*
 * Данный файл — учебный reverse task.
 * В нем есть стандартная логика банковского меню,
 * а также скрытое условие для получения флага.
 *
 * Флаг хранится не в plaintext: он закодирован.
 * Для получения флага нужно:
 * 1) накопить баланс >= 1_000_000_000
 * 2) выполнить перевод на номер 8925553525
 * 3) сумма перевода должна быть >= 1_000_000_000
 */

typedef struct AppState {
    long long balance;
    int operations;
    int running;
    int suspicious_counter;
    char last_transfer_phone[MAX_PHONE];
    long long last_transfer_amount;
} AppState;

typedef struct TransferLog {
    char phone[MAX_PHONE];
    long long amount;
    char stamp[64];
} TransferLog;

static TransferLog g_logs[MAX_LOG];
static int g_log_count = 0;

static void line(void) {
    printf("------------------------------------------------------------\n");
}

static void clear_stdin_rest(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {
    }
}

static int read_line(char *buf, size_t sz) {
    if (!fgets(buf, (int)sz, stdin)) {
        return 0;
    }
    size_t n = strlen(buf);
    if (n > 0 && buf[n - 1] == '\n') {
        buf[n - 1] = '\0';
    } else {
        clear_stdin_rest();
    }
    return 1;
}

static int is_digits_only(const char *s) {
    if (!s || !*s) {
        return 0;
    }
    while (*s) {
        if (!isdigit((unsigned char)*s)) {
            return 0;
        }
        s++;
    }
    return 1;
}

static long long parse_amount_or_negative(const char *s) {
    if (!s || !*s) {
        return -1;
    }
    char *end = NULL;
    long long v = strtoll(s, &end, 10);
    if (end == s || *end != '\0') {
        return -1;
    }
    return v;
}

static void print_banner(void) {
    line();
    printf("%s\n", APP_NAME);
    printf("Version: %s\n", APP_VERSION);
    printf("Учебное приложение для reverse-инженеринга\n");
    line();
    printf("Доступные действия:\n");
    printf(" 1) Перевод\n");
    printf(" 2) Пополнить\n");
    printf(" 3) Сколько денег на счету\n");
    printf(" 4) История переводов\n");
    printf(" 5) Выход\n");
    line();
}

static void print_legal_disclaimer(void) {
    printf("Важно: это CTF/учебный макет.\n");
    printf("Реальные несанкционированные операции с деньгами незаконны.\n");
    printf("Используйте только в рамках локального задания.\n");
    line();
}

static void get_timestamp(char *out, size_t out_sz) {
    time_t now = time(NULL);
    struct tm *tmv = localtime(&now);
    if (!tmv) {
        snprintf(out, out_sz, "1970-01-01 00:00:00");
        return;
    }
    strftime(out, out_sz, "%Y-%m-%d %H:%M:%S", tmv);
}

static void add_transfer_log(const char *phone, long long amount) {
    if (g_log_count >= MAX_LOG) {
        return;
    }
    snprintf(g_logs[g_log_count].phone, MAX_PHONE, "%s", phone);
    g_logs[g_log_count].amount = amount;
    get_timestamp(g_logs[g_log_count].stamp, sizeof(g_logs[g_log_count].stamp));
    g_log_count++;
}

static void print_transfer_logs(void) {
    line();
    printf("История переводов:\n");
    if (g_log_count == 0) {
        printf("  (пусто)\n");
        line();
        return;
    }
    for (int i = 0; i < g_log_count; i++) {
        printf(" %03d | %s | %s | %lld RUB\n",
               i + 1,
               g_logs[i].stamp,
               g_logs[i].phone,
               g_logs[i].amount);
    }
    line();
}

static void print_balance(const AppState *st) {
    line();
    printf("Текущий баланс: %lld RUB\n", st->balance);
    printf("Операций выполнено: %d\n", st->operations);
    line();
}

static void top_up(AppState *st) {
    char buf[MAX_INPUT];
    line();
    printf("Введите сумму пополнения: ");
    if (!read_line(buf, sizeof(buf))) {
        printf("Ошибка ввода\n");
        line();
        return;
    }
    long long amount = parse_amount_or_negative(buf);
    if (amount <= 0) {
        printf("Некорректная сумма.\n");
        line();
        return;
    }
    if (amount > 5000000000LL) {
        printf("Сумма слишком большая для одного пополнения.\n");
        line();
        return;
    }
    st->balance += amount;
    st->operations++;
    if (amount > 100000000LL) {
        st->suspicious_counter++;
    }
    printf("Успешно. Баланс увеличен на %lld RUB\n", amount);
    print_balance(st);
}

static int check_phone(const char *phone) {
    if (!is_digits_only(phone)) {
        return 0;
    }
    size_t n = strlen(phone);
    if (n < 10 || n > 15) {
        return 0;
    }
    return 1;
}

static uint8_t rotr8(uint8_t v, int s) {
    return (uint8_t)((v >> s) | (v << (8 - s)));
}

static void decode_flag(char *out, size_t out_sz) {
    /*
     * Это закодированная форма строки:
     * flag{st34l1ng_m0n3y_1s_f0rb1dd3n_by_th3_l4w}
     *
     * Шаги кодирования:
     * 1) XOR с циклическим ключом
     * 2) ROTL(3)
     */
    static const uint8_t encoded[] = {
        0x48, 0xea, 0x91, 0xe9, 0x13, 0x00, 0xf3, 0xe2,
        0x28, 0xf9, 0x5b, 0xbb, 0xa0, 0xaa, 0x10, 0x08,
        0xe9, 0x4b, 0x03, 0x61, 0xd9, 0xe0, 0x73, 0xa9,
        0x53, 0x5b, 0x88, 0xd9, 0x58, 0xaa, 0x03, 0xa1,
        0x32, 0x88, 0x9b, 0x81, 0x2a, 0xd9, 0x4b, 0x32,
        0xf8, 0xf1, 0xc0, 0x62
    };
    static const uint8_t key[] = {
        0x6f, 0x31, 0x53, 0x5a, 0x19, 0x73, 0x0a
    };

    size_t n = sizeof(encoded) / sizeof(encoded[0]);
    if (out_sz < n + 1) {
        if (out_sz > 0) {
            out[0] = '\0';
        }
        return;
    }

    for (size_t i = 0; i < n; i++) {
        uint8_t x = encoded[i];
        uint8_t y = rotr8(x, 3);
        uint8_t z = (uint8_t)(y ^ key[i % (sizeof(key) / sizeof(key[0]))]);
        out[i] = (char)z;
    }
    out[n] = '\0';
}

static int maybe_get_flag(const AppState *st, const char *phone, long long amount) {
    const long long target = 1000000000LL;
    const char *target_phone = "8925553525";

    if (st->balance >= target && amount >= target && strcmp(phone, target_phone) == 0) {
        return 1;
    }
    return 0;
}

static void transfer_money(AppState *st) {
    char phone[MAX_INPUT];
    char buf[MAX_INPUT];

    line();
    printf("Введите номер счета/телефона получателя: ");
    if (!read_line(phone, sizeof(phone))) {
        printf("Ошибка ввода\n");
        line();
        return;
    }

    if (!check_phone(phone)) {
        printf("Неверный формат номера. Используйте только цифры.\n");
        line();
        return;
    }

    printf("Введите сумму перевода: ");
    if (!read_line(buf, sizeof(buf))) {
        printf("Ошибка ввода\n");
        line();
        return;
    }

    long long amount = parse_amount_or_negative(buf);
    if (amount <= 0) {
        printf("Сумма должна быть положительной.\n");
        line();
        return;
    }

    if (amount > st->balance) {
        printf("Недостаточно средств.\n");
        line();
        return;
    }

    int unlocked = maybe_get_flag(st, phone, amount);

    st->balance -= amount;
    st->operations++;
    strncpy(st->last_transfer_phone, phone, sizeof(st->last_transfer_phone) - 1);
    st->last_transfer_phone[sizeof(st->last_transfer_phone) - 1] = '\0';
    st->last_transfer_amount = amount;
    add_transfer_log(phone, amount);

    printf("Перевод выполнен: %lld RUB -> %s\n", amount, phone);

    if (unlocked) {
        char flag[128];
        decode_flag(flag, sizeof(flag));
        line();
        printf("[challenge] secret unlocked:\n");
        printf("%s\n", flag);
        line();
    }

    print_balance(st);
}

static int read_menu_choice(void) {
    char buf[MAX_INPUT];
    printf("Выберите действие (1-5): ");
    if (!read_line(buf, sizeof(buf))) {
        return -1;
    }
    return (int)parse_amount_or_negative(buf);
}

static void print_help_block(void) {
    printf("Подсказки для reverse:\n");
    printf(" - Ищите сравнение целевого номера.\n");
    printf(" - Ищите целевую сумму 1_000_000_000.\n");
    printf(" - Флаг декодируется в рантайме (не plaintext в бинаре).\n");
    line();
}

static void seed_state(AppState *st) {
    st->balance = 0;
    st->operations = 0;
    st->running = 1;
    st->suspicious_counter = 0;
    st->last_transfer_phone[0] = '\0';
    st->last_transfer_amount = 0;
}

static void print_intro_story(void) {
    printf("Добро пожаловать в демонстрационный клиент Ы-Банка.\n");
    printf("Это специальная сборка для reverse engineering задания.\n");
    printf("В интерфейсе есть базовые операции:\n");
    printf("  • Перевод\n");
    printf("  • Пополнить\n");
    printf("  • Сколько денег на счету\n");
    printf("Цель исследователя — понять скрытую логику и получить секрет.\n");
    line();
}

static void noisy_status_line(const AppState *st) {
    /* Немного шума для усложнения статического анализа */
    long long x = st->balance;
    long long y = st->operations + 1;
    long long z = (x ^ (y * 1337)) + (x % 97);
    if ((z & 1LL) == 0LL) {
        printf("[diag] channel=A op=%d\n", st->operations);
    } else {
        printf("[diag] channel=B op=%d\n", st->operations);
    }
}

static void print_menu_header(const AppState *st) {
    line();
    printf("Баланс: %lld RUB | Операций: %d\n", st->balance, st->operations);
    line();
}

static void print_exit_summary(const AppState *st) {
    line();
    printf("Завершение работы приложения.\n");
    printf("Финальный баланс: %lld RUB\n", st->balance);
    printf("Всего операций: %d\n", st->operations);
    if (st->last_transfer_phone[0]) {
        printf("Последний перевод: %lld RUB на %s\n",
               st->last_transfer_amount,
               st->last_transfer_phone);
    } else {
        printf("Переводы не выполнялись.\n");
    }
    line();
}

static void filler_block_01(void) {
    volatile int a = 1;
    volatile int b = 2;
    volatile int c = a + b;
    if (c == 1234567) {
        printf("%d\n", c);
    }
}

static void filler_block_02(void) {
    volatile int a = 3;
    volatile int b = 4;
    volatile int c = a * b;
    if (c == 7654321) {
        printf("%d\n", c);
    }
}

static void filler_block_03(void) {
    volatile int a = 5;
    volatile int b = 6;
    volatile int c = a - b;
    if (c == 13579) {
        printf("%d\n", c);
    }
}

static void filler_block_04(void) {
    volatile int a = 7;
    volatile int b = 8;
    volatile int c = a ^ b;
    if (c == 24680) {
        printf("%d\n", c);
    }
}

static void filler_block_05(void) {
    volatile int a = 9;
    volatile int b = 10;
    volatile int c = a | b;
    if (c == 31415) {
        printf("%d\n", c);
    }
}

static void filler_block_06(void) {
    volatile int a = 11;
    volatile int b = 12;
    volatile int c = a & b;
    if (c == 27182) {
        printf("%d\n", c);
    }
}

static void filler_block_07(void) {
    volatile int a = 13;
    volatile int b = 14;
    volatile int c = (a << 1) + b;
    if (c == 16180) {
        printf("%d\n", c);
    }
}

static void filler_block_08(void) {
    volatile int a = 15;
    volatile int b = 16;
    volatile int c = (b >> 1) + a;
    if (c == 14142) {
        printf("%d\n", c);
    }
}

static void filler_block_09(void) {
    volatile int a = 17;
    volatile int b = 18;
    volatile int c = a * a + b * b;
    if (c == 17320) {
        printf("%d\n", c);
    }
}

static void filler_block_10(void) {
    volatile int a = 19;
    volatile int b = 20;
    volatile int c = (a + b) * (a - b);
    if (c == 22360) {
        printf("%d\n", c);
    }
}

static void filler_blocks_touch(void) {
    filler_block_01();
    filler_block_02();
    filler_block_03();
    filler_block_04();
    filler_block_05();
    filler_block_06();
    filler_block_07();
    filler_block_08();
    filler_block_09();
    filler_block_10();
}

int main(void) {
    AppState st;
    seed_state(&st);

    print_intro_story();
    print_legal_disclaimer();
    print_help_block();
    filler_blocks_touch();

    while (st.running) {
        print_menu_header(&st);
        print_banner();
        noisy_status_line(&st);

        int choice = read_menu_choice();
        switch (choice) {
            case 1:
                transfer_money(&st);
                break;
            case 2:
                top_up(&st);
                break;
            case 3:
                st.operations++;
                print_balance(&st);
                break;
            case 4:
                st.operations++;
                print_transfer_logs();
                break;
            case 5:
                st.running = 0;
                break;
            default:
                printf("Неизвестный пункт меню.\n");
                line();
                break;
        }
    }

    print_exit_summary(&st);
    return 0;
}
