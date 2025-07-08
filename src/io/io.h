#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace io::keyboard {
    enum Key {
        ESC = GLFW_KEY_ESCAPE,
        SPACE = GLFW_KEY_SPACE,
        RIGHT = GLFW_KEY_RIGHT,
        LEFT = GLFW_KEY_LEFT,
        UP = GLFW_KEY_UP,
        DOWN = GLFW_KEY_DOWN,
        LEFT_SHIFT = GLFW_KEY_LEFT_SHIFT,
        LEFT_CONTROL = GLFW_KEY_LEFT_CONTROL,
        LEFT_ALT = GLFW_KEY_LEFT_ALT,
        COMMA = GLFW_KEY_COMMA,
        PERIOD = GLFW_KEY_PERIOD,

        W = GLFW_KEY_W,
        S = GLFW_KEY_S,
        A = GLFW_KEY_A,
        D = GLFW_KEY_D,
        C = GLFW_KEY_C,
        Z = GLFW_KEY_Z,
        X = GLFW_KEY_X,
        P = GLFW_KEY_P,
        Q = GLFW_KEY_Q,
        E = GLFW_KEY_E,
        R = GLFW_KEY_R,
        T = GLFW_KEY_T,
        M = GLFW_KEY_M,
        N = GLFW_KEY_N,
        H = GLFW_KEY_H,
        B = GLFW_KEY_B,
        O = GLFW_KEY_O,
        K = GLFW_KEY_K,

        KEY_1 = GLFW_KEY_1,
        KEY_2 = GLFW_KEY_2,
        KEY_3 = GLFW_KEY_3,
        KEY_4 = GLFW_KEY_4,
        KEY_5 = GLFW_KEY_5,
        KEY_6 = GLFW_KEY_6,
        KEY_7 = GLFW_KEY_7,
        KEY_0 = GLFW_KEY_0,
        
        

        KP_ADD = GLFW_KEY_KP_ADD,
        KP_SUB = GLFW_KEY_KP_SUBTRACT,
        MINUS = GLFW_KEY_MINUS,
        EQUAL = GLFW_KEY_EQUAL,
        TAB = GLFW_KEY_TAB,
        DEL = GLFW_KEY_DELETE
    };
    namespace Action {
        static int RELEASE      = GLFW_RELEASE;
        static int PRESS        = GLFW_PRESS;
        static int REPEAT       = GLFW_REPEAT;
    }
    namespace Mod {
        static int NO           = 0;
        static int SHIFT        = GLFW_MOD_SHIFT;
        static int CONTROL      = GLFW_MOD_CONTROL;
        static int ALT          = GLFW_MOD_ALT;
        static int SUPER        = GLFW_MOD_SUPER;
        static int CAPS_LOCK    = GLFW_MOD_CAPS_LOCK;
        static int NUM_LOCK     = GLFW_MOD_NUM_LOCK;
        static int ANY          = GLFW_MOD_SHIFT | GLFW_MOD_CONTROL | GLFW_MOD_ALT;
    }
    class KeyEvent {
        int key;
        int action;
        int mod;
    public:
        KeyEvent();
        KeyEvent(int key, int action, int mod);
        bool pressed(Key key) const;
        bool is(Key key) const;
        bool is(int mod, Key key) const;
        bool is(int modPrev, Key keyPrev, Key key) const;
    };

    const KeyEvent& create(int key, int action, int mod);
}
