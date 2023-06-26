#include "ftxui/component/component.hpp" // for Input, Renderer, ResizableSplitLeft
#include "ftxui/component/loop.hpp"
#include "ftxui/component/screen_interactive.hpp" // for ScreenInteractive
#include "ftxui/dom/elements.hpp" // for operator|, separator, text, Element, flex, vbox, border
#include <cstdint>
#include <cstdio>
#include <format>
#include <fstream>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/terminal.hpp>
#include <memory> // for allocator, __shared_ptr_access, shared_ptr
#include <string> // for string
#include <thread>
#include <filesystem>

#define MAX_ROWS 12
#define X_OFFSET 1

class DynamicText : public ftxui::Node {
public:

    explicit DynamicText(std::string &text) { text_ = &text; }
    void ComputeRequirement() override {
        requirement_.min_x = ftxui::string_width(*text_);
        requirement_.min_y = 1;
    }

    void Render(ftxui::Screen &screen) override {
        int x = box_.x_min;
        const int y = box_.y_min;
        if (y > box_.y_max) {
            return;
        }
        for (const auto &cell : ftxui::Utf8ToGlyphs(*text_)) {
            if (x > box_.x_max) {
                return;
            }
            screen.PixelAt(x, y).character = cell;
            ++x;
        }
    }

private:

    std::string *text_;

};

ftxui::Element dynamictext(std::string &text) {
    return std::make_shared<DynamicText>(text);
}

#define BUFFER_SIZE 1024
char input_file_buffer[BUFFER_SIZE] = {0};

int max_rows = BUFFER_SIZE / 16;

int num_viewable_rows = 0;

int global_position = 0;

int cursor_x = 0;
int cursor_y = 0;

std::vector<std::vector<std::string>> hex_strings;
std::vector<std::string> ascii_strings;
std::vector<std::string> row_strings;
std::array<std::string, 10> byte_interpreter_strings;


int get_viewable_text_rows(ftxui::Screen &screen) { return screen.dimy() - 4; }

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
        for (int j = 0; j < 16; j++) {
            char val = input_file_buffer[i * 16 + j];
            hex_strings.at(i - start).at(j) =
            std::format("{:02X}", static_cast<uint8_t>(val));

            tmp[j] = (val >= ' ' && val < '~') ? val : '.';
        }
        ascii_strings.at(i - start) = std::string(tmp);
    }
}

