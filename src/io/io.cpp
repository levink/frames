#include <iostream>
#include "io.h"

using namespace io;

bool UIState::is(KeyMod mod) {
    if (mod == KeyMod::NO) {
        return
            !keyPressed[0] &&
            !keyPressed[1] &&
            !keyPressed[2];
    }
    if (mod == KeyMod::ANY) {
        return
            keyPressed[0] ||
            keyPressed[1] ||
            keyPressed[2];
    }
    if (mod == KeyMod::SHIFT) return keyPressed[0];
    if (mod == KeyMod::CTRL)  return keyPressed[1];
    if (mod == KeyMod::ALT)   return keyPressed[2];

    return false;
}
void UIState::print() {
    std::wcout
        << keyPressed[0] << " "
        << keyPressed[1] << " "
        << keyPressed[2] << " "
        << "x=" << x << " "
        << "y=" << y << " "
        << "mousePressed=" << (int)this->mousePressed
        << std::endl;
}
glm::ivec2 UIState::getCursor() {
    return { x, y };
}

namespace io {
    namespace keyboard {
        KeyEvent::KeyEvent(int key, int action, int mod) {
            this->key = (keyboard::Key)key;
            this->action = (keyboard::Action)action;
            //this->mod = mod;

            if (action == keyboard::Action::PRESS) {
                if (key == GLFW_KEY_LEFT_SHIFT)   io_state.keyPressed[0] = true;
                if (key == GLFW_KEY_LEFT_CONTROL) io_state.keyPressed[1] = true;
                if (key == GLFW_KEY_LEFT_ALT)     io_state.keyPressed[2] = true;
            }
            else if (action == keyboard::Action::REPEAT) {
                if (mod == GLFW_MOD_SHIFT)   io_state.keyPressed[0] = true;
                if (mod == GLFW_MOD_CONTROL) io_state.keyPressed[1] = true;
                if (mod == GLFW_MOD_ALT)     io_state.keyPressed[2] = true;
            }
            else if (action == keyboard::Action::RELEASE) {
                if (key == GLFW_KEY_LEFT_SHIFT)   io_state.keyPressed[0] = false;
                if (key == GLFW_KEY_LEFT_CONTROL) io_state.keyPressed[1] = false;
                if (key == GLFW_KEY_LEFT_ALT)     io_state.keyPressed[2] = false;
            }
        }
        bool KeyEvent::is(Key key) {
            return is(KeyMod::NO, key);
        }
        bool KeyEvent::is(KeyMod mod, Key key) {
            bool result =
                this->key == key && (
                    this->action == keyboard::Action::PRESS ||
                    this->action == keyboard::Action::REPEAT
                ) && io_state.is(mod);
            return result;
        }
        bool KeyEvent::is(KeyMod mod, keyboard::Action action) {
            using namespace keyboard;
            bool pressed = io_state.is(mod);
            if (action == Action::PRESS) {
                return pressed;
            }
            if (action == Action::RELEASE) {
                return !pressed;
            }
            return false;
        }
    }

    namespace mouse {
        MouseEvent move(int x, int y) {
            auto dx = x - io_state.x;
            auto dy = y - io_state.y;
            io_state.x = x;
            io_state.y = y;
            return MouseEvent{ io_state.mousePressed, Action::MOVE, dx, dy };
        }
        MouseEvent click(int button, int action, int mod) {

            auto mouseButton = Button::NO;
            if (button == GLFW_MOUSE_BUTTON_LEFT)        mouseButton = Button::LEFT;
            else if (button == GLFW_MOUSE_BUTTON_MIDDLE) mouseButton = Button::MIDDLE;
            else if (button == GLFW_MOUSE_BUTTON_RIGHT)  mouseButton = Button::RIGHT;
            else                                         mouseButton = Button::NO;

            auto mouseAction = Action::NONE;
            if (action == GLFW_PRESS)        mouseAction = Action::PRESS;
            else if (action == GLFW_RELEASE) mouseAction = Action::RELEASE;
            else                             mouseAction = Action::NONE;

            auto pressed = (action == GLFW_PRESS || action == GLFW_REPEAT);
            io_state.mousePressed = pressed ? mouseButton : Button::NO;

            return MouseEvent{ mouseButton, mouseAction, 0, 0 };
        }
        bool MouseEvent::is(Action action) const {

            if (action == Action::MOVE) {
                return is(Action::MOVE, Button::NO, KeyMod::NO);
            }

            throw std::runtime_error("Not implemented");
        }
        bool MouseEvent::is(Action action, Button button) const {
            return is(action, button, KeyMod::NO);
        }
        bool MouseEvent::is(Action action, Button button, KeyMod mod) const {
            if (action != this->action) {
                return false;
            }

            bool buttonCheck =
                (button == this->button) ||
                (button == Button::ANY && this->button != Button::NO);
            if (!buttonCheck) {
                return false;
            }

            bool modCheck = io_state.is(mod);
            return modCheck;
        }
        glm::ivec2 MouseEvent::getDelta() const {
            return { dx, dy };
        }
        glm::ivec2 MouseEvent::getCursor() {
            return io_state.getCursor();
        }
    }
}
