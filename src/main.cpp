#include <algorithm>
#include <cstdint>
#include <ftxui/component/component.hpp> // for Input, Renderer, ResizableSplitLeft
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/screen_interactive.hpp> // for ScreenInteractive
#include <ftxui/dom/elements.hpp> // for operator|, separator, text, Element, flex, vbox, border
#include <ftxui/component/event.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/terminal.hpp>
#include <ftxui/util/ref.hpp>
#include <memory> // for allocator, __shared_ptr_access, shared_ptr
#include <string> // for string
#include <thread>
#include <filesystem>
#include <cassert>
#include <format>
#include <regex>
#include "HexObject.hpp"
#include "DynamicText.h"
#include "Utils.hpp"
#include "InputHandler.hpp"

HexObject hexObject;

int max_rows = 0;
 
int num_viewable_rows = 0;

int global_position = 0;

int cursor_x = 0;
int cursor_y = 0;

bool selectionMode = false;
int selection_start = 0;

std::vector<int> matches;
std::vector<int> match_colors;
std::vector<int> selections = {0};

enum class HexEditorModes {
    VIEW_MODE,
    EDIT_MODE,
    PATTERN_MODE,
    COMMAND_MODE,
};

constexpr int addressViewSize = 8;
constexpr int byteViewSize = 3 * 16 - 1;
constexpr int asciiViewSize = 16;
constexpr int interpreterViewSize = 35;
constexpr int hexEditorSize = addressViewSize + byteViewSize + asciiViewSize + interpreterViewSize + 3;


std::regex hex_regex("0[xX][0-9a-fA-F]+");

int get_viewable_text_rows(ftxui::Screen &screen) { return screen.dimy() - 4; }

ftxui::Elements UpdateRow(int row, int start, int cursor, ftxui::Elements& ascii_view) {
    ftxui::Elements row_view;
    std::string tmp(16, '0');

    int row_size = std::distance(hexObject.get_ptr_at_index(start), hexObject.end());
    int end = (row_size > 16) ? 16 : row_size;

    for (int i = 0; i < end; i++) {
        int idx = i + start;

        std::byte val = hexObject.at(idx);

        if (hexObject._patternIndex.contains(idx)) {
            const auto [size, color] = hexObject._patternIndex[idx];
            for (int i = (idx + size - 1); i >= idx ; i--) {
                matches.push_back(i);
                match_colors.push_back(color);
            }
        }

        row_view.push_back(
            ftxui::text(std::format("{:02X}", static_cast<uint8_t>(val)))
        );
      
        if (matches.end() != std::find(matches.begin(), matches.end(), idx)) {
            row_view.back() |= ftxui::color(static_cast<ftxui::Color::Palette16>(match_colors.back()));
            match_colors.pop_back();
        }

        if (selections.end() != std::find(selections.begin(), selections.end(), idx)) { row_view.back() |= ftxui::inverted; }
      
        row_view.push_back(ftxui::separatorEmpty());

        tmp[i] = (val >= static_cast<std::byte>(' ') && val < static_cast<std::byte>('~')) ? static_cast<char>(val) : '.';
    }

    row_view.pop_back();
    ascii_view.push_back(ftxui::text(tmp));

    return row_view;
}


void UpdateScreen(ftxui::Elements& byte_view, ftxui::Elements& ascii_view, ftxui::Elements& address_view, int cursor_position) {
    byte_view.clear();
    ascii_view.clear();
    address_view.clear();
    matches.clear();
    match_colors.clear();
    
    int start = 0;
    int row = (cursor_position / 16);
    if (cursor_y == 0) {
        start = row;
    } else if (cursor_y == max_rows) {
        start = row - (num_viewable_rows - 1);
        start = (start < 0) ? 0 : start;
    } else {
        start = row - (cursor_y);
        start = (start < 0) ? 0 : start;
    }

    int end = (start + num_viewable_rows > max_rows) ? max_rows+1 : start + num_viewable_rows; 

    for (int i = start; i < end; i++) {
        address_view.push_back(ftxui::text(std::format("{:08X}", i * 16)));
        byte_view.push_back(ftxui::hbox(UpdateRow(i-start, i*16, cursor_position, ascii_view)));
    }
   
}

