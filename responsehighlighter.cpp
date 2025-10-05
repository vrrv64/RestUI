#include "responsehighlighter.h"
#include <QPalette>
#include <QApplication>

ResponseHighlighter::ResponseHighlighter(QTextDocument *parent, const QString &contentType)
    : QSyntaxHighlighter(parent) {
    QString ct = contentType.toLower();
    if (ct.contains("json")) format = JSON;
    else if (ct.contains("xml")) format = XML;
    else if (ct.contains("html")) format = HTML;
    else if (ct.contains("css")) format = CSS;
    else if (ct.contains("javascript") || ct.contains("js")) format = JavaScript;
    else format = Plain;

    // Detect dark mode based on application palette
    bool isDarkMode = QApplication::palette().color(QPalette::Window).value() < 128;

    if (isDarkMode) {
        // Dark mode colors inspired by GTKSourceView "Cobalt"
        keywordFormat.setForeground(QColor("#ffdd00")); // Yellow for keywords
        keywordFormat.setFontWeight(QFont::Bold);
        stringFormat.setForeground(QColor("#00ff00"));  // Bright green for strings
        numberFormat.setForeground(QColor("#ff00ff"));  // Magenta for numbers
        commentFormat.setForeground(QColor("#0088ff")); // Blue for comments
        tagFormat.setForeground(QColor("#ff8800"));     // Orange for tags
        attributeFormat.setForeground(QColor("#00ffff")); // Cyan for attributes
    } else {
        // Light mode colors inspired by GTKSourceView "Classic"
        keywordFormat.setForeground(QColor("#0000ff")); // Blue for keywords
        keywordFormat.setFontWeight(QFont::Bold);
        stringFormat.setForeground(QColor("#008000"));  // Dark green for strings
        numberFormat.setForeground(QColor("#ff00ff"));  // Magenta for numbers
        commentFormat.setForeground(QColor("#808080")); // Gray for comments
        tagFormat.setForeground(QColor("#800000"));     // Maroon for tags
        attributeFormat.setForeground(QColor("#000080")); // Navy for attributes
    }

    HighlightingRule rule;
    rule.pattern = QRegularExpression("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"");
    rule.format = stringFormat;
    highlightingRules.append(rule);

    rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*\\b");
    rule.format = numberFormat;
    highlightingRules.append(rule);

    if (format == JSON) {
        QStringList jsonKeywords = {"true", "false", "null"};
        for (const QString &keyword : jsonKeywords) {
            rule.pattern = QRegularExpression("\\b" + keyword + "\\b");
            rule.format = keywordFormat;
            highlightingRules.append(rule);
        }
    } else if (format == XML || format == HTML) {
        rule.pattern = QRegularExpression("<[^>]+>");
        rule.format = tagFormat;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\\b\\w+\\s*=");
        rule.format = attributeFormat;
        highlightingRules.append(rule);
    } else if (format == CSS) {
        rule.pattern = QRegularExpression("\\b[a-zA-Z-]+\\s*:");
        rule.format = keywordFormat;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("(?<=:)\\s*[^;]+(?=;)");
        rule.format = stringFormat;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("/\\*.*\\*/");
        rule.format = commentFormat;
        highlightingRules.append(rule);
    } else if (format == JavaScript) {
        QStringList jsKeywords = {"function", "var", "let", "const", "if", "else", "for", "while", "return"};
        for (const QString &keyword : jsKeywords) {
            rule.pattern = QRegularExpression("\\b" + keyword + "\\b");
            rule.format = keywordFormat;
            highlightingRules.append(rule);
        }

        rule.pattern = QRegularExpression("//.*$");
        rule.format = commentFormat;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("/\\*.*\\*/");
        rule.format = commentFormat;
        highlightingRules.append(rule);
    }
}

void ResponseHighlighter::highlightBlock(const QString &text) {
    if (format == Plain) return;

    for (const HighlightingRule &rule : highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}
