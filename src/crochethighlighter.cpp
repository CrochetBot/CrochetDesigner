/*************************************************\
| (c) 2011 Stitch Works Software                  |
| Brian C. Milco <brian@stitchworkssoftware.com>  |
\*************************************************/
#include "crochethighlighter.h"

#include "stitchcollection.h"
#include "stitchset.h"

#include <QDebug>

CrochetHighlighter::CrochetHighlighter(QTextDocument *parent)
: QSyntaxHighlighter(parent)
{
    HighlightingRule rule;
    
    keywordFormat.setForeground(Qt::darkBlue);
    keywordFormat.setFontWeight(QFont::Bold);
    QStringList keywordPatterns;

    StitchSet* set = StitchCollection::inst()->masterStitchSet();
    foreach(Stitch* s, set->stitches()) {
        keywordPatterns << "\\b" + s->name() + "(?=[0-9]+|\\b)";
    }
    
    foreach (const QString &pattern, keywordPatterns) {
        rule.pattern = QRegExp(pattern);
        rule.pattern.setMinimal(true);
        rule.pattern.setCaseSensitivity(Qt::CaseInsensitive);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }
    
    repeatFormat.setForeground(Qt::darkBlue);
    repeatFormat.setFontWeight(QFont::Bold);
    repeatStartExp = QRegExp("\\*");
    repeatStartExp.setCaseSensitivity(Qt::CaseInsensitive);
    repeatEndExp = QRegExp("; repeat from \\*");
    repeatEndExp.setCaseSensitivity(Qt::CaseInsensitive);

    repeat2Start = QRegExp("\\[");
    repeat2Start.setCaseSensitivity(Qt::CaseInsensitive);
    repeat2End = QRegExp("\\] [0-9]+ times");
    repeat2End.setCaseSensitivity(Qt::CaseInsensitive);
    
}

CrochetHighlighter::~CrochetHighlighter()
{
}

void CrochetHighlighter::resetRules()
{

}

void CrochetHighlighter::highlightBlock(const QString &text)
{
    foreach (const HighlightingRule &rule, highlightingRules) {
        QRegExp expression(rule.pattern);
        int index = expression.indexIn(text);
        while (index >= 0) {
            int length = expression.matchedLength();
            setFormat(index, length, rule.format);
            index = expression.indexIn(text, index + length);
        }
    }

    setCurrentBlockState(0);

    //find \* .*; repeat from \*
    int startIndex = 0;
    if (previousBlockState() != 1)
        startIndex = repeatStartExp.indexIn(text);

    while (startIndex >= 0) {
        int endIndex = repeatEndExp.indexIn(text, startIndex);
        int commentLength;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + repeatEndExp.matchedLength();
        }

        setFormat(startIndex, repeatStartExp.matchedLength(), repeatFormat);
        setFormat(endIndex, repeatEndExp.matchedLength(), repeatFormat);
        startIndex = repeatStartExp.indexIn(text, startIndex + commentLength);
    }

    //find [.*] X times
    if (previousBlockState() != 2)
        startIndex = repeat2Start.indexIn(text);

    while (startIndex >= 0) {
        int endIndex = repeat2End.indexIn(text, startIndex);
        int commentLength;
        if (endIndex == -1) {
            setCurrentBlockState(2);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + repeat2End.matchedLength();
        }

        setFormat(startIndex, repeat2Start.matchedLength(), repeatFormat);
        setFormat(endIndex, repeat2End.matchedLength(), repeatFormat);
        startIndex = repeat2Start.indexIn(text, startIndex + commentLength);
    }
    
}