void UpdateByteInterpreter(ftxui::Elements& byte_interpreter_view, int row, int col) {
    int idx = row*16 + col;
    byte_interpreter_view.clear();
    byte_interpreter_view.push_back(ftxui::text(std::format("char    {}", *reinterpret_cast<const char*>(hexObject.get_ptr_at_index(idx)))));
    byte_interpreter_view.push_back(ftxui::text(std::format("uint8   {}", *reinterpret_cast<const uint8_t*>(hexObject.get_ptr_at_index(idx)))));
    byte_interpreter_view.push_back(ftxui::text(std::format("int8    {}", *reinterpret_cast<const int8_t*>(hexObject.get_ptr_at_index(idx)))));
    byte_interpreter_view.push_back(ftxui::text(std::format("uint16  {}", *reinterpret_cast<const uint16_t*>(hexObject.get_ptr_at_index(idx)))));
    byte_interpreter_view.push_back(ftxui::text(std::format("int16   {}", *reinterpret_cast<const int16_t*>(hexObject.get_ptr_at_index(idx)))));
    byte_interpreter_view.push_back(ftxui::text(std::format("uint32  {}", *reinterpret_cast<const uint32_t*>(hexObject.get_ptr_at_index(idx)))));
    byte_interpreter_view.push_back(ftxui::text(std::format("int32   {}", *reinterpret_cast<const int32_t*>(hexObject.get_ptr_at_index(idx)))));
    byte_interpreter_view.push_back(ftxui::text(std::format("uint64  {}", *reinterpret_cast<const uint64_t*>(hexObject.get_ptr_at_index(idx)))));
    byte_interpreter_view.push_back(ftxui::text(std::format("int64   {}", *reinterpret_cast<const int64_t*>(hexObject.get_ptr_at_index(idx)))));
    byte_interpreter_view.push_back(ftxui::text(std::format("float   {}", *reinterpret_cast<const float*>(hexObject.get_ptr_at_index(idx)))));
    byte_interpreter_view.push_back(ftxui::text(std::format("double  {}", *reinterpret_cast<const double*>(hexObject.get_ptr_at_index(idx)))));
}


