#pragma once
#include <glm/glm.hpp>

struct DragDrop {

    bool active = false;
    glm::vec2 start;
    glm::vec2 end;
    glm::vec2 offset;

    void dragStart(const glm::vec2& cursor, const glm::vec2& position);
    void dragMove(const glm::vec2& cursor);
    void dragStop(const glm::vec2& cursor);
    void dragCancel();
    bool changed() const;
};