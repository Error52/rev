#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define ID_EDIT_TOPUP      101
#define ID_BTN_TOPUP       102
#define ID_EDIT_PHONE      103
#define ID_EDIT_TRANSFER   104
#define ID_BTN_TRANSFER    105
#define ID_BALANCE_LABEL   106
#define ID_HISTORY_LIST    107
#define ID_SECRET_LABEL    108

static long long g_balance = 0;
static HWND g_hBalance = NULL;
static HWND g_hHistory = NULL;
static HWND g_hSecret = NULL;

static const long long TARGET_AMOUNT = 1000000000LL;
static const char *TARGET_PHONE = "8925553525";

static uint8_t rotr8(uint8_t v, int s) {
    return (uint8_t)((v >> s) | (v << (8 - s)));
}

static void decode_flag(char *out, size_t out_sz) {
    static const uint8_t encoded[] = {
        0x48, 0xea, 0x91, 0xe9, 0x13, 0x00, 0xf3, 0xe2,
        0x28, 0xf9, 0x5b, 0xbb, 0xa0, 0xaa, 0x10, 0x08,
        0xe9, 0x4b, 0x03, 0x61, 0xd9, 0xe0, 0x73, 0xa9,
        0x53, 0x5b, 0x88, 0xd9, 0x58, 0xaa, 0x03, 0xa1,
        0x32, 0x88, 0x9b, 0x81, 0x2a, 0xd9, 0x4b, 0x32,
        0xf8, 0xf1, 0xc0, 0x62
    };
    static const uint8_t key[] = {0x6f, 0x31, 0x53, 0x5a, 0x19, 0x73, 0x0a};

    size_t n = sizeof(encoded) / sizeof(encoded[0]);
    if (out_sz < n + 1) {
        if (out_sz > 0) out[0] = '\0';
        return;
    }

    for (size_t i = 0; i < n; i++) {
        uint8_t y = rotr8(encoded[i], 3);
        uint8_t z = (uint8_t)(y ^ key[i % (sizeof(key) / sizeof(key[0]))]);
        out[i] = (char)z;
    }
    out[n] = '\0';
}

static int is_digits_only(const char *s) {
    if (!s || !*s) return 0;
    while (*s) {
        if (*s < '0' || *s > '9') return 0;
        s++;
    }
    return 1;
}

static long long parse_ll(const char *s) {
    char *end = NULL;
    long long v = strtoll(s, &end, 10);
    if (end == s || *end != '\0') return -1;
    return v;
}

static void set_balance_label(void) {
    char buf[128];
    snprintf(buf, sizeof(buf), "Баланс: %lld RUB", g_balance);
    SetWindowTextA(g_hBalance, buf);
}

static void add_history(const char *text) {
    SendMessageA(g_hHistory, LB_ADDSTRING, 0, (LPARAM)text);
    int count = (int)SendMessageA(g_hHistory, LB_GETCOUNT, 0, 0);
    if (count > 0) {
        SendMessageA(g_hHistory, LB_SETTOPINDEX, (WPARAM)(count - 1), 0);
    }
}

static void set_secret(const char *text) {
    SetWindowTextA(g_hSecret, text);
}

static int maybe_unlock(const char *phone, long long amount) {
    return g_balance >= TARGET_AMOUNT && amount >= TARGET_AMOUNT && strcmp(phone, TARGET_PHONE) == 0;
}

static void on_topup(HWND hwnd) {
    char raw[64];
    GetWindowTextA(GetDlgItem(hwnd, ID_EDIT_TOPUP), raw, sizeof(raw));

    long long amount = parse_ll(raw);
    if (amount <= 0) {
        MessageBoxA(hwnd, "Некорректная сумма пополнения", "Ошибка", MB_OK | MB_ICONERROR);
        return;
    }

    if (amount > 5000000000LL) {
        MessageBoxA(hwnd, "Слишком большая сумма для одного пополнения", "Ошибка", MB_OK | MB_ICONERROR);
        return;
    }

    g_balance += amount;
    set_balance_label();

    char msg[160];
    snprintf(msg, sizeof(msg), "Пополнение: +%lld RUB", amount);
    add_history(msg);
    set_secret("Операция пополнения выполнена");
}

static void on_transfer(HWND hwnd) {
    char phone[64];
    char raw_amount[64];

    GetWindowTextA(GetDlgItem(hwnd, ID_EDIT_PHONE), phone, sizeof(phone));
    GetWindowTextA(GetDlgItem(hwnd, ID_EDIT_TRANSFER), raw_amount, sizeof(raw_amount));

    if (!is_digits_only(phone) || strlen(phone) < 10 || strlen(phone) > 15) {
        MessageBoxA(hwnd, "Номер должен содержать 10-15 цифр", "Ошибка", MB_OK | MB_ICONERROR);
        return;
    }

    long long amount = parse_ll(raw_amount);
    if (amount <= 0) {
        MessageBoxA(hwnd, "Некорректная сумма перевода", "Ошибка", MB_OK | MB_ICONERROR);
        return;
    }

    if (amount > g_balance) {
        MessageBoxA(hwnd, "Недостаточно средств", "Ошибка", MB_OK | MB_ICONERROR);
        return;
    }

    int unlocked = maybe_unlock(phone, amount);

    g_balance -= amount;
    set_balance_label();

    char msg[200];
    snprintf(msg, sizeof(msg), "Перевод: -%lld RUB -> %s", amount, phone);
    add_history(msg);

    if (unlocked) {
        char flag[128];
        char out[180];
        decode_flag(flag, sizeof(flag));
        snprintf(out, sizeof(out), "[challenge] secret unlocked: %s", flag);
        set_secret(out);
    } else {
        set_secret("Перевод выполнен успешно");
    }
}

