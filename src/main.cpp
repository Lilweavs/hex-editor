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
#include "Controller.hpp"

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

enum class ModalView {
    HexView,
    HexEditView,
    HexCommandView,
    HexPatternView,
};

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

enum class RegexError { NO_REGEX_ERROR, REGEX_ERROR };

RegexError ErrorInPatternInput(const std::vector<std::pair<std::string, int>>& input_data) {

    for (const auto& data : input_data) {
        if (data.first.empty()) { continue; }
        if (std::regex_search(data.first, hex_regex) == false) {
            return RegexError::REGEX_ERROR;
        }               
    }

    return RegexError::NO_REGEX_ERROR;
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

    int modalDepth = 0;

    int xloc = 0;
    int yloc = 0;

    std::string ydimText;
    bool replaceMode = false;

    UpdateScreen(byte_view, ascii_view, address_view, 0);

    std::string answer = "0x00";
    int editor_cursor_position = answer.size() + 1;
    InputOption editor_input_options;
    editor_input_options.content = &answer;
    editor_input_options.multiline = false;

    editor_input_options.transform = [&](InputState state) {

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
    
    auto hex_viewer_renderer = Renderer([&] {
        return vbox({
            text(std::format("{} {}:{}", selections.size(), yloc, xloc)),
            separator(),
            hbox({
                vbox(address_view) | size(WIDTH, EQUAL, 8),
                separator(),
                vbox(byte_view) | size(WIDTH, EQUAL, 3 * 16 - 1),
                separator(),
                vbox(ascii_view) | size(WIDTH, EQUAL, 16),
                separator(),
                vbox(data_interpreter_view)
            })
        }) | border;
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

    auto main_window_renderer = Renderer([&] {
        Element current_view = hex_viewer_renderer->Render();
            
        // if (modal_view == ModalView::HexEditView) {
        //     current_view = dbox({current_view, hex_editor_renderer->Render() | center});
        // } else if (modal_view == ModalView::HexCommandView) {
        //     current_view = dbox({current_view, hex_editor_command_renderer->Render() | center});
        // } else if (modal_view == ModalView::HexPatternView) {
        //     current_view = dbox({current_view, hex_editor_pattern_renderer->Render() | center});
        // } else {
        //     // already in HexEditView
        // }

        if (modalDepth == 1) {
            current_view = dbox({current_view, hex_editor_renderer->Render() | center});
        } else if (modalDepth == 2) {
            current_view = dbox({current_view, hex_editor_command_renderer->Render() | center});
        } else if (modalDepth == 3) {
            current_view = dbox({current_view, hex_editor_pattern_renderer->Render() | center});
        } else {
            // HexView
        }

        return current_view | size(WIDTH, EQUAL, 107);
    });

    Controller controller;

    main_window_renderer |= CatchEvent([&](Event event) {


        Controller::Command command = controller.InputHandler(event, modalDepth);

        if (command == Controller::Command::NOP) { return true; }

        if (command == Controller::Command::ESCAPE) { modalDepth = 0; answer = "0x00"; return true; }

        if (modalDepth == 0) {

            if (command == Controller::Command::EDIT_MODE) { modalDepth = 1; return true; }
            
            if (command == Controller::Command::SELECTION_MODE) {
                selectionMode ^= true;
                selections.clear();
                selections.push_back(yloc*16 + xloc);
                selection_start = yloc*16 + xloc;
                return true;
            }

            if (command == Controller::Command::PATTERN_MODE) {
                modalDepth = 3;
                if (pattern_component->ChildCount() == 0) {
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
                }
                return true;
            }

            if (command == Controller::Command::RIGHT) {
                if (xloc < 15) {
                    xloc++;
                    cursor_x++;
                }
            } else if (command == Controller::Command::LEFT) {
                if (xloc > 0) {
                    xloc--;
                    cursor_x--;
                }
            } else if (command == Controller::Command::DOWN) {
                if (yloc < max_rows - 1) {
                    yloc++;
                    if (cursor_y < num_viewable_rows - 1) { cursor_y++; }
                }
            } else if (command == Controller::Command::UP) {
                if (yloc > 0) {
                    yloc--;
                    if (cursor_y > 0) { cursor_y--; } 
                }
            } else {
                // Do nothing
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
           
        } else if (modalDepth == 1) {
            
            if (event == Event::Return) {

                if (std::regex_match(answer, hex_regex) == false) {
                    return true;
                }
                
                int tmp = (answer.empty()) ? 0x00 : std::stoi(answer, 0, 16);

                if (selectionMode) {
                    for (const auto idx : selections) {
                        hexObject.set_byte(idx, static_cast<uint8_t>(tmp));
                    }
                } else {
                    hexObject.set_byte(selections.back(), static_cast<uint8_t>(tmp));
                    global_position = tmp;
                }                

                modalDepth = 0;
            } else {
                hex_editor_view->OnEvent(event);
            }
            
        } else if (modalDepth == 2) {
            
        } else if (modalDepth == 3) {

            if (event == Event::Return) {
                
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
                modalDepth = 0;
                UpdateScreen(byte_view, ascii_view, address_view, yloc*16 + xloc);
                UpdateByteInterpreter(data_interpreter_view, yloc, xloc);
                return true;
            } else if (command == Controller::Command::NEW_PATTERN) {

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
            } else if (command == Controller::Command::DELETE_PATTERN) {
                input_data.pop_back();
                return true;
            }

            return pattern_component->OnEvent(event); 

        } else {

            assert("shouldn't happen");

        }

        UpdateScreen(byte_view, ascii_view, address_view, yloc*16 + xloc);
        UpdateByteInterpreter(data_interpreter_view, yloc, xloc);
        // if (event == Event::Escape) {

        //     if (modalDepth == 3) {
        //         if (RegexError::NO_REGEX_ERROR == ErrorInPatternInput(input_data)) {
                    // patterns.clear();

        //             for (const auto& data : input_data) {
        //                 if (data.first == "") { continue; }
        //                 uint64_t pattern_value = stoi(data.first, 0, 16);
        //                 int num_bytes = (data.first.size() - 2) / 2;

        //                 std::vector<std::byte> tmp;
        //                 for (auto i = 0; i < num_bytes; i++) {
        //                     std::byte byte = static_cast<std::byte>((pattern_value >> i*8) & 0xFF);
        //                     tmp.push_back(byte);
        //                 }
        //                 std::reverse(tmp.begin(), tmp.end());
        //                 patterns.push_back(tmp);
        //             }
                                        
        //             hexObject.find_in_file(patterns);
        //         } else {
        //             return true;
        //         }                
        //     }
            
        //     modalDepth = 0;
        //     answer.clear();

        //     if (selectionMode == true) {
        //         selections.clear();
        //         selections.push_back(yloc*16 + xloc);
        //         selectionMode = false;
        //         UpdateScreen(byte_view, ascii_view, address_view, yloc*16 + xloc);
        //     }
            
        //     return true;
            
        // }

        // char c = event.character().at(0);

        // if (c == ':') { modalDepth = 2; }

        // if (c == 'Y') {
        //     std::string tmp;
            
        //     for (const auto idx : selections) {
        //         tmp += std::format("{:02X} ", static_cast<uint8_t>(hexObject.at(idx)));
        //     }
        //     AddToClipBoard(tmp);
        // }

        // if (c == 'f' && modalDepth != 3) {
        //     if (pattern_component->ChildCount() == 0) {
        //         InputOption option;
        //         input_data.push_back({"", 0});
        //         option.placeholder = "0x00";
        //         option.multiline = false;
        //         option.content = &input_data.back().first;
        //         option.cursor_position = &input_data.back().second;
    
        //         pattern_component->Add(Input(option));
        //     }
            
        //     modalDepth = 3;
        //     return true;
        // }

        // if (modalDepth == 0 && event.is_character()) {
            
        //     if (c == 'r') { modalDepth = 1; return true; }
        //     if (c == 'v') {
        //         selectionMode ^= true;
        //         selections.clear();
        //         selections.push_back(yloc*16 + xloc);
        //         selection_start = yloc*16 + xloc;
        //     }

        //     if (c == 'l') {
        //         if (xloc < 15) {
        //             xloc++;
        //             cursor_x++;
        //         }
        //     } else if (c == 'h') {
        //         if (xloc > 0) {
        //             xloc--;
        //             cursor_x--;
        //         }
        //     } else if (c == 'j') {
        //         if (yloc < max_rows - 1) {
        //             yloc++;
        //             if (cursor_y < num_viewable_rows - 1) { cursor_y++; }
        //         }
        //     } else if (c == 'k') {
        //         if (yloc > 0) {
        //             yloc--;
        //             if (cursor_y > 0) { cursor_y--; } 
        //         }
        //     } else {
        //         // Do nothing
        //     }
            
        //     if (selectionMode == false) {
        //         selections.back() = yloc*16 + xloc;
        //     } else {
        //         selections.clear();
        //         int end = yloc*16+xloc;
        //         if (selection_start < end) {
        //             for (int i = selection_start; i <= end; i++) {
        //                 selections.push_back(i);
        //             }
        //         } else {
        //             for (int i = end; i <= selection_start; i++) {
        //                 selections.push_back(i);
        //             }
        //         }
                
        //     }

        //     UpdateScreen(byte_view, ascii_view, address_view, yloc*16 + xloc);
        //     UpdateByteInterpreter(data_interpreter_view, yloc, xloc);
            
        // } else if (modalDepth == 1) {

        //     c = std::toupper(c);

        //     if (event == Event::Return) {
        //         int tmp = (answer.empty()) ? 0x00 : std::stoi(answer, nullptr, 16);
        //         if (tmp > 0xFF) { assert("something went wrong"); }


        //         if (selectionMode) {
        //             for (const auto idx : selections) {
        //                 hexObject.set_byte(idx, static_cast<uint8_t>(tmp));
        //             }
        //         } else {
        //             hexObject.set_byte(selections.back(), static_cast<uint8_t>(tmp));
        //             global_position = tmp;
        //         }                

        //         modalDepth = 0;
        //         answer.clear();
        //         return true;
        //     }
            
        //     if (Event::Backspace == event || Event::ArrowLeft == event || Event::ArrowRight == event) {
        //         return hex_editor_view->OnEvent(event);
        //     }

        //     if (answer.size() >= 4) { return true; }
            
        //     if (c == 'X') {
        //         return hex_editor_view->OnEvent(Event::Character(static_cast<char>(std::tolower(c))));
        //     }
            
        //     if (c <= '9' && c >= '0' || c <= 'F' && c >= 'A') {
        //         return hex_editor_view->OnEvent(Event::Character(c));
        //     }

        // } else if (modalDepth == 3) {

            // if (c == 'n') {
            //     InputOption option;
            //     input_data.push_back({"", 0});
            //     option.placeholder = "0x00";
            //     option.multiline = false;
            //     option.content = &input_data.back().first;
            //     option.cursor_position = &input_data.back().second;
            //     pattern_component->Add(Input(option));
            //     c = 0;
            // } else if (c == 'N') {
            //     input_data.pop_back();
            //     c = 0;
            // }

            // pattern_component->DetachAllChildren();

            // for (auto& config : input_data) {
            //     InputOption input_option;
            //     input_option.placeholder = "0x00";
            //     input_option.multiline = false;
            //     input_option.content = &config.first;
            //     input_option.cursor_position = &config.second;
                
            //     if (std::regex_search(config.first, hex_regex)) {
            //         pattern_component->Add(Input(input_option));
            //     } else {
            //         pattern_component->Add(Input(input_option) | color(ftxui::Color::Palette16::Red));
            //     }
            // }

        //     if (c) { return pattern_component->OnEvent(event); }
                                  
        // } else {
            
        //     if (event == Event::Return) {
                
        //         if (answer == ":w") {
        //             hexObject.save_file();
        //         } else if (answer == ":q") {
        //             screen.Exit();
        //         } else {
        //             assert("command not implemented");
        //         }
        //         answer.clear();
        //         modalDepth = 0;
        //         return true;                
        //     }

        //     return hex_editor_command_view->OnEvent(event);
        // }

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

