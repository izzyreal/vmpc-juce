#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include <vector>
#include <string>

#include "VmpcJuceResourceUtil.hpp"

#include "nlohmann/json.hpp"

namespace vmpc_juce::gui::vector {

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
        std::string hardware_label;
        std::string svg_placement;
        float font_scale;
        std::string font;

        // Should only be specified for the root node.
        // Throughout the UI, a getScale() is available. This function
        // returns the current root window height divided by the
        // base height specified below. Many aspects of the UI respond
        // to scale, such as line thickness and font size. So when you
        // define the root node for a new layout, play with values
        // between 300 to 1000 to affect such global aesthetics.
        // 
        // The root window's aspect ratio is base_width / base_height.
        int base_width;
        int base_height;

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

    static void from_json(const nlohmann::json& j, node& n)
    {
        if (j.contains("include"))
        {
            const auto jsonFileData = vmpc_juce::VmpcJuceResourceUtil::getResourceData("json/" + j.at("include").get<std::string>() + ".json");

            nlohmann::json data = nlohmann::json::parse(jsonFileData);
            n = data.template get<node>();

            if (j.contains("area")) j.at("area").get_to(n.area);

            if (j.contains("margin"))
            {
                if (j.at("margin").is_array())
                {
                    j.at("margin").get_to(n.margins);
                    n.margin = 0.f;
                }
                else
                {
                    j.at("margin").get_to(n.margin);
                }
            }
            else
            {
                n.margin = 0.f;
            }
        }
        else
        {
            if (j.contains("margin"))
            {
                if (j.at("margin").is_array())
                {
                    j.at("margin").get_to(n.margins);
                    n.margin = 0.f;
                }
                else
                {
                    j.at("margin").get_to(n.margin);
                }
            }
            else
            {
                n.margin = 0.f;
            }

            if (j.contains("name"))             j.at("name").get_to(n.name);
            if (j.contains("type"))             j.at("type").get_to(n.node_type);
            if (j.contains("svg"))              j.at("svg").get_to(n.svg);
            if (j.contains("children"))         j.at("children").get_to(n.children);
            if (j.contains("label"))            j.at("label").get_to(n.label);
            if (j.contains("label_style"))      j.at("label_style").get_to(n.label_style);
            if (j.contains("direction"))        j.at("direction").get_to(n.direction);
            if (j.contains("flex_grow"))        j.at("flex_grow").get_to(n.flex_grow); else n.flex_grow = 0.f;
            if (j.contains("align_items"))      j.at("align_items").get_to(n.align_items);
            if (j.contains("row_fractions"))    j.at("row_fractions").get_to(n.row_fractions);
            if (j.contains("column_fractions")) j.at("column_fractions").get_to(n.column_fractions);
            if (j.contains("area"))             j.at("area").get_to(n.area);
            if (j.contains("justify_items"))    j.at("justify_items").get_to(n.justify_items);
            if (j.contains("width"))            j.at("width").get_to(n.width);
            if (j.contains("hide_svg"))         j.at("hide_svg").get_to(n.hide_svg); else n.hide_svg = false;
            if (j.contains("is_inner_shadow"))  j.at("is_inner_shadow").get_to(n.is_inner_shadow); else n.is_inner_shadow = false;
            if (j.contains("magic_multiplier")) j.at("magic_multiplier").get_to(n.magic_multiplier); else n.magic_multiplier = 0.f;
            if (j.contains("hardware_label"))   j.at("hardware_label").get_to(n.hardware_label);
            if (j.contains("svg_placement"))    j.at("svg_placement").get_to(n.svg_placement);
            if (j.contains("font_scale"))       j.at("font_scale").get_to(n.font_scale); else n.font_scale = 0.f;
            if (j.contains("font"))             j.at("font").get_to(n.font);
            if (j.contains("base_width"))       j.at("base_width").get_to(n.base_width); else n.base_width = 0.f;
            if (j.contains("base_height"))      j.at("base_height").get_to(n.base_height); else n.base_height = 0.f;

            if (j.contains("shadow_size"))
            {
                j.at("shadow_size").get_to(n.shadow_size);

                if (n.shadow_size > 0.f)
                {
                    for (auto &c : n.children)
                    {
                        c.shadow_size = n.shadow_size;
                    }
                }
            }
            else
            {
                n.shadow_size = 0.f;
            }

            if (j.contains("shadow_darkness"))
            {
                j.at("shadow_darkness").get_to(n.shadow_darkness);

                if (n.shadow_darkness > 0.f)
                {
                    for (auto &c : n.children)
                    {
                        c.shadow_darkness = n.shadow_darkness;
                    }
                }
            }
            else
            {
                n.shadow_darkness = 0.f;
            }

            if (j.contains("label_text_to_calculate_width"))
            {
                j.at("label_text_to_calculate_width").get_to(n.label_text_to_calculate_width);
            }
            else if (j.contains("label"))
            {
                j.at("label").get_to(n.label_text_to_calculate_width);
            }
        }


        if (!n.area.empty())
        {
            // JUCE's withArea(rowStart, columnStart, rowEnd, columnEnd) API expects exclusive
            // rowEnd and columnEnd arguments. I find that unintuitive, so in the layouts I
            // specify inclusive end arguments, and do a fix-up here.
            n.area[2] += 1;
            n.area[3] += 1;
        }

#if DEBUG_NODES == 1
        printf("=== Deserialized node ===\n");
        if (!n.name.empty())        printf("-        name: %s\n", n.name.c_str());
        if (!n.svg.empty())         printf("-         svg: %s\n", n.svg.c_str());
        if (!n.children.empty())    printf("-    children: %zu\n", n.children.size());
        if (n.margin != 0.f)        printf("-      margin: %f\n", n.margin);
        if (!n.label.empty())       printf("-       label: %s\n", n.label.c_str());
        if (!n.label_style.empty()) printf("- label_style: %s\n", n.label_style.c_str());
        if (n.flex_grow != 0.f)     printf("-   flex_grow: %f\n", n.flex_grow);

        if (!n.row_fractions.empty())
        {
            printf("- row_fractions:");
            for (auto& f : n.row_fractions)
                printf(" %i ", f);
            printf("\n");
        }

        if (!n.area.empty())
        {
            printf("-          area:");
            for (auto& f : n.area)
                printf(" %i ", f);
            printf("\n");
        }

        if (!n.margins.empty())
        {
            printf("-       margins:");
            for (auto& f : n.margins)
                printf(" %f ", f);
            printf("\n");
        }

        printf("=========================\n");
#endif
    }

} // namespace vmpc_juce::gui::vector
