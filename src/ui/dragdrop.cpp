#include "dragdrop.h"
void DragDrop::dragStart(const glm::vec2& cursor, const glm::vec2& position) {
    if (active) {
        return;
    }
    active = true;
    start = position;
    end = position;
    offset = cursor - start;
}
void DragDrop::dragMove(const glm::vec2& cursor) {
    if (active) {
        end = cursor - offset;
    }
}
void DragDrop::dragStop(const glm::vec2& cursor) {
    if (active) {
        active = false;
        end = cursor - offset;
    }
}
void DragDrop::dragCancel() {
    active = false;
    end = start;
}
bool DragDrop::changed() const {
    return
        start.x != end.x ||
        start.y != end.y;
}
