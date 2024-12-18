#include "VmpcTextLayout.hpp"

namespace vmpc_juce::gui::vector
{

static juce::String substring (const juce::String& text, juce::Range<int> range)
{
    return text.substring (range.getStart(), range.getEnd());
}

VmpcTextLayout::Glyph::Glyph (int glyph, juce::Point<float> anch, float w) noexcept
    : glyphCode (glyph), anchor (anch), width (w)
{
}

VmpcTextLayout::Run::Run (juce::Range<int> range, int numGlyphsToPreallocate)
    : stringRange (range)
{
    glyphs.ensureStorageAllocated (numGlyphsToPreallocate);
}

juce::Range<float> VmpcTextLayout::Run::getRunBoundsX() const noexcept
{
    juce::Range<float> range;
    bool isFirst = true;

    for (auto& glyph : glyphs)
    {
        juce::Range<float> r (glyph.anchor.x, glyph.anchor.x + glyph.width);

        if (isFirst)
        {
            isFirst = false;
            range = r;
        }
        else
        {
            range = range.getUnionWith (r);
        }
    }

    return range;
}

//==============================================================================
VmpcTextLayout::Line::Line (juce::Range<int> range, juce::Point<float> o, float asc, float desc,
                        float lead, int numRunsToPreallocate)
    : stringRange (range), lineOrigin (o),
      ascent (asc), descent (desc), leading (lead)
{
    runs.ensureStorageAllocated (numRunsToPreallocate);
}

VmpcTextLayout::Line::Line (const Line& other)
    : stringRange (other.stringRange), lineOrigin (other.lineOrigin),
      ascent (other.ascent), descent (other.descent), leading (other.leading)
{
    runs.addCopiesOf (other.runs);
}

VmpcTextLayout::Line& VmpcTextLayout::Line::operator= (const Line& other)
{
    auto copy = other;
    swap (copy);
    return *this;
}

juce::Range<float> VmpcTextLayout::Line::getLineBoundsX() const noexcept
{
    juce::Range<float> range;
    bool isFirst = true;

    for (auto* run : runs)
    {
        auto runRange = run->getRunBoundsX();

        if (isFirst)
        {
            isFirst = false;
            range = runRange;
        }
        else
        {
            range = range.getUnionWith (runRange);
        }
    }

    return range + lineOrigin.x;
}

juce::Range<float> VmpcTextLayout::Line::getLineBoundsY() const noexcept
{
    return { lineOrigin.y - ascent,
             lineOrigin.y + descent };
}

juce::Rectangle<float> VmpcTextLayout::Line::getLineBounds() const noexcept
{
    auto x = getLineBoundsX();
    auto y = getLineBoundsY();

    return { x.getStart(), y.getStart(), x.getLength(), y.getLength() };
}

void VmpcTextLayout::Line::swap (Line& other) noexcept
{
    std::swap (other.runs,          runs);
    std::swap (other.stringRange,   stringRange);
    std::swap (other.lineOrigin,    lineOrigin);
    std::swap (other.ascent,        ascent);
    std::swap (other.descent,       descent);
    std::swap (other.leading,       leading);
}

VmpcTextLayout::VmpcTextLayout()
    : width (0), height (0), justification (juce::Justification::topLeft)
{
}

VmpcTextLayout::VmpcTextLayout (const VmpcTextLayout& other)
    : width (other.width), height (other.height),
      justification (other.justification)
{
    lines.addCopiesOf (other.lines);
}

VmpcTextLayout::VmpcTextLayout (VmpcTextLayout&& other) noexcept
    : lines (std::move (other.lines)),
      width (other.width), height (other.height),
      justification (other.justification)
{
}

VmpcTextLayout& VmpcTextLayout::operator= (VmpcTextLayout&& other) noexcept
{
    lines = std::move (other.lines);
    width = other.width;
    height = other.height;
    justification = other.justification;
    return *this;
}

VmpcTextLayout& VmpcTextLayout::operator= (const VmpcTextLayout& other)
{
    width = other.width;
    height = other.height;
    justification = other.justification;
    lines.clear();
    lines.addCopiesOf (other.lines);
    return *this;
}

VmpcTextLayout::~VmpcTextLayout()
{
}

VmpcTextLayout::Line& VmpcTextLayout::getLine (int index) const noexcept
{
    return *lines.getUnchecked (index);
}

void VmpcTextLayout::ensureStorageAllocated (int numLinesNeeded)
{
    lines.ensureStorageAllocated (numLinesNeeded);
}

void VmpcTextLayout::addLine (std::unique_ptr<Line> line)
{
    lines.add (line.release());
}

void VmpcTextLayout::draw (juce::Graphics& g, juce::Rectangle<float> area) const
{
    auto origin = justification.appliedToRectangle (juce::Rectangle<float> (width, getHeight()), area).getPosition();

    auto& context   = g.getInternalContext();
    context.saveState();

    auto clip       = context.getClipBounds();
    auto clipTop    = (float) clip.getY()      - origin.y;
    auto clipBottom = (float) clip.getBottom() - origin.y;

    for (auto& line : *this)
    {
        auto lineRangeY = line.getLineBoundsY();

        if (lineRangeY.getEnd() < clipTop)
            continue;

        if (lineRangeY.getStart() > clipBottom)
            break;

        auto lineOrigin = origin + line.lineOrigin;

        for (auto* run : line.runs)
        {
            context.setFont (run->font);
            context.setFill (run->colour);

            for (auto& glyph : run->glyphs)
                context.drawGlyph (glyph.glyphCode, juce::AffineTransform::translation (lineOrigin.x + glyph.anchor.x,
                                                                                  lineOrigin.y + glyph.anchor.y));

            if (run->font.isUnderlined())
            {
                auto runExtent = run->getRunBoundsX();
                auto lineThickness = run->font.getDescent() * 0.3f;

                context.fillRect ({ runExtent.getStart() + lineOrigin.x, lineOrigin.y + lineThickness * 2.0f,
                                    runExtent.getLength(), lineThickness });
            }
        }
    }

    context.restoreState();
}

void VmpcTextLayout::createLayout (const juce::AttributedString& text, float maxWidth)
{
    createLayout (text, maxWidth, 1.0e7f);
}

void VmpcTextLayout::createLayout (const juce::AttributedString& text, float maxWidth, float maxHeight)
{
    lines.clear();
    width = maxWidth;
    height = maxHeight;
    justification = text.getJustification();

    createStandardLayout (text);

    recalculateSize();
}

void VmpcTextLayout::createLayoutWithBalancedLineLengths (const juce::AttributedString& text, float maxWidth)
{
    createLayoutWithBalancedLineLengths (text, maxWidth, 1.0e7f);
}

void VmpcTextLayout::createLayoutWithBalancedLineLengths (const juce::AttributedString& text, float maxWidth, float maxHeight)
{
    auto minimumWidth = maxWidth / 2.0f;
    auto bestWidth = maxWidth;
    float bestLineProportion = 0.0f;

    while (maxWidth > minimumWidth)
    {
        createLayout (text, maxWidth, maxHeight);

        if (getNumLines() < 2)
            return;

        auto line1 = lines.getUnchecked (lines.size() - 1)->getLineBoundsX().getLength();
        auto line2 = lines.getUnchecked (lines.size() - 2)->getLineBoundsX().getLength();
        auto shortest = juce::jmin (line1, line2);
        auto longest  = juce::jmax (line1, line2);
        auto prop = shortest > 0 ? longest / shortest : 1.0f;

        if (prop > 0.9f && prop < 1.1f)
            return;

        if (prop > bestLineProportion)
        {
            bestLineProportion = prop;
            bestWidth = maxWidth;
        }

        maxWidth -= 10.0f;
    }

    if (! juce::approximatelyEqual (bestWidth, maxWidth))
        createLayout (text, bestWidth, maxHeight);
}

//==============================================================================
namespace VmpcTextLayoutHelpers
{
    struct Token
    {
        Token (const juce::String& t, const juce::Font& f, juce::Colour c, bool whitespace)
            : text (t), font (f), colour (c),
              area (font.getStringWidthFloat (t), f.getHeight()),
              isWhitespace (whitespace),
              isNewLine (t.containsChar ('\n') || t.containsChar ('\r'))
        {}

