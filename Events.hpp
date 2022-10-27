/**
 * @ Author: Matthieu Moinvaziri
 * @ Description: UI Event structures
 */

#pragma once

#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_mouse.h>

#include "Base.hpp"

namespace kF::UI
{
    struct MouseEvent;
    struct MotionEvent;
    struct WheelEvent;
    struct DropEvent;
    struct KeyEvent;

    /** @brief Mouse cursor */
    enum class Cursor : std::uint32_t
    {
        Arrow = SDL_SYSTEM_CURSOR_ARROW,
        Ibeam = SDL_SYSTEM_CURSOR_IBEAM,
        Wait = SDL_SYSTEM_CURSOR_WAIT,
        Crosshair = SDL_SYSTEM_CURSOR_CROSSHAIR,
        WaitArrow = SDL_SYSTEM_CURSOR_WAITARROW,
        SizeNWSE = SDL_SYSTEM_CURSOR_SIZENWSE,
        SizeNESW = SDL_SYSTEM_CURSOR_SIZENESW,
        SizeWE = SDL_SYSTEM_CURSOR_SIZEWE,
        SizeNS = SDL_SYSTEM_CURSOR_SIZENS,
        SizeAll = SDL_SYSTEM_CURSOR_SIZEALL,
        No = SDL_SYSTEM_CURSOR_NO,
        Hand = SDL_SYSTEM_CURSOR_HAND
    };

    /** @brief Number of cursors */
    constexpr auto CursorCount = SDL_NUM_SYSTEM_CURSORS;

    /** @brief Mouse button */
    enum class Button : std::uint8_t
    {
        None    = 0b000,
        Left    = 0b001,
        Middle  = 0b010,
        Right   = 0b100
    };

    /** @brief Key modifiers */
    enum class Modifier : std::uint16_t
    {
        None = 0b0,             // No modifier key is pressed.
        LShift = KMOD_LSHIFT,   // Left Shift key on the keyboard is pressed
        RShift = KMOD_RSHIFT,   // Right Shift key on the keyboard is pressed
        Shift = KMOD_SHIFT,     // Any Shift key on the keyboard is pressed
        LCtrl = KMOD_LCTRL,     // Left Ctrl key on the keyboard is pressed
        RCtrl = KMOD_RCTRL,     // Right Ctrl key on the keyboard is pressed
        Ctrl = KMOD_CTRL,       // Any Ctrl key on the keyboard is pressed
        LAlt = KMOD_LALT,       // Left Alt key on the keyboard is pressed
        RAlt = KMOD_RALT,       // Right Alt key on the keyboard is pressed
        Alt = KMOD_ALT,         // Any Alt key on the keyboard is pressed
        LSuper = KMOD_LGUI,     // Left Super key on the keyboard is pressed
        RSuper = KMOD_RGUI,     // Right Super key on the keyboard is pressed
        Super = KMOD_GUI,       // Any Super key on the keyboard is pressed
        Num = KMOD_NUM,         // Numpad lock is active
        Caps = KMOD_CAPS,       // Caps lock is active
        Mode = KMOD_MODE        // Mode key is pressed (AltGr on windows)
    };

