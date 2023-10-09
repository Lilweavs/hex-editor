#include <ftxui/component/event.hpp>
#include <string>

#ifndef CONTROLLER_H
#define CONTROLLER_H

class Controller {
public:
   
    Controller() { };

    ~Controller() { };

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
        DELETE_PATTERN
    };
    
    Controller::Command InputHandler(const ftxui::Event& event, const int modalDepth);

private:
    
    Controller::Command EventDispatcher(const ftxui::Event& event, const int modalDepth);
    
    Controller::Command InputHandlerView(const ftxui::Event& event);
    
    Controller::Command InputHandlerEdit(const ftxui::Event& event);

    Controller::Command InputHandlerPattern(const ftxui::Event& event);
    
    Controller::Command FunctionHandler(const ftxui::Event& event);
    
    std::string _command = "";
    
};


#endif // CONTROLLER_H
