#include <juce_gui_basics/juce_gui_basics.h>

#include "Node.hpp"

namespace vmpc_juce::gui::vector
{

    class GridWrapper : public juce::Component
    {
    public:
        GridWrapper(node &n, const std::function<float()> &getScale);
        ~GridWrapper() override;

        void resized() override;

        std::vector<juce::Component *> components;

    private:
        node &myNode;
        const std::function<float()> &getScale;
    };

} // namespace vmpc_juce::gui::vector