        const juce::String text;
        const juce::Font font;
        const juce::Colour colour;
        juce::Rectangle<float> area;
        int line;
        float lineHeight;
        const bool isWhitespace, isNewLine;

        Token& operator= (const Token&) = delete;
    };

    struct TokenList
    {
        TokenList() noexcept {}

        void createLayout (const juce::AttributedString& text, VmpcTextLayout& layout)
        {
            layout.ensureStorageAllocated (totalLines);

            addTextRuns (text);
            layoutRuns (layout.getWidth(), text.getLineSpacing(), text.getWordWrap());

            int charPosition = 0;
            int lineStartPosition = 0;
            int runStartPosition = 0;

            std::unique_ptr<VmpcTextLayout::Line> currentLine;
            std::unique_ptr<VmpcTextLayout::Run> currentRun;

            bool needToSetLineOrigin = true;

            for (int i = 0; i < tokens.size(); ++i)
            {
                auto& t = *tokens.getUnchecked (i);

                juce::Array<int> newGlyphs;
                juce::Array<float> xOffsets;
                t.font.getGlyphPositions (getTrimmedEndIfNotAllWhitespace (t.text), newGlyphs, xOffsets);

                if (currentRun == nullptr)  currentRun  = std::make_unique<VmpcTextLayout::Run>();
                if (currentLine == nullptr) currentLine = std::make_unique<VmpcTextLayout::Line>();

                const auto numGlyphs = newGlyphs.size();
                charPosition += numGlyphs;

                if (numGlyphs > 0
                    && (! (/*t.isWhitespace || */t.isNewLine) || needToSetLineOrigin))
                {
                    currentRun->glyphs.ensureStorageAllocated (currentRun->glyphs.size() + newGlyphs.size());
                    auto tokenOrigin = t.area.getPosition().translated (0, t.font.getAscent());

                    if (needToSetLineOrigin)
                    {
                        needToSetLineOrigin = false;
                        currentLine->lineOrigin = tokenOrigin;
                    }

                    auto glyphOffset = tokenOrigin - currentLine->lineOrigin;

                    for (int j = 0; j < newGlyphs.size(); ++j)
                    {
                        auto x = xOffsets.getUnchecked (j);
                        currentRun->glyphs.add (VmpcTextLayout::Glyph (newGlyphs.getUnchecked (j),
                                                                   glyphOffset.translated (x, 0),
                                                                   xOffsets.getUnchecked (j + 1) - x));
                    }
                }

                if (auto* nextToken = tokens[i + 1])
                {
                    if (t.font != nextToken->font || t.colour != nextToken->colour)
                    {
                        addRun (*currentLine, currentRun.release(), t, runStartPosition, charPosition);
                        runStartPosition = charPosition;
                    }

                    if (t.line != nextToken->line)
                    {
                        if (currentRun == nullptr)
                            currentRun = std::make_unique<VmpcTextLayout::Run>();

                        addRun (*currentLine, currentRun.release(), t, runStartPosition, charPosition);
                        currentLine->stringRange = { lineStartPosition, charPosition };

                        if (! needToSetLineOrigin)
                            layout.addLine (std::move (currentLine));

                        runStartPosition = charPosition;
                        lineStartPosition = charPosition;
                        needToSetLineOrigin = true;
                    }
                }
                else
                {
                    addRun (*currentLine, currentRun.release(), t, runStartPosition, charPosition);
                    currentLine->stringRange = { lineStartPosition, charPosition };

                    if (! needToSetLineOrigin)
                        layout.addLine (std::move (currentLine));

                    needToSetLineOrigin = true;
                }
            }

            if ((text.getJustification().getFlags() & (juce::Justification::right | juce::Justification::horizontallyCentred)) != 0)
            {
                auto totalW = layout.getWidth();
                bool isCentred = (text.getJustification().getFlags() & juce::Justification::horizontallyCentred) != 0;

                for (auto& line : layout)
                {
                    auto dx = totalW - line.getLineBoundsX().getLength();

                    if (isCentred)
                        dx /= 2.0f;

                    line.lineOrigin.x += dx;
                }
            }
        }