int main(int argc, char* argv[]) {

    std::filesystem::path file_path;
    
    if (argc <= 1) {
        std::cout << "Error: no input file provided\n>>> HexEditor.exe [filePath]" << std::endl;
        return -1;
    } 

    file_path = std::filesystem::absolute(std::string(argv[1]));
    if (!std::filesystem::exists(file_path)) {
        std::cout << "File does not exist: " << file_path << std::endl;
        return -1;
    }

    hexObject.set_filepath(file_path);
    size_t bytesRead = hexObject.get_binary_data_from_file();
    max_rows = bytesRead / 16;
    
    using namespace ftxui;

    auto screen = ScreenInteractive::Fullscreen();
    auto screen_dim = Terminal::Size();
    num_viewable_rows = screen_dim.dimy - 4;

    Elements byte_view;
    Elements ascii_view;
    Elements address_view;
    Elements data_interpreter_view;

    int xloc = 0;
    int yloc = 0;

    HexEditorModes hexEditorMode = HexEditorModes::VIEW_MODE;

    UpdateScreen(byte_view, ascii_view, address_view, 0);
    UpdateByteInterpreter(data_interpreter_view, yloc, xloc);

    std::string answer = "";
    int editor_cursor_position = answer.size() + 1;
    InputOption editor_input_options;
    editor_input_options.content = &answer;
    editor_input_options.placeholder = "0x00";
    editor_input_options.multiline = false;

    editor_input_options.transform = [&](InputState state) {

        if (state.is_placeholder) { return state.element |= dim; }

        if (std::regex_match(answer, hex_regex)) {
            state.element |= color(Color::White);
        } else {
            state.element |= color(Color::Red);
        }

        return state.element;    
    };

    
    editor_input_options.cursor_position = &editor_cursor_position;

    Component hex_editor_view = Input(editor_input_options);
    Component hex_editor_command_view = Input(&answer, "");

    // Create Modal Layers
    auto hex_editor_renderer = Renderer([&] {
        return window(
            text("Hex Editor") | hcenter,
            hex_editor_view->Render() | hcenter) | size(WIDTH, EQUAL, 14) | size(HEIGHT, EQUAL, 3) | clear_under;
    });

    auto hex_editor_command_renderer = Renderer([&] {
        return window(
            text("Command Prompt") | hcenter,
            hex_editor_command_view->Render()) | size(WIDTH, EQUAL, 48) | size(HEIGHT, EQUAL, 3) | clear_under;
    });
    

    std::vector<std::vector<std::byte>> patterns;
    std::vector<std::pair<std::string, int>> input_data;
    input_data.reserve(10);
    Components pattern_input_components;
    auto pattern_component = Container::Vertical({});
   
    auto hex_editor_pattern_renderer = Renderer([&] {
        return window(
            text(std::format(" Pattern View ")),
            vbox(pattern_component->Render())
        ) | clear_under;
    });
    
    auto hex_viewer_renderer = Renderer([&] {
        return vbox({
            text(std::format("{} {}:{}", selections.size(), yloc, xloc)),
            separator(),
            hbox({
                vbox(address_view) | size(WIDTH, EQUAL, addressViewSize),
                separator(),
                vbox(byte_view) | size(WIDTH, EQUAL, byteViewSize),
                separator(),
                vbox(ascii_view) | size(WIDTH, EQUAL, asciiViewSize),
                separator(),
                vbox(data_interpreter_view,
                     separator(), 
                     hex_editor_pattern_renderer->Render()) | size(WIDTH, EQUAL, interpreterViewSize)
            })
        }) | border;
    });


    // auto hex_editor_pattern_renderer = Renderer([&] {
    //     return window(
    //         text(std::format(" Pattern View ")),
    //         vbox(pattern_component->Render())
    //     ) | clear_under;
    // });


    auto main_window_renderer = Renderer([&] {
        Element current_view = hex_viewer_renderer->Render();
            
        if (hexEditorMode == HexEditorModes::EDIT_MODE) {
            current_view = dbox({current_view, hex_editor_renderer->Render() | center});
        } else if (hexEditorMode == HexEditorModes::COMMAND_MODE) {
            current_view = dbox({current_view, hex_editor_command_renderer->Render() | center});
        // } else if (hexEditorMode == HexEditorModes::PATTERN_MODE) {
            // current_view = dbox({current_view, hex_editor_pattern_renderer->Render() | center});
        } else {
            // HexView
        }

        return current_view | size(WIDTH, EQUAL, hexEditorSize);
    });

    InputHandler inputHandler;

    main_window_renderer |= CatchEvent([&](Event event) {

        InputHandler::Command command = InputHandler::Command::NOP;
         
        switch (hexEditorMode) {
            case HexEditorModes::VIEW_MODE:

                command = inputHandler.ViewModeInputHandler(event);

                switch (command) {
                    case InputHandler::Command::UP:
                        if (yloc > 0) {
                            yloc--;
                            if (cursor_y > 0) { cursor_y--; } 
                        }
                        break;
                    case InputHandler::Command::DOWN:
                        if (yloc < max_rows - 1) {
                            yloc++;
                            if (cursor_y < num_viewable_rows - 1) { cursor_y++; }
                        }
                        break;
                    case InputHandler::Command::LEFT:
                        if (xloc > 0) {
                            xloc--;
                            cursor_x--;
                        }
                        break;
                    case InputHandler::Command::RIGHT:
                        if (xloc < 15) {
                            xloc++;
                            cursor_x++;
                        }
                        break;
                    case InputHandler::Command::ESCAPE:

                        hexEditorMode = HexEditorModes::VIEW_MODE;

                        selectionMode = false;
                        selections.clear();
                        selections.push_back(yloc*16 + xloc);

                        answer = "";
                        
                        break;
                    case InputHandler::Command::SELECTION_MODE:
                        selectionMode = true;
                        selection_start = yloc*16 + xloc;
                        break;
                    case InputHandler::Command::EDIT_MODE:
                        hexEditorMode = HexEditorModes::EDIT_MODE;
                        break;
                    case InputHandler::Command::PATTERN_MODE:
                        hexEditorMode = HexEditorModes::PATTERN_MODE;
                        break;
                    case InputHandler::Command::NOP:
                        return true;
                        break;
                    default:
                        assert("View Mode: impossible");
                        break;                    
                }

                if (selectionMode == false) {
                    selections.back() = yloc*16 + xloc;
                } else {
                    selections.clear();
                    int end = yloc*16+xloc;
                    if (selection_start < end) {
                        for (int i = selection_start; i <= end; i++) {
                            selections.push_back(i);
                        }
                    } else {
                        for (int i = end; i <= selection_start; i++) {
                            selections.push_back(i);
                        }
                    }
                }
                                
                break;
            case HexEditorModes::EDIT_MODE:

                command = inputHandler.EditModeInputHandler(event);

                switch (command) {
                    case InputHandler::Command::ENTER: {

                        int tmp = 0x00;
                        
                        if (!answer.empty()) {
                            if (!std::regex_match(answer, hex_regex)) { return true; }
                            tmp = std::stoi(answer, 0, 16);
                        }
                                                
                        if (selectionMode) {
                            for (const auto idx : selections) {
                                hexObject.set_byte(idx, static_cast<uint8_t>(tmp));
                            }
                        } else {
                            hexObject.set_byte(selections.back(), static_cast<uint8_t>(tmp));
                        }                

                        hexEditorMode = HexEditorModes::VIEW_MODE;
                    
                        break;
                    }
                    case InputHandler::Command::ESCAPE:

                        hexEditorMode = HexEditorModes::VIEW_MODE;

                        break;
                    case InputHandler::Command::PASS:
                        return hex_editor_view->OnEvent(event);
                        break;
                    default:
                        break;
                }
                break;
            case HexEditorModes::PATTERN_MODE:

                command = inputHandler.PatternModeInputHandler(event);

                switch (command) {
                    case InputHandler::Command::ENTER:

                        patterns.clear();

                        for (const auto& data : input_data) {
                            if (data.first == "" || std::regex_match(data.first, hex_regex) == false) { continue; }
                            uint64_t pattern_value = stoi(data.first, 0, 16);
                            int num_bytes = (data.first.size() - 2) / 2;

                            std::vector<std::byte> tmp;
                            for (auto i = 0; i < num_bytes; i++) {
                                std::byte byte = static_cast<std::byte>((pattern_value >> i*8) & 0xFF);
                                tmp.push_back(byte);
                            }
                            std::reverse(tmp.begin(), tmp.end());
                            patterns.push_back(tmp);
                        }

                        hexObject.find_in_file(patterns);
                        hexEditorMode = HexEditorModes::VIEW_MODE;

                        break;
                    case InputHandler::Command::NEW_PATTERN: {
                        
                        InputOption option;
                        input_data.push_back({"", 0});
                        option.placeholder = "0x00";
                        option.multiline = false;
                        option.content = &input_data.back().first;
                        option.cursor_position = &input_data.back().second;
                        option.transform = [](InputState state) {
                            
                            if (!state.focused) {
                                state.element |= dim;
                            }
                        
                            return state.element | color(Color::White);  
                        };
                        pattern_component->Add(Input(option));

                        return true;
                        break;
                    }
                    case InputHandler::Command::DELETE_PATTERN:
                        if (!input_data.empty()) { input_data.pop_back(); }
                        pattern_component->Detach();
                        break;
                    case InputHandler::Command::ESCAPE:
                        hexEditorMode = HexEditorModes::VIEW_MODE;
                        break;
                    case InputHandler::Command::PASS:
                        return pattern_component->OnEvent(event);
                    default:
                        break;

                    break;
                }
            case HexEditorModes::COMMAND_MODE:

                if (event == Event::Return) {
                    if (answer == ":w") {
                        hexObject.save_file();
                    } else if (answer == ":q") {
                        screen.Exit();
                    } else {
                        assert("Command Mode: command not implemented");
                    }
                } else {
                    return hex_editor_command_view->OnEvent(event);
                }
                break;
            default:
                assert("Unsupported Mode");
                break;
        }

        UpdateScreen(byte_view, ascii_view, address_view, yloc*16 + xloc);
        UpdateByteInterpreter(data_interpreter_view, yloc, xloc);

        return true;

    });

    Loop loop(&screen, main_window_renderer);
    while (!loop.HasQuitted()) {
        num_viewable_rows = get_viewable_text_rows(screen);
        loop.RunOnce();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return 0;
}

