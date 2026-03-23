#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>

#define ID_EDIT_TOPUP        101
#define ID_BTN_TOPUP         102
#define ID_EDIT_PHONE        103
#define ID_EDIT_TRANSFER     104
#define ID_BTN_TRANSFER      105
#define ID_LBL_BALANCE       106
#define ID_LIST_HISTORY      107
#define ID_LBL_SECRET        108
#define ID_LBL_HEADER        109
#define ID_LBL_SUBHEADER     110

static const long long TARGET_AMOUNT = 1000000000LL;
static const wchar_t *TARGET_PHONE = L"8925553525";

static long long g_balance = 0;
static HWND g_hBalance = NULL;
static HWND g_hHistory = NULL;
static HWND g_hSecret = NULL;

static HFONT g_fontTitle = NULL;
static HFONT g_fontNormal = NULL;
static HFONT g_fontMono = NULL;

static HBRUSH g_bgBrush = NULL;
static HBRUSH g_cardBrush = NULL;
static HBRUSH g_whiteBrush = NULL;
static COLORREF g_textColor = RGB(31, 45, 76);

static uint8_t rotr8(uint8_t v, int s) {
    return (uint8_t)((v >> s) | (v << (8 - s)));
}

static void decode_flag(wchar_t *out, size_t out_sz) {
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
        if (out_sz > 0) out[0] = L'\0';
        return;
    }

    for (size_t i = 0; i < n; i++) {
        uint8_t y = rotr8(encoded[i], 3);
        uint8_t z = (uint8_t)(y ^ key[i % (sizeof(key) / sizeof(key[0]))]);
        out[i] = (wchar_t)z;
    }
    out[n] = L'\0';
}

static int is_digits_only(const wchar_t *s) {
    if (!s || !*s) return 0;
    while (*s) {
        if (*s < L'0' || *s > L'9') return 0;
        s++;
    }
    return 1;
}

static long long parse_ll(const wchar_t *s) {
    wchar_t *end = NULL;
    long long v = wcstoll(s, &end, 10);
    if (end == s || *end != L'\0') return -1;
    return v;
}

static void update_balance_label(void) {
    wchar_t text[128];
    swprintf(text, sizeof(text) / sizeof(text[0]), L"Баланс: %lld ₽", g_balance);
    SetWindowTextW(g_hBalance, text);
}

static void add_history(const wchar_t *line) {
    SendMessageW(g_hHistory, LB_ADDSTRING, 0, (LPARAM)line);
    int count = (int)SendMessageW(g_hHistory, LB_GETCOUNT, 0, 0);
    if (count > 0) {
        SendMessageW(g_hHistory, LB_SETTOPINDEX, (WPARAM)(count - 1), 0);
    }
}

static void set_secret_text(const wchar_t *text) {
    SetWindowTextW(g_hSecret, text);
}

static int maybe_unlock(const wchar_t *phone, long long amount) {
    return (g_balance >= TARGET_AMOUNT && amount >= TARGET_AMOUNT && wcscmp(phone, TARGET_PHONE) == 0);
}

static void show_error(HWND hwnd, const wchar_t *msg) {
    MessageBoxW(hwnd, msg, L"Ошибка", MB_OK | MB_ICONERROR);
}

static void on_topup(HWND hwnd) {
    wchar_t raw[64];
    GetWindowTextW(GetDlgItem(hwnd, ID_EDIT_TOPUP), raw, 64);

    long long amount = parse_ll(raw);
    if (amount <= 0) {
        show_error(hwnd, L"Некорректная сумма пополнения");
        return;
    }
    if (amount > 5000000000LL) {
        show_error(hwnd, L"Слишком большая сумма для одного пополнения");
        return;
    }

    g_balance += amount;
    update_balance_label();

    wchar_t logline[180];
    swprintf(logline, sizeof(logline) / sizeof(logline[0]), L"Пополнение: +%lld ₽", amount);
    add_history(logline);
    set_secret_text(L"Операция пополнения выполнена");
}

