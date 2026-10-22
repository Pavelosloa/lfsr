#include <windows.h>
const int ROWS = 4;
const int COLS = 5;
const int PAD = 4;
const int MIN_SCALE = 10;
const int MAX_SCALE = 400;

struct FontSpec {
    const char *face;
    int height;
    int weight;
    BOOL italic;
    BOOL underline;
    COLORREF color;
};

const FontSpec SPECS[] = {
    {"Arial", 20, FW_NORMAL, FALSE, FALSE, RGB(200, 0, 0)},
    {"Times New Roman", 22, FW_BOLD, FALSE, FALSE, RGB(0, 0, 0)},
    {"Courier New", 20, FW_NORMAL, TRUE, FALSE, RGB(0, 130, 0)},
    {"Georgia", 22, FW_BOLD, TRUE, FALSE, RGB(128, 0, 160)},
    {"Verdana", 18, FW_NORMAL, FALSE, TRUE, RGB(0, 80, 200)},
    {"Tahoma", 20, FW_BOLD, FALSE, TRUE, RGB(220, 110, 0)},
    {"Impact", 26, FW_NORMAL, FALSE, FALSE, RGB(90, 90, 90)},
    {"Comic Sans MS", 20, FW_NORMAL, TRUE, FALSE, RGB(180, 0, 120)},
    {"Trebuchet MS", 22, FW_BOLD, TRUE, TRUE, RGB(0, 120, 120)},
    {"Palatino Linotype", 22, FW_NORMAL, FALSE, FALSE, RGB(120, 60, 0)},
    {"Consolas", 20, FW_BOLD, FALSE, FALSE, RGB(60, 60, 200)},
    {"Lucida Console", 18, FW_NORMAL, FALSE, TRUE, RGB(0, 100, 50)},
    {"Segoe UI", 22, FW_BOLD, TRUE, FALSE, RGB(200, 50, 50)},
    {"Calibri", 24, FW_NORMAL, TRUE, TRUE, RGB(80, 0, 120)},
    {"Cambria", 22, FW_BOLD, FALSE, FALSE, RGB(0, 0, 140)},
    {"Arial Black", 18, FW_NORMAL, FALSE, FALSE, RGB(200, 80, 160)},
    {"Segoe Script", 20, FW_BOLD, FALSE, FALSE, RGB(150, 100, 0)},
    {"Terminal", 16, FW_NORMAL, FALSE, FALSE, RGB(0, 70, 70)},
    {"Fixedsys", 16, FW_NORMAL, FALSE, FALSE, RGB(100, 0, 200)},
    {"System", 16, FW_BOLD, FALSE, FALSE, RGB(20, 100, 200)},
};
const char *TEXTS[] = {
    "Arial",       "Times New Roman", "Courier New",   "Georgia",      "Verdana",
    "Tahoma",      "Impact",          "Comic Sans MS", "Trebuchet MS", "Palatino Linotype",
    "Consolas",    "Lucida Console",  "Segoe UI",      "Calibri",      "Cambria",
    "Arial Black", "Segoe Script",    "Terminal",      "Fixedsys",     "System",
};
HFONT g_hFonts[ROWS * COLS];

void BuildFonts(int nScale) {
    for (int i = 0; i < ROWS * COLS; ++i) {
        if (g_hFonts[i])
            DeleteObject(g_hFonts[i]);
        g_hFonts[i] = CreateFontA(SPECS[i].height * nScale / 100, 0, 0, 0, SPECS[i].weight,
                                  SPECS[i].italic, SPECS[i].underline, FALSE, DEFAULT_CHARSET,
                                  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                  DEFAULT_PITCH | FF_DONTCARE, SPECS[i].face);
    }
}

int ColLeft(int c, int w) { return c * w / COLS; }

int WrapText(HDC hdc, const char *text, int x, int y, int width, bool draw) {
    TEXTMETRICA tm;
    GetTextMetricsA(hdc, &tm);
    int len = lstrlenA(text);
    int top = y;
    while (len > 0) {
        int fit = 0;
        SIZE sz;
        GetTextExtentExPointA(hdc, text, len, width, &fit, nullptr, &sz);
        if (fit < 1)
            fit = 1;
        int lineLen = fit, skip = fit;
        if (fit < len) {
            int p = fit;
            while (p > 0 && text[p] != ' ')
                --p;
            if (p > 0) {
                lineLen = p;
                skip = p + 1;
            }
        }
        if (draw)
            TextOutA(hdc, x, y, text, lineLen);
        y += tm.tmHeight;
        text += skip;
        len -= skip;
    }
    return y - top;
}

int RowHeight(HDC hdc, int r, int w) {
    int rowH = 0;
    for (int c = 0; c < COLS; ++c) {
        int idx = r * COLS + c;
        int textW = ColLeft(c + 1, w) - ColLeft(c, w) - 2 * PAD;
        HGDIOBJ old = SelectObject(hdc, g_hFonts[idx]);
        int h = WrapText(hdc, TEXTS[idx], 0, 0, textW, false) + 2 * PAD;
        SelectObject(hdc, old);
        if (h > rowH)
            rowH = h;
    }
    return rowH;
}

int TableHeight(HDC hdc, int w) {
    int total = 0;
    for (int r = 0; r < ROWS; ++r)
        total += RowHeight(hdc, r, w);
    return total;
}

void FitFonts(HWND hwnd) {
    RECT client;
    GetClientRect(hwnd, &client);
    HDC hdc = GetDC(hwnd);
    int lo = MIN_SCALE, hi = MAX_SCALE;
    while (lo < hi) {
        int mid = (lo + hi + 1) / 2;
        BuildFonts(mid);
        if (TableHeight(hdc, client.right) <= client.bottom)
            lo = mid;
        else
            hi = mid - 1;
    }
    BuildFonts(lo);
    ReleaseDC(hwnd, hdc);
}

void Paint(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    RECT client;
    GetClientRect(hwnd, &client);
    SetBkMode(hdc, TRANSPARENT);
    int y = 0;
    for (int r = 0; r < ROWS; ++r) {
        int rowH = RowHeight(hdc, r, client.right);
        for (int c = 0; c < COLS; ++c) {
            int idx = r * COLS + c;
            RECT cell = {ColLeft(c, client.right), y, ColLeft(c + 1, client.right), y + rowH};
            Rectangle(hdc, cell.left, cell.top, cell.right, cell.bottom);
            HGDIOBJ old = SelectObject(hdc, g_hFonts[idx]);
            SetTextColor(hdc, SPECS[idx].color);
            WrapText(hdc, TEXTS[idx], cell.left + PAD, cell.top + PAD,
                     cell.right - cell.left - 2 * PAD, true);
            SelectObject(hdc, old);
        }
        y += rowH;
    }
    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_SIZE:
        if (wp != SIZE_MINIMIZED) {
            FitFonts(hwnd);
            InvalidateRect(hwnd, nullptr, TRUE);
        }
        return 0;
    case WM_PAINT:
        Paint(hwnd);
        return 0;
    case WM_DESTROY:
        for (int i = 0; i < ROWS * COLS; ++i)
            DeleteObject(g_hFonts[i]);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nShow) {
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "TableWindow";
    RegisterClassA(&wc);
    HWND hwnd = CreateWindowA("TableWindow", "Table", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                              CW_USEDEFAULT, 1000, 600, nullptr, nullptr, hInst, nullptr);
    ShowWindow(hwnd, nShow);
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
        DispatchMessage(&msg);
    return 0;
}
