#include <juce_gui_basics/juce_gui_basics.h>

#include "Node.hpp"

class FlexBoxWrapper : public juce::Component {
    public:
        FlexBoxWrapper(node &n, const std::function<float()>& getScale);
        ~FlexBoxWrapper() override;

        void resized() override;

        std::vector<juce::Component*> components;

    private:
        node& node;
        const std::function<float()> getScale;
};