static void on_transfer(HWND hwnd) {
    wchar_t phone[64];
    wchar_t rawAmount[64];

    GetWindowTextW(GetDlgItem(hwnd, ID_EDIT_PHONE), phone, 64);
    GetWindowTextW(GetDlgItem(hwnd, ID_EDIT_TRANSFER), rawAmount, 64);

    if (!is_digits_only(phone) || wcslen(phone) < 10 || wcslen(phone) > 15) {
        show_error(hwnd, L"Номер должен содержать 10-15 цифр");
        return;
    }

    long long amount = parse_ll(rawAmount);
    if (amount <= 0) {
        show_error(hwnd, L"Некорректная сумма перевода");
        return;
    }

    if (amount > g_balance) {
        show_error(hwnd, L"Недостаточно средств");
        return;
    }

    int unlocked = maybe_unlock(phone, amount);

    g_balance -= amount;
    update_balance_label();

    wchar_t logline[220];
    swprintf(logline, sizeof(logline) / sizeof(logline[0]), L"Перевод: -%lld ₽ → %ls", amount, phone);
    add_history(logline);

    if (unlocked) {
        wchar_t flag[128];
        wchar_t out[256];
        decode_flag(flag, sizeof(flag) / sizeof(flag[0]));
        swprintf(out, sizeof(out) / sizeof(out[0]), L"[challenge] secret unlocked: %ls", flag);
        set_secret_text(out);
    } else {
        set_secret_text(L"Перевод выполнен успешно");
    }
}

static HFONT make_font(int height, int weight) {
    return CreateFontW(
        height, 0, 0, 0, weight, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, VARIABLE_PITCH, L"Segoe UI"
    );
}

static void create_ui(HWND hwnd) {
    HINSTANCE hInst = (HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE);

    g_fontTitle = make_font(36, FW_BOLD);
    g_fontNormal = make_font(20, FW_NORMAL);
    g_fontMono = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, FIXED_PITCH, L"Consolas");

    HWND hHeader = CreateWindowW(L"STATIC", L"Ы-Банк",
        WS_CHILD | WS_VISIBLE,
        28, 18, 220, 48,
        hwnd, (HMENU)ID_LBL_HEADER, hInst, NULL);
    SendMessageW(hHeader, WM_SETFONT, (WPARAM)g_fontTitle, TRUE);

    HWND hSub = CreateWindowW(L"STATIC", L"Премиум клиент • Защищённые переводы",
        WS_CHILD | WS_VISIBLE,
        28, 64, 380, 28,
        hwnd, (HMENU)ID_LBL_SUBHEADER, hInst, NULL);
    SendMessageW(hSub, WM_SETFONT, (WPARAM)g_fontNormal, TRUE);

    g_hBalance = CreateWindowW(L"STATIC", L"Баланс: 0 ₽",
        WS_CHILD | WS_VISIBLE,
        28, 112, 380, 38,
        hwnd, (HMENU)ID_LBL_BALANCE, hInst, NULL);
    SendMessageW(g_hBalance, WM_SETFONT, (WPARAM)g_fontTitle, TRUE);

    CreateWindowW(L"STATIC", L"Пополнить счёт:", WS_CHILD | WS_VISIBLE,
        28, 180, 180, 24, hwnd, NULL, hInst, NULL);

    HWND hTopup = CreateWindowW(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        28, 208, 250, 34, hwnd, (HMENU)ID_EDIT_TOPUP, hInst, NULL);
    SendMessageW(hTopup, WM_SETFONT, (WPARAM)g_fontNormal, TRUE);

    HWND hTopupBtn = CreateWindowW(L"BUTTON", L"Пополнить",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        290, 208, 160, 36, hwnd, (HMENU)ID_BTN_TOPUP, hInst, NULL);
    SendMessageW(hTopupBtn, WM_SETFONT, (WPARAM)g_fontNormal, TRUE);

    CreateWindowW(L"STATIC", L"Перевод:", WS_CHILD | WS_VISIBLE,
        28, 262, 180, 24, hwnd, NULL, hInst, NULL);

    HWND hPhone = CreateWindowW(L"EDIT", L"8925553525",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        28, 290, 250, 34, hwnd, (HMENU)ID_EDIT_PHONE, hInst, NULL);
    SendMessageW(hPhone, WM_SETFONT, (WPARAM)g_fontNormal, TRUE);

    HWND hAmount = CreateWindowW(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        290, 290, 160, 34, hwnd, (HMENU)ID_EDIT_TRANSFER, hInst, NULL);
    SendMessageW(hAmount, WM_SETFONT, (WPARAM)g_fontNormal, TRUE);

    HWND hTransferBtn = CreateWindowW(L"BUTTON", L"Перевести",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        460, 290, 160, 36, hwnd, (HMENU)ID_BTN_TRANSFER, hInst, NULL);
    SendMessageW(hTransferBtn, WM_SETFONT, (WPARAM)g_fontNormal, TRUE);

    CreateWindowW(L"STATIC", L"История операций:", WS_CHILD | WS_VISIBLE,
        28, 340, 220, 24, hwnd, NULL, hInst, NULL);

    g_hHistory = CreateWindowW(L"LISTBOX", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL,
        28, 368, 592, 140, hwnd, (HMENU)ID_LIST_HISTORY, hInst, NULL);
    SendMessageW(g_hHistory, WM_SETFONT, (WPARAM)g_fontNormal, TRUE);

    CreateWindowW(L"STATIC", L"Секрет:", WS_CHILD | WS_VISIBLE,
        28, 518, 120, 24, hwnd, NULL, hInst, NULL);

    g_hSecret = CreateWindowW(L"STATIC", L"Ожидание...",
        WS_CHILD | WS_VISIBLE,
        28, 546, 592, 32, hwnd, (HMENU)ID_LBL_SECRET, hInst, NULL);
    SendMessageW(g_hSecret, WM_SETFONT, (WPARAM)g_fontMono, TRUE);

    add_history(L"Система запущена");
}

