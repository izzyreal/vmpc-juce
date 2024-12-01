#include <juce_gui_basics/juce_gui_basics.h>

#include "Node.hpp"

class GridWrapper : public juce::Component {
    public:
        GridWrapper(node &n, const std::function<float()> &getScale);
        ~GridWrapper() override;

        void resized() override;

        std::vector<juce::Component*> components;

    private:
        node& node;
        const std::function<float()> &getScale;
};
