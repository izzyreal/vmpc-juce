#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace vmpc_juce::gui::vector
{

class  VmpcTextLayout  final
{
private:
    template <typename Iterator>
    class DereferencingIterator
    {
    public:
        using value_type = std::remove_reference_t<decltype(**std::declval<Iterator>())>;
        using difference_type = typename std::iterator_traits<Iterator>::difference_type;
        using pointer = value_type*;
        using reference = value_type&;
        using iterator_category = typename std::iterator_traits<Iterator>::iterator_category;

        explicit DereferencingIterator (Iterator in) : iterator (std::move (in)) {}

        DereferencingIterator& operator+= (difference_type distance)
        {
            iterator += distance;
            return *this;
        }

        friend DereferencingIterator operator+ (DereferencingIterator i, difference_type d) { return i += d; }
        friend DereferencingIterator operator+ (difference_type d, DereferencingIterator i) { return i += d; }

        DereferencingIterator& operator-= (difference_type distance)
        {
            iterator -= distance;
            return *this;
        }

        friend DereferencingIterator operator- (DereferencingIterator i, difference_type d) { return i -= d; }

        friend difference_type operator- (DereferencingIterator a, DereferencingIterator b)   { return a.iterator - b.iterator; }

        reference operator[] (difference_type d) const { return *iterator[d]; }

        friend bool operator<  (DereferencingIterator a, DereferencingIterator b) { return a.iterator <  b.iterator; }
        friend bool operator<= (DereferencingIterator a, DereferencingIterator b) { return a.iterator <= b.iterator; }
        friend bool operator>  (DereferencingIterator a, DereferencingIterator b) { return a.iterator >  b.iterator; }
        friend bool operator>= (DereferencingIterator a, DereferencingIterator b) { return a.iterator >= b.iterator; }
        friend bool operator== (DereferencingIterator a, DereferencingIterator b) { return a.iterator == b.iterator; }
        friend bool operator!= (DereferencingIterator a, DereferencingIterator b) { return a.iterator != b.iterator; }

        DereferencingIterator& operator++()           { ++iterator; return *this; }
        DereferencingIterator& operator--()           { --iterator; return *this; }
        DereferencingIterator  operator++ (int) const { DereferencingIterator copy (*this); ++(*this); return copy; }
        DereferencingIterator  operator-- (int) const { DereferencingIterator copy (*this); --(*this); return copy; }

        reference operator* () const { return **iterator; }
        pointer   operator->() const { return  *iterator; }

    private:
        Iterator iterator;
    };

public:
    VmpcTextLayout();
    VmpcTextLayout (const VmpcTextLayout&);
    VmpcTextLayout& operator= (const VmpcTextLayout&);
    VmpcTextLayout (VmpcTextLayout&&) noexcept;
    VmpcTextLayout& operator= (VmpcTextLayout&&) noexcept;

    ~VmpcTextLayout();

    void createLayout (const juce::AttributedString&, float maxWidth);

    void createLayout (const juce::AttributedString&, float maxWidth, float maxHeight);

    void createLayoutWithBalancedLineLengths (const juce::AttributedString&, float maxWidth);

    void createLayoutWithBalancedLineLengths (const juce::AttributedString&, float maxWidth, float maxHeight);

    void draw (juce::Graphics&, juce::Rectangle<float> area) const;

    class Glyph
    {
    public:
        Glyph (int glyphCode, juce::Point<float> anchor, float width) noexcept;

        int glyphCode;

        juce::Point<float> anchor;

        float width;

    private:
        JUCE_LEAK_DETECTOR (Glyph)
    };

    class Run
    {
    public:
        Run() = default;
        Run (juce::Range<int> stringRange, int numGlyphsToPreallocate);

        juce::Range<float> getRunBoundsX() const noexcept;

        juce::Font font;
        juce::Colour colour { 0xff000000 };
        juce::Array<Glyph> glyphs;
        juce::Range<int> stringRange;
    private:
        JUCE_LEAK_DETECTOR (Run)
    };

    class Line
    {
    public:
        Line() = default;
        Line (juce::Range<int> stringRange, juce::Point<float> lineOrigin,
              float ascent, float descent, float leading, int numRunsToPreallocate);

        Line (const Line&);
        Line& operator= (const Line&);

        Line (Line&&) noexcept = default;
        Line& operator= (Line&&) noexcept = default;

        ~Line() noexcept = default;

        juce::Range<float> getLineBoundsX() const noexcept;

        juce::Range<float> getLineBoundsY() const noexcept;

        juce::Rectangle<float> getLineBounds() const noexcept;

        void swap (Line& other) noexcept;

        juce::OwnedArray<Run> runs;
        juce::Range<int> stringRange;
        juce::Point<float> lineOrigin;
        float ascent = 0.0f, descent = 0.0f, leading = 0.0f;

    private:
        JUCE_LEAK_DETECTOR (Line)
    };

    float getWidth() const noexcept     { return width; }

    float getHeight() const noexcept    { return height; }

    int getNumLines() const noexcept    { return lines.size(); }

    Line& getLine (int index) const noexcept;

    void addLine (std::unique_ptr<Line>);

    void ensureStorageAllocated (int numLinesNeeded);

    using       iterator = DereferencingIterator<      Line* const*>;
    using const_iterator = DereferencingIterator<const Line* const*>;

          iterator  begin()       { return       iterator (lines.begin()); }
    const_iterator  begin() const { return const_iterator (lines.begin()); }
    const_iterator cbegin() const { return const_iterator (lines.begin()); }

          iterator  end()       { return       iterator (lines.end()); }
    const_iterator  end() const { return const_iterator (lines.end()); }
    const_iterator cend() const { return const_iterator (lines.end()); }

    void recalculateSize();

private:
    juce::OwnedArray<Line> lines;
    float width, height;
    juce::Justification justification;

    void createStandardLayout (const juce::AttributedString&);
    bool createNativeLayout (const juce::AttributedString&);

    JUCE_LEAK_DETECTOR (VmpcTextLayout)
};

} // namespace vmpc_juce::gui::vector
