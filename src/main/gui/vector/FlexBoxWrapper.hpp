#include <juce_gui_basics/juce_gui_basics.h>

#include "Node.hpp"

namespace vmpc_juce::gui::vector
{

    class FlexBoxWrapper : public juce::Component
    {
    public:
        FlexBoxWrapper(node &n, const std::function<float()> &getScale);
        ~FlexBoxWrapper() override;

        void resized() override;

        std::vector<juce::Component *> components;

    private:
        node &myNode;
        const std::function<float()> getScale;
    };

} // namespace vmpc_juce::gui::vector
