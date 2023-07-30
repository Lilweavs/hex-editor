#include "ftxui/component/component.hpp" // for Input, Renderer, ResizableSplitLeft
#include "ftxui/component/loop.hpp"
#include "ftxui/component/screen_interactive.hpp" // for ScreenInteractive
#include "ftxui/dom/elements.hpp" // for operator|, separator, text, Element, flex, vbox, border
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
#include <HexObject.hpp>
#include <DynamicText.h>


HexObject hexObject;

int max_rows = 0;
 
int num_viewable_rows = 0;

int global_position = 0;

int cursor_x = 0;
int cursor_y = 0;

bool selectionMode = false;

std::vector<std::pair<int, int>> selections;
std::vector<std::string> hex_strings;
std::vector<std::string> ascii_strings;
std::vector<std::string> row_strings;
std::array<std::string, 10> byte_interpreter_strings;

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

void update_row(int row, int cursor_y) {
    
    char tmp[17] = {0};
    for (int j = 0; j < 16; j++) {
        char val = hexObject.at(row * 16 + j);
        hex_strings.at(cursor_y*16 + j) = std::format("{:02X}", static_cast<uint8_t>(val));

        tmp[j] = (val >= ' ' && val < '~') ? val : '.';
    }
    ascii_strings.at(cursor_y) = std::string(tmp);
}

void UpdateScreen(int row, int cursor_y) {
    int start = row - cursor_y;

    char tmp[17] = {0};
    for (int i = start; i < (start + num_viewable_rows); i++) {
        row_strings.at(i - start) = std::format("{:08X}", i * 16);
        update_row(i, i - start);
    }
}

void scrollScreen(int row, int col, int cursor_y) {

    int start = 0, end = 0;

    if (cursor_y == 0) {
        start = row;
        end = row + num_viewable_rows;
    } else {
        start = row - (num_viewable_rows - 1);
        end = row + 1;
    }

    char tmp[17] = {0};
    for (int i = start; i < end; i++) {
        row_strings.at(i - start) = std::format("{:08X}", i * 16);
        update_row(i, i - start);
    }
}

