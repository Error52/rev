#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>

#define ID_EDIT_PHONE        103
#define ID_EDIT_TRANSFER     104
#define ID_BTN_TRANSFER      105
#define ID_LBL_BALANCE       106
#define ID_LIST_HISTORY      107
#define ID_LBL_SECRET        108
#define ID_HEADER_TITLE      109
#define ID_HEADER_SUB        110
#define ID_TAB_DIALOGS       111
#define ID_TAB_PAYMENTS      112
#define ID_TAB_PROFILE       113

static const long long TARGET_AMOUNT = 1000000000LL;
static const wchar_t *TARGET_PHONE = L"8925553525";

static long long g_balance = 732;
static HWND g_hBalance = NULL;
static HWND g_hHistory = NULL;
static HWND g_hSecret = NULL;

static HFONT g_fontTitle = NULL;
static HFONT g_fontNormal = NULL;
static HFONT g_fontSmall = NULL;
static HFONT g_fontMono = NULL;

static HBRUSH g_bgBrush = NULL;
static HBRUSH g_cardBrush = NULL;
static HBRUSH g_headerBrush = NULL;
static HBRUSH g_accentBrush = NULL;
static HBRUSH g_whiteBrush = NULL;

static COLORREF g_textMain = RGB(24, 34, 58);
static COLORREF g_textMuted = RGB(120, 131, 150);

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
    if (count > 0) SendMessageW(g_hHistory, LB_SETTOPINDEX, (WPARAM)(count - 1), 0);
}

static void set_secret_text(const wchar_t *text) {
    SetWindowTextW(g_hSecret, text);
}

static int maybe_unlock(const wchar_t *phone, long long amount) {
    return g_balance >= TARGET_AMOUNT && amount >= TARGET_AMOUNT && wcscmp(phone, TARGET_PHONE) == 0;
}

static void show_error(HWND hwnd, const wchar_t *msg) {
    MessageBoxW(hwnd, msg, L"Ошибка", MB_OK | MB_ICONERROR);
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

    wchar_t logline[200];
    swprintf(logline, sizeof(logline)/sizeof(logline[0]), L"Перевод -%lld ₽ → %ls", amount, phone);
    add_history(logline);

    if (unlocked) {
        wchar_t flag[128];
        wchar_t out[256];
        decode_flag(flag, sizeof(flag)/sizeof(flag[0]));
        swprintf(out, sizeof(out)/sizeof(out[0]), L"[challenge] %ls", flag);
        set_secret_text(out);
    } else {
        set_secret_text(L"Перевод выполнен");
    }
}

static HFONT make_font(int h, int w, const wchar_t *name) {
    return CreateFontW(h, 0, 0, 0, w, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, VARIABLE_PITCH, name);
}

static void create_ui(HWND hwnd) {
    HINSTANCE hInst = (HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE);

    g_fontTitle = make_font(34, FW_BOLD, L"Segoe UI");
    g_fontNormal = make_font(20, FW_NORMAL, L"Segoe UI");
    g_fontSmall = make_font(17, FW_NORMAL, L"Segoe UI");
    g_fontMono = make_font(17, FW_NORMAL, L"Consolas");

    HWND hTitle = CreateWindowW(L"STATIC", L"Диалоги",
        WS_CHILD | WS_VISIBLE, 24, 18, 180, 40,
        hwnd, (HMENU)ID_HEADER_TITLE, hInst, NULL);
    SendMessageW(hTitle, WM_SETFONT, (WPARAM)g_fontTitle, TRUE);

    HWND hSub = CreateWindowW(L"STATIC", L"Ы-Банк • Secure Messaging",
        WS_CHILD | WS_VISIBLE, 24, 58, 260, 24,
        hwnd, (HMENU)ID_HEADER_SUB, hInst, NULL);
    SendMessageW(hSub, WM_SETFONT, (WPARAM)g_fontSmall, TRUE);

    HWND hTab1 = CreateWindowW(L"STATIC", L"Диалоги",
        WS_CHILD | WS_VISIBLE, 24, 102, 90, 22,
        hwnd, (HMENU)ID_TAB_DIALOGS, hInst, NULL);
    SendMessageW(hTab1, WM_SETFONT, (WPARAM)g_fontSmall, TRUE);

    HWND hTab2 = CreateWindowW(L"STATIC", L"Платежи",
        WS_CHILD | WS_VISIBLE, 128, 102, 90, 22,
        hwnd, (HMENU)ID_TAB_PAYMENTS, hInst, NULL);
    SendMessageW(hTab2, WM_SETFONT, (WPARAM)g_fontSmall, TRUE);

    HWND hTab3 = CreateWindowW(L"STATIC", L"Профиль",
        WS_CHILD | WS_VISIBLE, 232, 102, 90, 22,
        hwnd, (HMENU)ID_TAB_PROFILE, hInst, NULL);
    SendMessageW(hTab3, WM_SETFONT, (WPARAM)g_fontSmall, TRUE);

    g_hBalance = CreateWindowW(L"STATIC", L"Баланс: 0 ₽",
        WS_CHILD | WS_VISIBLE, 24, 136, 320, 36,
        hwnd, (HMENU)ID_LBL_BALANCE, hInst, NULL);
    SendMessageW(g_hBalance, WM_SETFONT, (WPARAM)g_fontNormal, TRUE);

    CreateWindowW(L"STATIC", L"Кому:", WS_CHILD | WS_VISIBLE,
        24, 180, 90, 22, hwnd, NULL, hInst, NULL);
    HWND hPhone = CreateWindowW(L"EDIT", L"8925553525",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        24, 204, 200, 32, hwnd, (HMENU)ID_EDIT_PHONE, hInst, NULL);
    SendMessageW(hPhone, WM_SETFONT, (WPARAM)g_fontSmall, TRUE);

    HWND hAmount = CreateWindowW(L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        232, 204, 140, 32, hwnd, (HMENU)ID_EDIT_TRANSFER, hInst, NULL);
    SendMessageW(hAmount, WM_SETFONT, (WPARAM)g_fontSmall, TRUE);

    HWND hTransferBtn = CreateWindowW(L"BUTTON", L"Перевести",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        24, 248, 348, 36, hwnd, (HMENU)ID_BTN_TRANSFER, hInst, NULL);
    SendMessageW(hTransferBtn, WM_SETFONT, (WPARAM)g_fontNormal, TRUE);

    CreateWindowW(L"STATIC", L"Сообщения:", WS_CHILD | WS_VISIBLE,
        24, 300, 120, 22, hwnd, NULL, hInst, NULL);
    g_hHistory = CreateWindowW(L"LISTBOX", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL,
        24, 324, 348, 200, hwnd, (HMENU)ID_LIST_HISTORY, hInst, NULL);
    SendMessageW(g_hHistory, WM_SETFONT, (WPARAM)g_fontSmall, TRUE);

    g_hSecret = CreateWindowW(L"STATIC", L"Ожидание...",
        WS_CHILD | WS_VISIBLE,
        24, 536, 348, 40, hwnd, (HMENU)ID_LBL_SECRET, hInst, NULL);
    SendMessageW(g_hSecret, WM_SETFONT, (WPARAM)g_fontMono, TRUE);

    update_balance_label();
    add_history(L"Добро пожаловать в Ы-Банк");
}

