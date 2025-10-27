#include <juce_gui_basics/juce_gui_basics.h>

#include "Node.hpp"

#include <vector>

namespace vmpc_juce::gui::vector
{

    class SvgWithLabelGrid : public juce::Component
    {
    public:
        SvgWithLabelGrid(const node &n, const std::function<float()> &getScale);
        ~SvgWithLabelGrid() override;

        void resized() override;

        std::vector<juce::Component *> components;

    private:
        const node &myNode;
        const std::function<float()> &getScale;
    };

} // namespace vmpc_juce::gui::vector
