#include "Controller.hpp"
#include <cassert>

Controller::Command Controller::InputHandler(const ftxui::Event& event, const int modalDepth) {

    if (event.is_character()) {
        return EventDispatcher(event, modalDepth);
    } else {
        return FunctionHandler(event);
    }
    
}

Controller::Command Controller::InputHandlerView(const ftxui::Event& event) {
    
    Command command = Command::NOP;
    char input = event.character().at(0);
    
    switch (input) {
        case 'j':
            command = Command::DOWN;
            break;
        case 'k':            
            command = Command::UP;
            break;
        case 'l':            
            command = Command::RIGHT;
            break;
        case 'h':            
            command = Command::LEFT;
            break;
        case 'v':            
            command = Command::SELECTION_MODE;            
            break;
        case 'r':            
            command = Command::EDIT_MODE;            
            break;
        case 'f':            
            command = Command::PATTERN_MODE;            
            break;
        default:
            break;
    }

    return command;
}

Controller::Command Controller::InputHandlerPattern(const ftxui::Event& event) {
    
    Command command = Command::INPUT;
    char input = event.character().at(0);
    
    switch (input) {
        case 'n':
            command = Command::NEW_PATTERN;
            break;
        case 'N':            
            command = Command::DELETE_PATTERN;
            break;
        default:
            break;
    }

    return command;
}

Controller::Command Controller::EventDispatcher(const ftxui::Event& event, const int modalDepth) {

    switch (modalDepth) {
        case 0:
            return InputHandlerView(event);
            break;
        case 1:
            return Command::INPUT;
            break;
        case 2:
            break;
        case 3:
            return InputHandlerPattern(event);
            break;
        case 4:
            break;
        default:
            assert("Not Implemented");
            break;
    }

    return Command::NOP;
  
}

Controller::Command Controller::FunctionHandler(const ftxui::Event& event) {
    Command command = Command::INPUT;

    if (event == ftxui::Event::Escape) {
        command = Command::ESCAPE;
    }

    return command;
}