    /** @brief Keys */
    enum class Key : std::uint32_t
    {
        Unknown = SDLK_UNKNOWN,
        Return = SDLK_RETURN,
        Escape = SDLK_ESCAPE,
        Backspace = SDLK_BACKSPACE,
        Tab = SDLK_TAB,
        Space = SDLK_SPACE,
        Exclaim = SDLK_EXCLAIM,
        DoubleQuote = SDLK_QUOTEDBL,
        Hash = SDLK_HASH,
        Percent = SDLK_PERCENT,
        Dollar = SDLK_DOLLAR,
        Ampersand = SDLK_AMPERSAND,
        Quote = SDLK_QUOTE,
        LeftParenthesis = SDLK_LEFTPAREN,
        RightParenthesis = SDLK_RIGHTPAREN,
        Asterisk = SDLK_ASTERISK,
        Plus = SDLK_PLUS,
        Comma = SDLK_COMMA,
        Minus = SDLK_MINUS,
        Period = SDLK_PERIOD,
        Slash = SDLK_SLASH,
        Key0 = SDLK_0,
        Key1 = SDLK_1,
        Key2 = SDLK_2,
        Key3 = SDLK_3,
        Key4 = SDLK_4,
        Key5 = SDLK_5,
        Key6 = SDLK_6,
        Key7 = SDLK_7,
        Key8 = SDLK_8,
        Key9 = SDLK_9,
        Colon = SDLK_COLON,
        Semicolon = SDLK_SEMICOLON,
        Less = SDLK_LESS,
        Equals = SDLK_EQUALS,
        Greater = SDLK_GREATER,
        Question = SDLK_QUESTION,
        At = SDLK_AT,
        LeftBracket = SDLK_LEFTBRACKET,
        Backslash = SDLK_BACKSLASH,
        RightBracket = SDLK_RIGHTBRACKET,
        Caret = SDLK_CARET,
        Underscore = SDLK_UNDERSCORE,
        BackQuote = SDLK_BACKQUOTE,
        A = SDLK_a,
        B = SDLK_b,
        C = SDLK_c,
        D = SDLK_d,
        E = SDLK_e,
        F = SDLK_f,
        G = SDLK_g,
        H = SDLK_h,
        I = SDLK_i,
        J = SDLK_j,
        K = SDLK_k,
        L = SDLK_l,
        M = SDLK_m,
        N = SDLK_n,
        O = SDLK_o,
        P = SDLK_p,
        Q = SDLK_q,
        R = SDLK_r,
        S = SDLK_s,
        T = SDLK_t,
        U = SDLK_u,
        V = SDLK_v,
        W = SDLK_w,
        X = SDLK_x,
        Y = SDLK_y,
        Z = SDLK_z,
        Capslock = SDLK_CAPSLOCK,
        F1 = SDLK_F1,
        F2 = SDLK_F2,
        F3 = SDLK_F3,
        F4 = SDLK_F4,
        F5 = SDLK_F5,
        F6 = SDLK_F6,
        F7 = SDLK_F7,
        F8 = SDLK_F8,
        F9 = SDLK_F9,
        F10 = SDLK_F10,
        F11 = SDLK_F11,
        F12 = SDLK_F12,
        PrintScreen = SDLK_PRINTSCREEN,
        ScrollLock = SDLK_SCROLLLOCK,
        Pause = SDLK_PAUSE,
        Insert = SDLK_INSERT,
        Home = SDLK_HOME,
        Pageup = SDLK_PAGEUP,
        Delete = SDLK_DELETE,
        End = SDLK_END,
        Pagedown = SDLK_PAGEDOWN,
        Right = SDLK_RIGHT,
        Left = SDLK_LEFT,
        Down = SDLK_DOWN,
        Up = SDLK_UP,
        NumlockClear = SDLK_NUMLOCKCLEAR,
        KeypadDivide = SDLK_KP_DIVIDE,
        KeypadMultiply = SDLK_KP_MULTIPLY,
        KeypadMinus = SDLK_KP_MINUS,
        KeypadPlus = SDLK_KP_PLUS,
        KeypadEnter = SDLK_KP_ENTER,
        Keypad1 = SDLK_KP_1,
        Keypad2 = SDLK_KP_2,
        Keypad3 = SDLK_KP_3,
        Keypad4 = SDLK_KP_4,
        Keypad5 = SDLK_KP_5,
        Keypad6 = SDLK_KP_6,
        Keypad7 = SDLK_KP_7,
        Keypad8 = SDLK_KP_8,
        Keypad9 = SDLK_KP_9,
        Keypad0 = SDLK_KP_0,
        KeypadPeriod = SDLK_KP_PERIOD,
        Application = SDLK_APPLICATION,
        Power = SDLK_POWER,
        KeypadEquals = SDLK_KP_EQUALS,
        F13 = SDLK_F13,
        F14 = SDLK_F14,
        F15 = SDLK_F15,
        F16 = SDLK_F16,
        F17 = SDLK_F17,
        F18 = SDLK_F18,
        F19 = SDLK_F19,
        F20 = SDLK_F20,
        F21 = SDLK_F21,
        F22 = SDLK_F22,
        F23 = SDLK_F23,
        F24 = SDLK_F24,
        Execute = SDLK_EXECUTE,
        Help = SDLK_HELP,
        Menu = SDLK_MENU,
        Select = SDLK_SELECT,
        Stop = SDLK_STOP,
        Again = SDLK_AGAIN,
        Undo = SDLK_UNDO,
        Cut = SDLK_CUT,
        Copy = SDLK_COPY,
        Paste = SDLK_PASTE,
        Find = SDLK_FIND,
        Mute = SDLK_MUTE,
        VolumeUp = SDLK_VOLUMEUP,
        VolumeDown = SDLK_VOLUMEDOWN,
        KeypadComma = SDLK_KP_COMMA,
        KeypadEqualsAs400 = SDLK_KP_EQUALSAS400,
        Alterase = SDLK_ALTERASE,
        SysReq = SDLK_SYSREQ,
        Cancel = SDLK_CANCEL,
        Clear = SDLK_CLEAR,
        Prior = SDLK_PRIOR,
        Return2 = SDLK_RETURN2,
        Separator = SDLK_SEPARATOR,
        Out = SDLK_OUT,
        Oper = SDLK_OPER,
        ClearAgain = SDLK_CLEARAGAIN,
        CrSel = SDLK_CRSEL,
        ExSel = SDLK_EXSEL,
        Keypad00 = SDLK_KP_00,
        Keypad000 = SDLK_KP_000,
        ThousandsSeparator = SDLK_THOUSANDSSEPARATOR,
        DecimalSeparator = SDLK_DECIMALSEPARATOR,
        CurrencyUnit = SDLK_CURRENCYUNIT,
        CurrencySubunit = SDLK_CURRENCYSUBUNIT,
        KeypadLeftParenthesis = SDLK_KP_LEFTPAREN,
        KeypadRightParenthesis = SDLK_KP_RIGHTPAREN,
        KeypadLeftBrace = SDLK_KP_LEFTBRACE,
        KeypadRightBrace = SDLK_KP_RIGHTBRACE,
        KeypadTab = SDLK_KP_TAB,
        KeypadBackspace = SDLK_KP_BACKSPACE,
        KeypadA = SDLK_KP_A,
        KeypadB = SDLK_KP_B,
        KeypadC = SDLK_KP_C,
        KeypadD = SDLK_KP_D,
        KeypadE = SDLK_KP_E,
        KeypadF = SDLK_KP_F,
        KeypadXor = SDLK_KP_XOR,
        KeypadPower = SDLK_KP_POWER,
        KeypadPercent = SDLK_KP_PERCENT,
        KeypadLess = SDLK_KP_LESS,
        KeypadGreater = SDLK_KP_GREATER,
        KeypadAmpersand = SDLK_KP_AMPERSAND,
        KeypadDoubleAmpersand = SDLK_KP_DBLAMPERSAND,
        KeypadVerticalBar = SDLK_KP_VERTICALBAR,
        KeypadDblverticalBar = SDLK_KP_DBLVERTICALBAR,
        KeypadColon = SDLK_KP_COLON,
        KeypadHash = SDLK_KP_HASH,
        KeypadSpace = SDLK_KP_SPACE,
        KeypadAt = SDLK_KP_AT,
        KeypadExclam = SDLK_KP_EXCLAM,
        KeypadMemStore = SDLK_KP_MEMSTORE,
        KeypadMemRecall = SDLK_KP_MEMRECALL,
        KeypadMemClear = SDLK_KP_MEMCLEAR,
        KeypadMemAdd = SDLK_KP_MEMADD,
        KeypadMemSubtract = SDLK_KP_MEMSUBTRACT,
        KeypadMemMultiply = SDLK_KP_MEMMULTIPLY,
        KeypadMemDivide = SDLK_KP_MEMDIVIDE,
        KeypadPlusMinus = SDLK_KP_PLUSMINUS,
        KeypadClear = SDLK_KP_CLEAR,
        KeypadClearEntry = SDLK_KP_CLEARENTRY,
        KeypadBinary = SDLK_KP_BINARY,
        KeypadOctal = SDLK_KP_OCTAL,
        KeypadDecimal = SDLK_KP_DECIMAL,
        KeypadHexadecimal = SDLK_KP_HEXADECIMAL,
        LCtrl = SDLK_LCTRL,
        LShift = SDLK_LSHIFT,
        LAlt = SDLK_LALT,
        LGui = SDLK_LGUI,
        RCtrl = SDLK_RCTRL,
        RShift = SDLK_RSHIFT,
        RAlt = SDLK_RALT,
        RGui = SDLK_RGUI,
        Mode = SDLK_MODE,
        AudioNext = SDLK_AUDIONEXT,
        AudioPrev = SDLK_AUDIOPREV,
        AudioStop = SDLK_AUDIOSTOP,
        AudioPlay = SDLK_AUDIOPLAY,
        AudioMute = SDLK_AUDIOMUTE,
        MediaSelect = SDLK_MEDIASELECT,
        Www = SDLK_WWW,
        Mail = SDLK_MAIL,
        Calculator = SDLK_CALCULATOR,
        Computer = SDLK_COMPUTER,
        ActionSearch = SDLK_AC_SEARCH,
        ActionHome = SDLK_AC_HOME,
        ActionBack = SDLK_AC_BACK,
        ActionForward = SDLK_AC_FORWARD,
        ActionStop = SDLK_AC_STOP,
        ActionRefresh = SDLK_AC_REFRESH,
        ActionBookmarks = SDLK_AC_BOOKMARKS,
        BrightnessDown = SDLK_BRIGHTNESSDOWN,
        BrightnessUp = SDLK_BRIGHTNESSUP,
        DisplaySwitch = SDLK_DISPLAYSWITCH,
        KeyboardDillumToggle = SDLK_KBDILLUMTOGGLE,
        KeyboardDillumDown = SDLK_KBDILLUMDOWN,
        KeyboardDillumUp = SDLK_KBDILLUMUP,
        Eject = SDLK_EJECT,
        Sleep = SDLK_SLEEP,
        App1 = SDLK_APP1,
        App2 = SDLK_APP2,
        AudioRewind = SDLK_AUDIOREWIND,
        AudioFastForward = SDLK_AUDIOFASTFORWARD
    };
}