    private:
        static void addRun (VmpcTextLayout::Line& glyphLine, VmpcTextLayout::Run* glyphRun,
                            const Token& t, int start, int end)
        {
            glyphRun->stringRange = { start, end };
            glyphRun->font = t.font;
            glyphRun->colour = t.colour;
            glyphLine.ascent  = juce::jmax (glyphLine.ascent,  t.font.getAscent());
            glyphLine.descent = juce::jmax (glyphLine.descent, t.font.getDescent());
            glyphLine.runs.add (glyphRun);
        }

        static int getCharacterType (juce::juce_wchar c) noexcept
        {
            if (c == '\r' || c == '\n')
                return 0;

            return juce::CharacterFunctions::isWhitespace (c) ? 2 : 1;
        }

        void appendText (const juce::String& stringText, const juce::Font& font, juce::Colour colour)
        {
            auto t = stringText.getCharPointer();
            juce::String currentString;
            int lastCharType = 0;

            for (;;)
            {
                auto c = t.getAndAdvance();

                if (c == 0)
                    break;

                auto charType = getCharacterType (c);

                if (charType == 0 || charType != lastCharType)
                {
                    if (currentString.isNotEmpty())
                        tokens.add (new Token (currentString, font, colour,
                                               lastCharType == 2 || lastCharType == 0));

                    currentString = juce::String::charToString (c);

                    if (c == '\r' && *t == '\n')
                        currentString += t.getAndAdvance();
                }
                else
                {
                    currentString += c;
                }

                lastCharType = charType;
            }

            if (currentString.isNotEmpty())
                tokens.add (new Token (currentString, font, colour, lastCharType == 2));
        }

