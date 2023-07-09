#include <ftxui/dom/node.hpp>
#include <ftxui/screen/string.hpp>

#ifndef DYNAMICTEXT_H
#define DYNAMICTEXT_H


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

#endif // DYNAMICTEXT_H