void updateByteInterpreter(int row, int col) {
    int idx = row*16 + col;
    byte_interpreter_strings.at(0) = std::format("uint8   {}", *reinterpret_cast<uint8_t*>(&input_file_buffer[idx]));    
    byte_interpreter_strings.at(1) = std::format("int8    {}", *reinterpret_cast<int8_t*>(&input_file_buffer[idx]));    
    byte_interpreter_strings.at(2) = std::format("uint16  {}", *reinterpret_cast<uint16_t*>(&input_file_buffer[idx]));    
    byte_interpreter_strings.at(3) = std::format("int16   {}", *reinterpret_cast<int16_t*>(&input_file_buffer[idx]));    
    byte_interpreter_strings.at(4) = std::format("uint32  {}", *reinterpret_cast<uint32_t*>(&input_file_buffer[idx]));    
    byte_interpreter_strings.at(5) = std::format("int32   {}", *reinterpret_cast<int32_t*>(&input_file_buffer[idx]));    
    byte_interpreter_strings.at(6) = std::format("uint64  {}", *reinterpret_cast<uint64_t*>(&input_file_buffer[idx]));    
    byte_interpreter_strings.at(7) = std::format("int64   {}", *reinterpret_cast<int64_t*>(&input_file_buffer[idx]));    
    byte_interpreter_strings.at(8) = std::format("float   {}", *reinterpret_cast<float*>(&input_file_buffer[idx]));    
    byte_interpreter_strings.at(9) = std::format("double  {}", *reinterpret_cast<double*>(&input_file_buffer[idx]));    
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

    
    using namespace ftxui;

    // TODO: Move to non-vector type to gaurantee no reallocations
    // This will cause the string pointers in dynamictext to point to bad memory
    // locations
    ascii_strings.reserve(100);
    row_strings.reserve(100);

    std::ifstream file(file_path, std::ios::binary);
    file.read(input_file_buffer, BUFFER_SIZE);

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
            char val = input_file_buffer[i * 16 + j];

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
        std::vector<std::string> tmpStrings;
        for (int j = 0; j < 16; j++) {
            tmpStrings.push_back(std::format("{:02X}", static_cast<uint8_t>(input_file_buffer[i * 16 + j])));
        }
        hex_strings.push_back(tmpStrings);
    }

    for (int i = 0; i < num_viewable_rows; i++) {
        Elements bytes;
        std::vector<Element *> ref;
        for (int j = 0; j < 16; j++) {

            if (xloc == j and yloc == i) {
                bytes.push_back(dynamictext(hex_strings.at(i).at(j)) | inverted);
            } else {
                bytes.push_back(dynamictext(hex_strings.at(i).at(j)));
            }

            if (j == 15) {
                ref.push_back(&bytes.back());
            } else {
                ref.push_back(&bytes.back());
                bytes.push_back(separatorEmpty());
            }
        }
        view.push_back(hbox(bytes));
    }

    std::string answer = "";
    Component hex_editor_view = Input(&answer, "0x00");
    

    byte_interpreter_strings.at(0) = std::format("uint8   {}", *reinterpret_cast<uint8_t*>(&input_file_buffer[0]));    
    byte_interpreter_strings.at(1) = std::format("int8    {}", *reinterpret_cast<int8_t*>(&input_file_buffer[0]));    
    byte_interpreter_strings.at(2) = std::format("uint16  {}", *reinterpret_cast<uint16_t*>(&input_file_buffer[0]));    
    byte_interpreter_strings.at(3) = std::format("int16   {}", *reinterpret_cast<int16_t*>(&input_file_buffer[0]));    
    byte_interpreter_strings.at(4) = std::format("uint32  {}", *reinterpret_cast<uint32_t*>(&input_file_buffer[0]));    
    byte_interpreter_strings.at(5) = std::format("int32   {}", *reinterpret_cast<int32_t*>(&input_file_buffer[0]));    
    byte_interpreter_strings.at(6) = std::format("uint64  {}", *reinterpret_cast<uint64_t*>(&input_file_buffer[0]));    
    byte_interpreter_strings.at(7) = std::format("int64   {}", *reinterpret_cast<int64_t*>(&input_file_buffer[0]));    
    byte_interpreter_strings.at(8) = std::format("float   {}", *reinterpret_cast<float*>(&input_file_buffer[0]));    
    byte_interpreter_strings.at(9) = std::format("double  {}", *reinterpret_cast<double*>(&input_file_buffer[0]));    

    std::vector<Element> data_interpreter_view;
    for (auto& str : byte_interpreter_strings) {
        data_interpreter_view.push_back(dynamictext(str));
    }

    // Create Modal Layers
    auto hex_editor_renderer = Renderer([&] {
    return window(
        text("Hex Editor") | hcenter,
        hex_editor_view->Render() | hcenter) | size(WIDTH, EQUAL, 12) | size(HEIGHT, EQUAL, 3);
    });

    auto hex_viewer_renderer = Renderer([&] {
        return vbox({
            text(std::to_string(global_position) + ":" + std::to_string(yloc) + " V: " + std::to_string(num_viewable_rows)),
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
        }

        return current_view | size(WIDTH, EQUAL, 107);
    });

    main_window_renderer |= CatchEvent([&](Event event) {

        if (event == Event::Escape) { modalDepth = 0; }

        if (modalDepth == 1) { return hex_editor_view->OnEvent(event); }

        if (event.is_character()) {
            const char c = event.character().at(0);

            view.at(cursor_y)->GetChildren().at(cursor_x * 2) |= inverted;

            switch (c) {
                case 'l':
                    if (xloc < 15) {
                        xloc++;
                        cursor_x++;
                    }
                    break;
                case 'h':
                    if (xloc > 0) {
                        xloc--;
                        cursor_x--;
                    }
                    break;
                case 'j':
                    if (yloc < max_rows - 1) {
                        yloc++;
                        if (cursor_y < num_viewable_rows - 1) {
                            cursor_y++;
                        } else {
                            scrollScreen(yloc, xloc, cursor_y);
                        }
                    }
                    break;
                case 'k':
                    if (yloc > 0) {
                        yloc--;
                        if (cursor_y > 0) {
                            cursor_y--;
                        } else {
                            scrollScreen(yloc, xloc, cursor_y);
                        }
                    }
                    break;
                case 'r':
                    modalDepth = 1;
                    answer = "";
                    break;
                default:
                    break;
            }

            view.at(cursor_y)->GetChildren().at(cursor_x * 2) |= inverted;
            updateByteInterpreter(yloc, xloc);
        }

        return true;
    });

    Loop loop(&screen, main_window_renderer);
    while (!loop.HasQuitted()) {
        num_viewable_rows = get_viewable_text_rows(screen);
        loop.RunOnce();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// Copyright 2020 Arthur Sonzogni. All rights reserved.
// Use of this source code is governed by the MIT license that can be found in
// the LICENSE file.