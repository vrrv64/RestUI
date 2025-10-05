#ifndef RESPONSEHIGHLIGHTER_H
#define RESPONSEHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QRegularExpression>

class ResponseHighlighter : public QSyntaxHighlighter {
    Q_OBJECT
public:
    ResponseHighlighter(QTextDocument *parent, const QString &contentType);
protected:
    void highlightBlock(const QString &text) override;
private:
    enum Format { JSON, XML, HTML, CSS, JavaScript, Plain };
    Format format;
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;
    QTextCharFormat keywordFormat;
    QTextCharFormat stringFormat;
    QTextCharFormat numberFormat;
    QTextCharFormat commentFormat;
    QTextCharFormat tagFormat;
    QTextCharFormat attributeFormat;
};

#endif // RESPONSEHIGHLIGHTER_H
