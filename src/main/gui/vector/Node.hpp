#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include <vector>
#include <string>

struct node {         
    std::string name;
    std::string node_type;
    std::string svg;
    std::vector<node> children;
    float margin;
    std::string label;
    std::string label_style;
    std::string direction;
    float flex_grow;
    std::string align_items;
    std::vector<float> margins;
    std::string justify_items;
    std::string width;
    std::string label_text_to_calculate_width;
    float shadow_size;
    float shadow_darkness;
    bool hide_svg;
    bool is_inner_shadow;
    float magic_multiplier;

    // grid
    std::vector<uint16_t> row_fractions;
    std::vector<uint16_t> column_fractions;

    // grid leaf item
    std::vector<uint8_t> area;

    juce::Component *svg_component;
    juce::Component *label_component;
    juce::Component *grid_wrapper_component;
    juce::Component *flex_box_wrapper_component;
    juce::Component *svg_with_label_grid_component;
    juce::Component *line_flanked_label_component;
    juce::Component *j_or_l_shape_component;
    juce::Component *rectangle_component;
    juce::Component *num_key_component;
    juce::Component *slider_component;
    juce::Component *data_wheel_component;
    juce::Component *lcd_component;
    juce::Component *led_component;
};
