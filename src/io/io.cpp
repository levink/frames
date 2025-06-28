#include <iostream>
#include "io.h"

namespace io::keyboard {

    KeyEvent lastPressed[2];
    KeyEvent current;

    const KeyEvent& create(int key, int action, int mod) {
        current = KeyEvent(key, action, mod);
        if (action == Action::PRESS) {
            lastPressed[0] = lastPressed[1];
            lastPressed[1] = current;
        }
        return current;
    }
    static const KeyEvent& prevPressed() {
        return lastPressed[0];
    }
    static void clearPressHistory() {
        lastPressed[0] = KeyEvent();
        lastPressed[1] = KeyEvent();
    }


    KeyEvent::KeyEvent() : key(0), action(0), mod(0) { }
    KeyEvent::KeyEvent(int key, int action, int mod) : key(key), action(action), mod(mod) { }
    bool KeyEvent::is(Key key) const {
        if (this->key != key) {
            return false;
        }
        if (action == Action::RELEASE) {
            return false;
        }
        return (mod == Mod::NO);
    }
    bool KeyEvent::is(int mod, Key key) const {
        if (this->key != key) {
            return false;
        }
        if (action == Action::RELEASE) {
            return false;
        }
        return (this->mod & mod);
    }
    bool KeyEvent::is(int modPrev, Key keyPrev, Key key) const {
        
        if (!prevPressed().is(modPrev, keyPrev)) {
            return false;
        }

        if (this->key != key) {
            return false;
        }
        
        if (action == Action::RELEASE) {
            return false;
        }

        clearPressHistory();
        return true;        
    }
}
