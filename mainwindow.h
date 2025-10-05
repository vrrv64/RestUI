#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QMap>
#include <QTreeWidgetItem>
#include <QSplitter>
#include <QSettings>
#include <QPlainTextEdit> // New: For syntax highlighting
#include <QSyntaxHighlighter> // New: For syntax highlighting
#include <QStyle>
#include <QLabel>
#include <QShortcut>
#include <QCheckBox>
#include <QDebug>
#include <KSyntaxHighlighting/Theme>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/SyntaxHighlighter>
#include "customtreewidget.h"
#include "responsehighlighter.h" // New include

QT_BEGIN_NAMESPACE
class QComboBox;
class QLineEdit;
class QTextEdit;
class QPushButton;
class QTabWidget;
class QTreeWidget;
class QNetworkReply;
class QTableWidget;
QT_END_NAMESPACE

struct RequestData {
    QString url;
    QString method;
    QString headers;
    QString body;
    QMap<QString, QString> formParams;
    QString folder; // Added to track the folder (empty for root-level requests)
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void sendRequest();
    void setThemeBasedOnMode();
    bool checkDarkMode();
    void handleResponse(QNetworkReply *reply);
    void saveRequest();
    void loadRequest(QTreeWidgetItem *item, int column);
    void renameItem();
    void deleteItem();
    void addFolder();
    void moveRequestToFolder();
    void focusUrlField();
    void addNewFolder();
    void newRequest();
    void showSearchDialog();

private:
    void loadSavedRequests();
    void saveRequestsToFile();
    void saveSettings();
    void restoreSettings();
    bool eventFilter(QObject*, QEvent*);
    void showFormParamsContextMenu(const QPoint&);
    QString generateUniqueKey(const QString &folder, const QString &name); // New helper function
    void saveFolderState(QTreeWidgetItem *item);
    void restoreFolderState(QTreeWidgetItem *item);
    QComboBox *methodComboBox;
    QLineEdit *urlLineEdit;
    QTextEdit *headersTextEdit;
    QTextEdit *bodyTextEdit;
    QTableWidget *formParamsTable;
    QTabWidget *requestTabWidget;
    QLabel *responseStatusLabel;
    QPushButton *sendButton;
    QPushButton *saveButton;
    QTabWidget *responseTabWidget;
    QTextEdit *responseHeadersEdit;
    KSyntaxHighlighting::Repository *syntaxRepository;
    KSyntaxHighlighting::SyntaxHighlighter *syntaxHighlighter;
    QTextEdit *responseBodyEdit;
    QNetworkAccessManager *networkManager;
    CustomTreeWidget *sidebar;
    QSplitter *splitter;
    //ResponseHighlighter *highlighter; // Commented out as it's no longer used
    QShortcut *urlFocusShortcut;
    QTreeWidgetItem *currentFolder;
    QPushButton *addFolderButton;
    QPushButton *resetRequestButton;
    QShortcut *newRequestShortcut;
    QMap<QString, RequestData> requests; // Key will now be folder/name or just name
    QMap<QString, QTreeWidgetItem*> folders;
    QString lastSearchTerm;
};

#endif // MAINWINDOW_H
