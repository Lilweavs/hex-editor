#include <ftxui/component/component.hpp> // for Input, Renderer, ResizableSplitLeft
#include <ftxui/component/loop.hpp>
#include <ftxui/component/screen_interactive.hpp> // for ScreenInteractive
#include <ftxui/dom/elements.hpp> // for operator|, separator, text, Element, flex, vbox, border
#include <ftxui/component/event.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/terminal.hpp>
#include <memory> // for allocator, __shared_ptr_access, shared_ptr
#include <string> // for string
#include <thread>
#include <filesystem>
#include <cassert>
#include <format>
#include "HexObject.hpp"
#include "DynamicText.h"
#include "Utils.hpp"


HexObject hexObject;

int max_rows = 0;
 
int num_viewable_rows = 0;

int global_position = 0;

int cursor_x = 0;
int cursor_y = 0;

bool selectionMode = false;

std::vector<std::pair<int, int>> selections;

int get_viewable_text_rows(ftxui::Screen &screen) { return screen.dimy() - 4; }

void ResetSelectionMode(std::vector<ftxui::Element>& view) {
    for (const auto [tmpx, tmpy] : selections) {
        view.at(tmpy)->GetChildren().at(tmpx * 2) |= ftxui::inverted;
    }
    auto curr_selection = selections.back();
    selections.clear();
    selections.push_back(curr_selection);
    view.at(curr_selection.second)->GetChildren().at(curr_selection.first * 2) |= ftxui::inverted;
}

void update_single_selction(std::vector<ftxui::Element>& view, int curr_x, int curr_y) {
    auto [last_x, last_y] = selections.back();
    selections.pop_back();
    
    int dy = curr_y - last_y;
    int dx = curr_x - last_x;

    view.at(last_y)->GetChildren().at(last_x * 2) |= ftxui::inverted;
    view.at(curr_y)->GetChildren().at(curr_x * 2) |= ftxui::inverted;
    selections.push_back({curr_x, curr_y});
}

void update_multi_selection(std::vector<ftxui::Element>& view, int curr_x, int curr_y) {
    auto [last_x, last_y] = selections.back();

    int dy = curr_y - last_y;
    int dx = curr_x - last_x;
    
    if (dy == 0) {

        if (dx > 0) {
            view.at(curr_y)->GetChildren().at(curr_x * 2) |= ftxui::inverted;
            selections.push_back({curr_x, curr_y});
        } else if (dx < 0) {
            view.at(last_y)->GetChildren().at(last_x * 2) |= ftxui::inverted;
            selections.pop_back();
        } else {
            
        }

    } else if (dy > 0) {
      
        for (size_t i = curr_x+1; i < 16; i++) {
            view.at(last_y)->GetChildren().at(i * 2) |= ftxui::inverted;
            selections.push_back({i, last_y});
        }

        for (size_t i = 0; i < curr_x+1; i++) {
            view.at(curr_y)->GetChildren().at(i * 2) |= ftxui::inverted;
            selections.push_back({i, curr_y});
        }
        
    } else {

        for (size_t i = 0; i < 16; i++) {
            auto [tmpx, tmpy] = selections.back();
            selections.pop_back();
            view.at(tmpy)->GetChildren().at(tmpx * 2) |= ftxui::inverted;
        }
    }
}

bool is_selected(int cx, int cy) {
    bool selected = false;
    for (const auto& [x, y] : selections) {
        selected = (x == cx) & (y == cy);
        if (selected) { return true; }
    }
    return false;
}

std::vector<int> matches;

