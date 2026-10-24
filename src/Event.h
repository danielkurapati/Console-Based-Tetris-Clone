#pragma once

#include <queue>

enum class EventType {
    MoveLeft,
    MoveRight,
    RotateCW,
    SoftDrop,
    HardDrop,
    Pause,
    Confirm,   // Enter
    Quit,
    Tick       // gravity timer fired
};

struct Event {
    EventType type;
};

class EventQueue {
public:
    void push(Event e) { q_.push(e); }

    // Pops the oldest event into 'out'. Returns false if there is none.
    bool poll(Event& out) {
        if (q_.empty()) return false;
        out = q_.front();
        q_.pop();
        return true;
    }

    bool empty() const { return q_.empty(); }

private:
    std::queue<Event> q_;
};