        void layoutRuns (float maxWidth, float extraLineSpacing, juce::AttributedString::WordWrap wordWrap)
        {
            float x = 0, y = 0, h = 0;
            int i;

            for (i = 0; i < tokens.size(); ++i)
            {
                auto& t = *tokens.getUnchecked (i);
                t.area.setPosition (x, y);
                t.line = totalLines;
                x += t.area.getWidth();
                h = juce::jmax (h, t.area.getHeight() + extraLineSpacing);

                auto* nextTok = tokens[i + 1];

                if (nextTok == nullptr)
                    break;

                bool tokenTooLarge = (x + nextTok->area.getWidth() > maxWidth);

                if (t.isNewLine || ((! nextTok->isWhitespace) && (tokenTooLarge && wordWrap != juce::AttributedString::none)))
                {
                    setLastLineHeight (i + 1, h);
                    x = 0;
                    y += h;
                    h = 0;
                    ++totalLines;
                }
            }

            setLastLineHeight (juce::jmin (i + 1, tokens.size()), h);
            ++totalLines;
        }

        void setLastLineHeight (int i, float height) noexcept
        {
            while (--i >= 0)
            {
                auto& tok = *tokens.getUnchecked (i);

                if (tok.line == totalLines)
                    tok.lineHeight = height;
                else
                    break;
            }
        }

        void addTextRuns (const juce::AttributedString& text)
        {
            auto numAttributes = text.getNumAttributes();
            tokens.ensureStorageAllocated (juce::jmax (64, numAttributes));

            for (int i = 0; i < numAttributes; ++i)
            {
                auto& attr = text.getAttribute (i);

                appendText (substring (text.getText(), attr.range),
                            attr.font, attr.colour);
            }
        }

        static juce::String getTrimmedEndIfNotAllWhitespace (const juce::String& s)
        {
            auto trimmed = s.trimEnd();

            if (trimmed.isEmpty() && s.isNotEmpty())
                trimmed = s.replaceCharacters ("\r\n\t", "   ");

            return trimmed;
        }

        juce::OwnedArray<Token> tokens;
        int totalLines = 0;

        JUCE_DECLARE_NON_COPYABLE (TokenList)
    };
}

void VmpcTextLayout::createStandardLayout (const juce::AttributedString& text)
{
    VmpcTextLayoutHelpers::TokenList l;
    l.createLayout (text, *this);
}

void VmpcTextLayout::recalculateSize()
{
    if (! lines.isEmpty())
    {
        auto bounds = lines.getFirst()->getLineBounds();

        for (auto* line : lines)
            bounds = bounds.getUnion (line->getLineBounds());

        for (auto* line : lines)
            line->lineOrigin.x -= bounds.getX();

        width  = bounds.getWidth();
        height = bounds.getHeight();
    }
    else
    {
        width = 0;
        height = 0;
    }
}

} // namespace vmpc_juce::gui::vector