static void draw_bank_background(HWND hwnd, HDC hdc) {
    RECT rc;
    GetClientRect(hwnd, &rc);

    FillRect(hdc, &rc, g_bgBrush);

    RECT header = {0, 0, rc.right, 96};
    HBRUSH headerBrush = CreateSolidBrush(RGB(18, 47, 103));
    FillRect(hdc, &header, headerBrush);
    DeleteObject(headerBrush);

    RECT card = {14, 100, rc.right - 14, rc.bottom - 10};
    FillRect(hdc, &card, g_cardBrush);

    HPEN pen = CreatePen(PS_SOLID, 1, RGB(200, 210, 230));
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
    Rectangle(hdc, card.left, card.top, card.right, card.bottom);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            g_bgBrush = CreateSolidBrush(RGB(236, 242, 252));
            g_cardBrush = CreateSolidBrush(RGB(255, 255, 255));
            g_whiteBrush = CreateSolidBrush(RGB(255, 255, 255));
            create_ui(hwnd);
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_BTN_TOPUP:
                    on_topup(hwnd);
                    return 0;
                case ID_BTN_TRANSFER:
                    on_transfer(hwnd);
                    return 0;
            }
            break;

        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wParam;
            HWND hCtl = (HWND)lParam;
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, g_textColor);
            if (hCtl == g_hSecret) {
                SetTextColor(hdc, RGB(14, 90, 70));
            }
            return (LRESULT)g_whiteBrush;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            draw_bank_background(hwnd, hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_DESTROY:
            if (g_fontTitle) DeleteObject(g_fontTitle);
            if (g_fontNormal) DeleteObject(g_fontNormal);
            if (g_fontMono) DeleteObject(g_fontMono);
            if (g_bgBrush) DeleteObject(g_bgBrush);
            if (g_cardBrush) DeleteObject(g_cardBrush);
            if (g_whiteBrush) DeleteObject(g_whiteBrush);
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;

    const wchar_t CLASS_NAME[] = L"YBankWindowClass";

    WNDCLASSW wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassW(&wc)) {
        MessageBoxW(NULL, L"Ошибка регистрации класса окна", L"Ошибка", MB_OK | MB_ICONERROR);
        return 1;
    }

    HWND hwnd = CreateWindowW(
        CLASS_NAME,
        L"Ы-Банк — Графический EXE",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        670,
        650,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hwnd) {
        MessageBoxW(NULL, L"Ошибка создания главного окна", L"Ошибка", MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}