ftxui::Elements UpdateRow(int row, int start, int cursor, ftxui::Elements& ascii_view) {
    ftxui::Elements row_view;
    std::string tmp(16, '0');

    for (int i = 0; i < 16 ; i++) {
        int idx = i + start;

        std::byte val = hexObject.at(idx);

        if (hexObject._patternIndex.contains(idx)) {
            for (int i = (idx + hexObject._patternIndex[idx] - 1); i >= idx ; i--) {
                matches.push_back(i);
            }
        }


        if (matches.end() == std::find(matches.begin(), matches.end(), idx)) {
            if (i + start != cursor) {
                row_view.push_back(
                    ftxui::text(std::format("{:02X}", static_cast<uint8_t>(val)))
                );
            } else {
                row_view.push_back(
                    ftxui::text(std::format("{:02X}", static_cast<uint8_t>(val))) | ftxui::inverted
                );
            }
        } else {
            if (i + start != cursor) {
                row_view.push_back(
                    ftxui::text(std::format("{:02X}", static_cast<uint8_t>(val))) | ftxui::color(ftxui::Color::Palette16::Red)
                );
            } else {
                row_view.push_back(
                    ftxui::text(std::format("{:02X}", static_cast<uint8_t>(val))) | ftxui::inverted | ftxui::color(ftxui::Color::Palette16::Red)
                );
            }
        }

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

    for (int i = start; i < (start + num_viewable_rows); i++) {
        address_view.push_back(ftxui::text(std::format("{:08X}", i * 16)));
        byte_view.push_back(ftxui::hbox(UpdateRow(i-start, i*16, cursor_position, ascii_view)));
    }
   
}

void UpdateByteInterpreter(ftxui::Elements& byte_interpreter_view, int row, int col) {
    int idx = row*16 + col;
    byte_interpreter_view.clear();
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
    hexObject.find_in_file();
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

    std::string answer = "";
    Component hex_editor_view = Input(&answer, "0x00");
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

    auto main_window_renderer = Renderer([&] {
        Element current_view = hex_viewer_renderer->Render();

        if (modalDepth == 1) {
            current_view = dbox({current_view, hex_editor_renderer->Render() | center});
        } else if (modalDepth == 2) {
            current_view = dbox({current_view, hex_editor_command_renderer->Render() | center});
        }

        return current_view | size(WIDTH, EQUAL, 107);
    });

    main_window_renderer |= CatchEvent([&](Event event) {

        if (event == Event::Escape) {
            modalDepth = 0;
            answer.clear();

            if (selectionMode == true) {
                ResetSelectionMode(byte_view);
                selectionMode = false;
            }
            
            return true;
        }

        char c = event.character().at(0);

        if (c == ':') { modalDepth = 2; }

        if (c == 'Y') {
            std::string tmp;
            for (const auto& [x, y] : selections) {
                tmp += std::format("{:02X} ", static_cast<uint8_t>(hexObject.at(y*16 + x)));
            }
            AddToClipBoard(tmp);
        }

        if (modalDepth == 0 && event.is_character()) {
            
            if (c == 'r') { modalDepth = 1; return true; }
            if (c == 'v') {
                if (selectionMode == true) {
                    ResetSelectionMode(byte_view);
                }
                selectionMode ^= true;
                return true;
            }

            if (c == 'l') {
                if (xloc < 15) {
                    xloc++;
                    cursor_x++;
                }
            } else if (c == 'h') {
                if (xloc > 0) {
                    xloc--;
                    cursor_x--;
                }
            } else if (c == 'j') {
                if (yloc < max_rows - 1) {
                    yloc++;
                    if (cursor_y < num_viewable_rows - 1) {
                        cursor_y++;
                    } else {
                        // UpdateScreen(byte_view, ascii_view, address_view, yloc*16 + xloc);
                    }
                }
            } else if (c == 'k') {
                if (yloc > 0) {
                    yloc--;
                    if (cursor_y > 0) {
                        cursor_y--;
                    } else {
                        // UpdateScreen(byte_view, ascii_view, address_view, yloc*16 + xloc);
                    }
                }
            } else {
                // Do nothing
            }

            UpdateScreen(byte_view, ascii_view, address_view, yloc*16 + xloc);
                
            
            // if (selectionMode == true) {
            //     update_multi_selection(byte_view, cursor_x, cursor_y);
            // } else {
            //     update_single_selction(byte_view, cursor_x, cursor_y);
            // }

            UpdateByteInterpreter(data_interpreter_view, yloc, xloc);
            
        } else if (modalDepth == 1) {

            c = std::toupper(c);

            if (event == Event::Return) {
                int tmp = (answer.empty()) ? 0x00 : std::stoi(answer, nullptr, 16);
                if (tmp > 0xFF) { assert("something went wrong"); }


                if (selectionMode) {
                    for (const auto& [x, y] : selections) {
                        hexObject.set_byte(y*16 + x, static_cast<uint8_t>(tmp));
                    }
                    // UpdateScreen(yloc, cursor_y);
                                                    
                } else {
                    hexObject.set_byte(yloc*16 + xloc, static_cast<uint8_t>(tmp));
                    // update_row(yloc, cursor_y);
                    global_position = tmp;
                }                

                modalDepth = 0;
                answer.clear();
                return true;
            }
            
            if (Event::Backspace == event || Event::ArrowLeft == event || Event::ArrowRight == event) {
                return hex_editor_view->OnEvent(event);
            }

            if (answer.size() >= 4) { return true; }
            
            if (c == 'X') {
                return hex_editor_view->OnEvent(Event::Character(static_cast<char>(std::tolower(c))));
            }
            
            if (c <= '9' && c >= '0' || c <= 'F' && c >= 'A') {
                return hex_editor_view->OnEvent(Event::Character(c));
            }
            
        } else {
            
            if (event == Event::Return) {
                
                if (answer == ":w") {
                    hexObject.save_file();
                } else if (answer == ":q") {
                    screen.Exit();
                } else {
                    assert("command not implemented");
                }
                answer.clear();
                modalDepth = 0;
                return true;;                
            }

            return hex_editor_command_view->OnEvent(event);
        }

        return true;
    });

    selections.push_back({cursor_x, cursor_x});

    Loop loop(&screen, main_window_renderer);
    while (!loop.HasQuitted()) {
        num_viewable_rows = get_viewable_text_rows(screen);
        loop.RunOnce();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return 0;
}

// Copyright 2020 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.