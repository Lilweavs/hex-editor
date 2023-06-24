#include <cstdint>
#include <cstdio>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/node.hpp>
#include "ftxui/dom/elements.hpp"  // for operator|, separator, text, Element, flex, vbox, border
#include <ftxui/screen/color.hpp>
#include <ftxui/screen/screen.hpp>
#include "ftxui/component/loop.hpp"
#include "ftxui/component/component.hpp"  // for Input, Renderer, ResizableSplitLeft
#include "ftxui/component/screen_interactive.hpp"  // for ScreenInteractive
#include <ftxui/screen/terminal.hpp>
#include <memory>  // for allocator, __shared_ptr_access, shared_ptr
#include <string>  // for string
#include <thread>
#include <fstream> 
#include <format>


#define MAX_ROWS 12
#define X_OFFSET 1

class DynamicText: public ftxui::Node {
public:
  explicit DynamicText(std::string& text) {
		text_ = &text;
	}
  void ComputeRequirement() override {
    requirement_.min_x = ftxui::string_width(*text_);
    requirement_.min_y = 1;
  }

  void Render(ftxui::Screen& screen) override {
    int x = box_.x_min;
    const int y = box_.y_min;
    if (y > box_.y_max) {
      return;
    }
    for (const auto& cell : ftxui::Utf8ToGlyphs(*text_)) {
      if (x > box_.x_max) {
        return;
      }
      screen.PixelAt(x, y).character = cell;
      ++x;
    }
  }

private:
	std::string* text_;
};


ftxui::Element dynamictext(std::string& text) {
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

std::vector<std::vector<ftxui::Element*>> references;

int get_viewable_text_rows(ftxui::Screen& screen) {
	return screen.dimy() - 4;	
}

void scrollScreen(int row, int col, int cursor_y) {

	int start = 0, end = 0;

	if (cursor_y == 0) {
		start = row;
		end   = row + num_viewable_rows;
	} else {
		start = row - (num_viewable_rows - 1);
		end   = row+1;
	}
	
	char tmp[17] = {0};
	for (int i = start; i < end; i++) {
		row_strings.at(i-start) = std::format("{:04X}", i*16);
		for (int j = 0; j < 16; j++) {
			char val = input_file_buffer[i*16+j];
			hex_strings.at(i-start).at(j) = std::format("{:02X}",static_cast<uint8_t>(val));

			tmp[j] = (val >= ' ' && val < '~') ? val : '.';		
		}
		ascii_strings.at(i-start) = std::string(tmp);
	}
}

int main() {
  using namespace ftxui;
	
	// TODO: Move to non-vector type to gaurantee no reallocations
	// This will cause the string pointers in dynamictext to point to bad memory locations
	ascii_strings.reserve(100);
	row_strings.reserve(100);

	std::ifstream file("your_file_here", std::ios::binary);
	file.read(input_file_buffer, BUFFER_SIZE);

  auto screen = ScreenInteractive::Fullscreen();
	auto screen_dim = Terminal::Size();
	num_viewable_rows = screen_dim.dimy - 4;

	
	Elements rowByteIndex;
	for (int i = 0; i < num_viewable_rows; i++) {
		row_strings.push_back(std::format("{:04X}", i*16));
		rowByteIndex.push_back(dynamictext(row_strings.back()) | dim);
	}
	
	Elements rowByteAscii;
	for (int i = 0; i < num_viewable_rows; i++) {
		char tmp[17] = {0};
		for (int j = 0; j < 16; j++) {
			char val = input_file_buffer[i*16+j];

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
			tmpStrings.push_back(std::format("{:02X}", static_cast<uint8_t>(input_file_buffer[i*16+j])));
		}
		hex_strings.push_back(tmpStrings);	
	} 

	for (int i = 0; i < num_viewable_rows; i++) {
		Elements bytes;
		std::vector<Element*> ref;
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
		references.push_back(ref);
		view.push_back(hbox(bytes));
	}

	std::string answer = "";
	// Create Input Box for hex viewer
	Component hex_editor_view = Input(&answer, "0x00");

	// Create Modal Layers
	auto hex_editor_renderer = Renderer([&] {
		return window(text("Hex Editor") | hcenter,
			hex_editor_view->Render() | hcenter
		) | size(WIDTH, EQUAL, 12) | size(HEIGHT, EQUAL, 3);
	});

	auto hex_viewer_renderer = Renderer([&] {
		return vbox({
			text(std::to_string(global_position) + ":" + std::to_string(yloc) + " V: " + std::to_string(num_viewable_rows)), 
			separator(),
			hbox({
				vbox(rowByteIndex) | size(WIDTH, EQUAL, 4),
				separator(),
				vbox(view) | size(WIDTH, EQUAL, 3*16-1),
				separator(),
				vflow(rowByteAscii) | size(WIDTH, EQUAL, 16)
			})
		}) | border;
	});

  auto main_window_renderer = Renderer([&] {
		Element current_view = hex_viewer_renderer->Render();
		
		if (modalDepth == 1) {
			current_view = dbox({
				current_view,
				hex_editor_renderer->Render() | center
			});			
		}
		
		return current_view | size(WIDTH, EQUAL, 72);
  });

	main_window_renderer |= CatchEvent([&] (Event event) {

		if (event == Event::Escape) { modalDepth = 0; }

		if (modalDepth == 1) {
			return hex_editor_view->OnEvent(event);
		}
		
		if (event.is_character()) {
			const char c = event.character().at(0);

			view.at(cursor_y)->getChildren().at(cursor_x*2) |= inverted;
			
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
					// if (((yloc+1) % num_viewable_rows != 0) && yloc < max_rows - 2) {
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

			view.at(cursor_y)->getChildren().at(cursor_x*2) |= inverted;
		
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