/** @brief Describe a mouse event (single button action) */
struct alignas_quarter_cacheline kF::UI::MouseEvent
{
    Point pos {};
    Button button {};
    bool state {};
    Modifier modifiers {};
    std::uint32_t timestamp {};
};
static_assert_fit_quarter_cacheline(kF::UI::MouseEvent);

/** @brief Describe a motion event (mouse movement) */
struct alignas_half_cacheline kF::UI::MotionEvent
{
    /** @brief Motion event type */
    enum class Type : std::uint8_t
    {
        None,
        Enter,
        Leave
    };

    Point pos {};
    Point motion {};
    Type type {};
    Button buttons {};
    Modifier modifiers {};
    std::uint32_t timestamp {};
};
static_assert_fit_half_cacheline(kF::UI::MotionEvent);

/** @brief Describe a wheel event */
struct alignas_half_cacheline kF::UI::WheelEvent
{
    Point pos {};
    Point offset {};
    Modifier modifiers {};
    std::uint32_t timestamp {};
};
static_assert_fit_half_cacheline(kF::UI::WheelEvent);

/** @brief Describe a drop event */
struct alignas_half_cacheline kF::UI::DropEvent
{
    /** @brief Type of drop event */
    enum class Type
    {
        Begin,
        End,
        Enter,
        Motion,
        Leave,
        Drop
    };

    Type type {};
    Point pos {};
    std::uint32_t timestamp {};
};
static_assert_fit_half_cacheline(kF::UI::DropEvent);

/** @brief Describe a key event (single key action) */
struct alignas_quarter_cacheline kF::UI::KeyEvent
{
    Key key {};
    Modifier modifiers {};
    bool state {};
    bool repeat {};
    std::uint32_t timestamp {};
};
static_assert_fit_quarter_cacheline(kF::UI::KeyEvent);