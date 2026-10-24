#pragma once
// InputHandler: translates raw keys into game Events.

#include "Event.h"
#include "Terminal.h"

class InputHandler {
public:
    void poll(Terminal& term, EventQueue& q) {
        int k;
        while ((k = term.readKey()) != KEY_NONE) {
            switch (k) {
                case KEY_LEFT:  case 'a': case 'A': q.push(Event{EventType::MoveLeft});  break;
                case KEY_RIGHT: case 'd': case 'D': q.push(Event{EventType::MoveRight}); break;
                case KEY_UP:    case 'w': case 'W': q.push(Event{EventType::RotateCW});  break;
                case KEY_DOWN:  case 's': case 'S': q.push(Event{EventType::SoftDrop});  break;
                case ' ':                           q.push(Event{EventType::HardDrop});  break;
                case 'p': case 'P':                 q.push(Event{EventType::Pause});     break;
                case KEY_ENTER: case '\n':          q.push(Event{EventType::Confirm});   break;
                case 'q': case 'Q': case KEY_ESC: case KEY_CTRL_C: case KEY_EOF:
                                                    q.push(Event{EventType::Quit});      break;
                default: break;
            }
        }
    }
};