static HFONT create_font(int size, int weight) {
    return CreateFontA(
        size, 0, 0, 0, weight, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, VARIABLE_PITCH, "Segoe UI"
    );
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HFONT fTitle = NULL;
    static HFONT fNormal = NULL;

    switch (msg) {
        case WM_CREATE: {
            HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;

            fTitle = create_font(28, FW_BOLD);
            fNormal = create_font(20, FW_NORMAL);

            HWND hTitle = CreateWindowA("STATIC", "Ы-Банк — Windows GUI",
                WS_VISIBLE | WS_CHILD, 20, 15, 520, 36,
                hwnd, NULL, hInst, NULL);
            SendMessageA(hTitle, WM_SETFONT, (WPARAM)fTitle, TRUE);

            g_hBalance = CreateWindowA("STATIC", "Баланс: 0 RUB",
                WS_VISIBLE | WS_CHILD, 20, 60, 320, 30,
                hwnd, (HMENU)ID_BALANCE_LABEL, hInst, NULL);
            SendMessageA(g_hBalance, WM_SETFONT, (WPARAM)fNormal, TRUE);

            CreateWindowA("STATIC", "Пополнение:", WS_VISIBLE | WS_CHILD,
                20, 110, 120, 24, hwnd, NULL, hInst, NULL);

            HWND hTopup = CreateWindowA("EDIT", "",
                WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                20, 138, 220, 28, hwnd, (HMENU)ID_EDIT_TOPUP, hInst, NULL);
            SendMessageA(hTopup, WM_SETFONT, (WPARAM)fNormal, TRUE);

            HWND hTopupBtn = CreateWindowA("BUTTON", "Пополнить",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                255, 138, 140, 30, hwnd, (HMENU)ID_BTN_TOPUP, hInst, NULL);
            SendMessageA(hTopupBtn, WM_SETFONT, (WPARAM)fNormal, TRUE);

            CreateWindowA("STATIC", "Перевод на счёт/телефон:", WS_VISIBLE | WS_CHILD,
                20, 190, 220, 24, hwnd, NULL, hInst, NULL);

            HWND hPhone = CreateWindowA("EDIT", "8925553525",
                WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                20, 218, 220, 28, hwnd, (HMENU)ID_EDIT_PHONE, hInst, NULL);
            SendMessageA(hPhone, WM_SETFONT, (WPARAM)fNormal, TRUE);

            HWND hAmount = CreateWindowA("EDIT", "",
                WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                255, 218, 140, 28, hwnd, (HMENU)ID_EDIT_TRANSFER, hInst, NULL);
            SendMessageA(hAmount, WM_SETFONT, (WPARAM)fNormal, TRUE);

            HWND hTransferBtn = CreateWindowA("BUTTON", "Перевести",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                410, 218, 120, 30, hwnd, (HMENU)ID_BTN_TRANSFER, hInst, NULL);
            SendMessageA(hTransferBtn, WM_SETFONT, (WPARAM)fNormal, TRUE);

            CreateWindowA("STATIC", "История:", WS_VISIBLE | WS_CHILD,
                20, 265, 100, 24, hwnd, NULL, hInst, NULL);

            g_hHistory = CreateWindowA("LISTBOX", "",
                WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_NOTIFY | WS_VSCROLL,
                20, 292, 510, 140, hwnd, (HMENU)ID_HISTORY_LIST, hInst, NULL);
            SendMessageA(g_hHistory, WM_SETFONT, (WPARAM)fNormal, TRUE);

            CreateWindowA("STATIC", "Секрет:", WS_VISIBLE | WS_CHILD,
                20, 445, 100, 24, hwnd, NULL, hInst, NULL);

            g_hSecret = CreateWindowA("STATIC", "Ожидание...",
                WS_VISIBLE | WS_CHILD,
                20, 472, 510, 40, hwnd, (HMENU)ID_SECRET_LABEL, hInst, NULL);
            SendMessageA(g_hSecret, WM_SETFONT, (WPARAM)fNormal, TRUE);

            add_history("Приложение запущено");
            return 0;
        }

        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case ID_BTN_TOPUP:
                    on_topup(hwnd);
                    return 0;
                case ID_BTN_TRANSFER:
                    on_transfer(hwnd);
                    return 0;
            }
            break;
        }

        case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = (HDC)wParam;
            SetTextColor(hdcStatic, RGB(25, 40, 90));
            SetBkMode(hdcStatic, TRANSPARENT);
            return (INT_PTR)GetStockObject(WHITE_BRUSH);
        }

        case WM_DESTROY:
            if (fTitle) DeleteObject(fTitle);
            if (fNormal) DeleteObject(fNormal);
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;

    const char CLASS_NAME[] = "YBankGUIWindowClass";

    WNDCLASSA wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassA(&wc)) {
        MessageBoxA(NULL, "Не удалось зарегистрировать класс окна", "Ошибка", MB_OK | MB_ICONERROR);
        return 1;
    }

    HWND hwnd = CreateWindowA(
        CLASS_NAME,
        "Ы-Банк — EXE GUI",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        570,
        580,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hwnd) {
        MessageBoxA(NULL, "Не удалось создать окно", "Ошибка", MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