void updateByteInterpreter(int row, int col) {
    int idx = row*16 + col;
    byte_interpreter_strings.at(0) = std::format("uint8   {}", *reinterpret_cast<const uint8_t*>(hexObject.get_ptr_at_index(idx)));
    byte_interpreter_strings.at(1) = std::format("int8    {}", *reinterpret_cast<const int8_t*>(hexObject.get_ptr_at_index(idx)));
    byte_interpreter_strings.at(2) = std::format("uint16  {}", *reinterpret_cast<const uint16_t*>(hexObject.get_ptr_at_index(idx)));
    byte_interpreter_strings.at(3) = std::format("int16   {}", *reinterpret_cast<const int16_t*>(hexObject.get_ptr_at_index(idx)));
    byte_interpreter_strings.at(4) = std::format("uint32  {}", *reinterpret_cast<const uint32_t*>(hexObject.get_ptr_at_index(idx)));
    byte_interpreter_strings.at(5) = std::format("int32   {}", *reinterpret_cast<const int32_t*>(hexObject.get_ptr_at_index(idx)));
    byte_interpreter_strings.at(6) = std::format("uint64  {}", *reinterpret_cast<const uint64_t*>(hexObject.get_ptr_at_index(idx)));
    byte_interpreter_strings.at(7) = std::format("int64   {}", *reinterpret_cast<const int64_t*>(hexObject.get_ptr_at_index(idx)));
    byte_interpreter_strings.at(8) = std::format("float   {}", *reinterpret_cast<const float*>(hexObject.get_ptr_at_index(idx)));
    byte_interpreter_strings.at(9) = std::format("double  {}", *reinterpret_cast<const double*>(hexObject.get_ptr_at_index(idx)));
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

    // TODO: Move to non-vector type to gaurantee no reallocations
    // This will cause the string pointers in dynamictext to point to bad memory
    // locations
    ascii_strings.reserve(100);
    row_strings.reserve(100);

    auto screen = ScreenInteractive::Fullscreen();
    auto screen_dim = Terminal::Size();
    num_viewable_rows = screen_dim.dimy - 4;

    Elements rowByteIndex;
    for (int i = 0; i < num_viewable_rows; i++) {
        row_strings.push_back(std::format("{:08X}", i * 16));
        rowByteIndex.push_back(dynamictext(row_strings.back()) | dim);
    }

    Elements rowByteAscii;
    for (int i = 0; i < num_viewable_rows; i++) {
        char tmp[17] = {0};
        for (int j = 0; j < 16; j++) {
            char val = hexObject.at(i * 16 + j);

            tmp[j] = (val >= ' ' && val < '~') ? val : '.';
        }
        ascii_strings.push_back(std::string(tmp));
        rowByteAscii.push_back(dynamictext(ascii_strings.back()));
    }

    int modalDepth = 0;
    std::vector<Element> view;

    int xloc = 0;
    int yloc = 0;

    std::string ydimText;
    bool replaceMode = false;

    for (int i = 0; i < num_viewable_rows; i++) {
        for (int j = 0; j < 16; j++) {
            hex_strings.push_back(std::format("{:02X}", static_cast<uint8_t>(hexObject.at(i * 16 + j))));
        }
    }

    for (int i = 0; i < num_viewable_rows; i++) {
        Elements bytes;
        for (int j = 0; j < 16; j++) {
            bytes.push_back(dynamictext(hex_strings.at(i*16 + j)));
            bytes.push_back(separatorEmpty());
        }
        bytes.pop_back();
        view.push_back(hbox(bytes));
    }

    view.at(0)->GetChildren().at(0) |= inverted;

    std::string answer = "";
    Component hex_editor_view = Input(&answer, "0x00");
    Component hex_editor_command_view = Input(&answer, "");


    byte_interpreter_strings.at(0) = std::format("uint8   {}", *reinterpret_cast<uint8_t*>(hexObject.get_ptr_at_index(0)));
    byte_interpreter_strings.at(1) = std::format("int8    {}", *reinterpret_cast<int8_t*>(hexObject.get_ptr_at_index(0)));
    byte_interpreter_strings.at(2) = std::format("uint16  {}", *reinterpret_cast<uint16_t*>(hexObject.get_ptr_at_index(0)));
    byte_interpreter_strings.at(3) = std::format("int16   {}", *reinterpret_cast<int16_t*>(hexObject.get_ptr_at_index(0)));
    byte_interpreter_strings.at(4) = std::format("uint32  {}", *reinterpret_cast<uint32_t*>(hexObject.get_ptr_at_index(0)));
    byte_interpreter_strings.at(5) = std::format("int32   {}", *reinterpret_cast<int32_t*>(hexObject.get_ptr_at_index(0)));
    byte_interpreter_strings.at(6) = std::format("uint64  {}", *reinterpret_cast<uint64_t*>(hexObject.get_ptr_at_index(0)));
    byte_interpreter_strings.at(7) = std::format("int64   {}", *reinterpret_cast<int64_t*>(hexObject.get_ptr_at_index(0)));
    byte_interpreter_strings.at(8) = std::format("float   {}", *reinterpret_cast<float*>(hexObject.get_ptr_at_index(0)));
    byte_interpreter_strings.at(9) = std::format("double  {}", *reinterpret_cast<double*>(hexObject.get_ptr_at_index(0)));

    std::vector<Element> data_interpreter_view;
    for (auto& str : byte_interpreter_strings) {
        data_interpreter_view.push_back(dynamictext(str));
    }

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
                vbox(rowByteIndex) | size(WIDTH, EQUAL, 8),
                separator(),
                vbox(view) | size(WIDTH, EQUAL, 3 * 16 - 1),
                separator(),
                vbox(rowByteAscii) | size(WIDTH, EQUAL, 16),
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
                ResetSelectionMode(view);
                selectionMode = false;
            }
            
            return true;
        }

        char c = event.character().at(0);

        if (c == ':') { modalDepth = 2; }

        if (modalDepth == 0) {

            if (c == 'r') { modalDepth = 1; return true; }
            if (c == 'v') {
                if (selectionMode == true) {
                    ResetSelectionMode(view);
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
                        scrollScreen(yloc, xloc, cursor_y);
                    }
                }
            } else if (c == 'k') {
                if (yloc > 0) {
                    yloc--;
                    if (cursor_y > 0) {
                        cursor_y--;
                    } else {
                        scrollScreen(yloc, xloc, cursor_y);
                    }
                }
            } else {
                // Do nothing
            }

            if (selectionMode == true) {
                update_multi_selection(view, cursor_x, cursor_y);
            } else {
                update_single_selction(view, cursor_x, cursor_y);
            }

            updateByteInterpreter(yloc, xloc);
            
        } else if (modalDepth == 1) {

            c = std::toupper(c);

            if (event == Event::Return) {
                int tmp = (answer.empty()) ? 0x00 : std::stoi(answer, nullptr, 16);
                if (tmp > 0xFF) { assert("something went wrong"); }


                if (selectionMode) {
                    for (const auto& [x, y] : selections) {
                        hexObject.set_byte(y*16 + x, static_cast<uint8_t>(tmp));
                    }
                    UpdateScreen(yloc, cursor_y);
                                                    
                } else {
                    hexObject.set_byte(yloc*16 + xloc, static_cast<uint8_t>(tmp));
                    update_row(yloc, cursor_y);
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