static void draw_phone_style(HWND hwnd, HDC hdc) {
    RECT rc;
    GetClientRect(hwnd, &rc);

    FillRect(hdc, &rc, g_bgBrush);

    RECT phone = {10, 10, rc.right - 10, rc.bottom - 10};
    HBRUSH phoneBrush = CreateSolidBrush(RGB(246, 248, 251));
    FillRect(hdc, &phone, phoneBrush);
    DeleteObject(phoneBrush);

    HPEN borderPen = CreatePen(PS_SOLID, 2, RGB(40, 53, 70));
    HGDIOBJ oldPen = SelectObject(hdc, borderPen);
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
    Rectangle(hdc, phone.left, phone.top, phone.right, phone.bottom);

    RECT topbar = {phone.left + 1, phone.top + 1, phone.right - 1, phone.top + 126};
    FillRect(hdc, &topbar, g_headerBrush);

    RECT tabline = {24, 126, 108, 130};
    FillRect(hdc, &tabline, g_accentBrush);

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(borderPen);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            g_bgBrush = CreateSolidBrush(RGB(198, 206, 214));
            g_cardBrush = CreateSolidBrush(RGB(246, 248, 251));
            g_headerBrush = CreateSolidBrush(RGB(39, 168, 170));
            g_accentBrush = CreateSolidBrush(RGB(245, 255, 255));
            g_whiteBrush = CreateSolidBrush(RGB(246, 248, 251));
            create_ui(hwnd);
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_BTN_TRANSFER:
                    on_transfer(hwnd);
                    return 0;
            }
            break;

        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wParam;
            HWND ctl = (HWND)lParam;
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, g_textMain);

            if (GetDlgCtrlID(ctl) == ID_HEADER_TITLE || GetDlgCtrlID(ctl) == ID_HEADER_SUB ||
                GetDlgCtrlID(ctl) == ID_TAB_DIALOGS || GetDlgCtrlID(ctl) == ID_TAB_PAYMENTS ||
                GetDlgCtrlID(ctl) == ID_TAB_PROFILE) {
                SetTextColor(hdc, RGB(255, 255, 255));
                return (LRESULT)g_headerBrush;
            }

            if (ctl == g_hSecret) {
                SetTextColor(hdc, RGB(10, 106, 77));
            }
            return (LRESULT)g_whiteBrush;
        }

        case WM_CTLCOLOREDIT: {
            HDC hdc = (HDC)wParam;
            SetBkMode(hdc, OPAQUE);
            SetBkColor(hdc, RGB(255, 255, 255));
            SetTextColor(hdc, g_textMain);
            return (LRESULT)GetStockObject(WHITE_BRUSH);
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            draw_phone_style(hwnd, hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_DESTROY:
            if (g_fontTitle) DeleteObject(g_fontTitle);
            if (g_fontNormal) DeleteObject(g_fontNormal);
            if (g_fontSmall) DeleteObject(g_fontSmall);
            if (g_fontMono) DeleteObject(g_fontMono);
            if (g_bgBrush) DeleteObject(g_bgBrush);
            if (g_cardBrush) DeleteObject(g_cardBrush);
            if (g_headerBrush) DeleteObject(g_headerBrush);
            if (g_accentBrush) DeleteObject(g_accentBrush);
            if (g_whiteBrush) DeleteObject(g_whiteBrush);
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;

    const wchar_t cls[] = L"YBankMobileStyleWindow";

    WNDCLASSW wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = cls;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassW(&wc)) {
        MessageBoxW(NULL, L"Ошибка регистрации класса", L"Ошибка", MB_OK | MB_ICONERROR);
        return 1;
    }

    HWND hwnd = CreateWindowW(
        cls,
        L"Ы-Банк — Mobile Style EXE",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        410,
        690,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hwnd) {
        MessageBoxW(NULL, L"Ошибка создания окна", L"Ошибка", MB_OK | MB_ICONERROR);
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
