#include <juce_gui_basics/juce_gui_basics.h>

#include "Node.hpp"

#include <vector>

class SvgWithLabelGrid : public juce::Component {
    public:
        SvgWithLabelGrid(const node &n, const std::function<float()>& getScale);
        ~SvgWithLabelGrid() override;

        void resized() override;

        std::vector<juce::Component*> components;

    private:
        const node &node;
        const std::function<float()>& getScale;
};
