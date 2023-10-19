#include <ftxui/component/event.hpp>
#include <string>

#ifndef CONTROLLER_H
#define CONTROLLER_H

class InputHandler {
public:
   
    InputHandler() { };

    ~InputHandler() { };

    enum class Command {
        NOP,
        UP,
        DOWN,
        LEFT,
        RIGHT,
        SELECTION_MODE,
        EDIT_MODE,
        PATTERN_MODE,
        ESCAPE,
        INPUT,
        NEW_PATTERN,
        DELETE_PATTERN,
        PASS,
        ENTER,
        YANK,
        PASTE
    };
    
    InputHandler::Command ViewModeInputHandler(const ftxui::Event& event);
    
    InputHandler::Command EditModeInputHandler(const ftxui::Event& event);
    
    InputHandler::Command PatternModeInputHandler(const ftxui::Event& event);

private:
    
    std::string _command = "";
    
};

#endif // CONTROLLER_H
