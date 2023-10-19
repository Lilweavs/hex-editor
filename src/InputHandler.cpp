#include "InputHandler.hpp"
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>

InputHandler::Command InputHandler::ViewModeInputHandler(const ftxui::Event& event) {
    
    Command command = Command::NOP;
    char input = event.character().at(0);

    if (event.is_character()) {
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
    } else {
        if (event == ftxui::Event::Escape) {
            command = Command::ESCAPE;
        } else {
            command = Command::NOP;
        }
    }
    

    return command;
}

InputHandler::Command InputHandler::EditModeInputHandler(const ftxui::Event& event) {
    Command command = Command::NOP;

    if (event == ftxui::Event::Return) {
        command = Command::ENTER;   
    } else if (event == ftxui::Event::Escape) {
        command = Command::ESCAPE;
    } else {
        command = Command::PASS;
    }

    return command;
}

InputHandler::Command InputHandler::PatternModeInputHandler(const ftxui::Event& event) {
    Command command = Command::NOP;

    if (event.is_character()) {
        char c = event.character().at(0);
        switch (c) {
            case 'n':
                command = Command::NEW_PATTERN;
                break;
            case 'N':
                command = Command::DELETE_PATTERN;
                break;
            default:
                command = Command::PASS;
                break;
        }
    } else {
        if (event == ftxui::Event::Return) {
            command = Command::ENTER;   
        } else if (event == ftxui::Event::Escape){
            command = Command::ESCAPE;
        } else {
            command = Command::PASS;
        }
    }

    return command;
}
