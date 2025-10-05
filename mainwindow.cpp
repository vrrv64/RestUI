#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QMessageBox>
#include <QTabWidget>
#include <QTreeWidget>
#include <QFile>
#include <QIcon>
#include <QDomDocument>
#include <QDomElement>
#include <QTextStream>
#include <QInputDialog>
#include <QMenu>
#include <QTableWidget>
#include <QSplitter>
#include <QKeyEvent>
#include <QAction>
#include <QStandardPaths>
#include <QDir>
#include <QApplication>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QNetworkReply>
#include <QHeaderView>
#include <QUrlQuery>
#include <QShortcut>
#include <QCheckBox>
#include <QDebug>

// Removed unused includes:
// - <QJsonObject>, <QJsonArray>, <QRegularExpression> (not directly used in the file)
// - <QSettings> (used in saveSettings/restoreSettings, but already included via mainwindow.h)
// - <QStyle>, <QLabel> (used in UI setup, but already included via mainwindow.h or Qt widgets)

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), networkManager(new QNetworkAccessManager(this)), currentFolder(nullptr), lastSearchTerm("") {
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // URL Row with equal spacing above and below
    QHBoxLayout *urlLayout = new QHBoxLayout;
    urlLayout->addWidget(new QLabel("    URL:"));
    //urlLineEdit = new QLineEdit("https://jsonplaceholder.typicode.com/posts/1");
    urlLineEdit = new QLineEdit("");
    urlLineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    urlLayout->addWidget(urlLineEdit);
    sendButton = new QPushButton("Go");
    urlLayout->addWidget(sendButton);
    methodComboBox = new QComboBox;
    methodComboBox->addItems({"GET", "POST", "PUT", "DELETE", "HEAD"});
    urlLayout->addWidget(methodComboBox);
    saveButton = new QPushButton;
    saveButton->setIcon(style()->standardIcon(QStyle::SP_DriveFDIcon));
    saveButton->setToolTip("Save Request");
    saveButton->setFixedSize(24, 24);
    //urlLayout->addWidget(saveButton);
    urlLayout->setContentsMargins(0, 10, 0, 4);
    mainLayout->addLayout(urlLayout);

    // Content layout for sidebar and splitter
    QWidget *contentWidget = new QWidget;
    contentWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QHBoxLayout *contentLayout = new QHBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    // Sidebar
    QVBoxLayout *sidebarLayout = new QVBoxLayout;
    sidebar = new CustomTreeWidget(this);
    sidebar->setHeaderLabel("Saved Requests");
    sidebar->setContextMenuPolicy(Qt::CustomContextMenu);
    sidebar->setMaximumWidth(200);
    sidebar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    sidebarLayout->addWidget(sidebar, 1);
    sidebar->setDragEnabled(true);
    sidebar->setAcceptDrops(true);
    sidebar->setDropIndicatorShown(true);
    sidebar->setDefaultDropAction(Qt::MoveAction);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    addFolderButton = new QPushButton;
    addFolderButton->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
    addFolderButton->setToolTip("Add Folder");
    addFolderButton->setFixedSize(24, 24);
    buttonLayout->addWidget(addFolderButton);

    buttonLayout->addWidget(saveButton);

    resetRequestButton = new QPushButton;
    resetRequestButton->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
    resetRequestButton->setToolTip("Clear Fields");
    resetRequestButton->setFixedSize(24, 24);
    buttonLayout->addWidget(resetRequestButton);
    sidebarLayout->addLayout(buttonLayout);
    sidebarLayout->addStretch();
    contentLayout->addLayout(sidebarLayout);

    // Splitter
    splitter = new QSplitter(Qt::Vertical);
    splitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    contentLayout->addWidget(splitter, 1);

    // Request Pane
    QWidget *requestPane = new QWidget;
    requestPane->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout *requestLayout = new QVBoxLayout(requestPane);
    requestLayout->setContentsMargins(0, 0, 0, 0);
    QLabel *requestLabel = new QLabel("Request Data");
    requestLabel->setAlignment(Qt::AlignCenter);
    requestLayout->addWidget(requestLabel);
    requestTabWidget = new QTabWidget;
    requestTabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    requestTabWidget->setCurrentIndex(0);

    headersTextEdit = new QTextEdit;
    headersTextEdit->setText("Content-Type: application/x-www-form-urlencoded");
    headersTextEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    requestTabWidget->addTab(headersTextEdit, "Headers");

    bodyTextEdit = new QTextEdit;
    bodyTextEdit->setPlaceholderText("{\"title\": \"foo\", \"body\": \"bar\", \"userId\": 1}");
    bodyTextEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    requestTabWidget->addTab(bodyTextEdit, "Body");

    formParamsTable = new QTableWidget(4, 2);
    formParamsTable->setHorizontalHeaderLabels({"Key", "Value"});
    formParamsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    formParamsTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    formParamsTable->installEventFilter(this);
    formParamsTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(formParamsTable, &QTableWidget::customContextMenuRequested, this, &MainWindow::showFormParamsContextMenu);
    requestTabWidget->addTab(formParamsTable, "Form Parameters");

    requestLayout->addWidget(requestTabWidget);
    requestLayout->addStretch();
    splitter->addWidget(requestPane);

    // Response Pane
    QWidget *responsePane = new QWidget;
    responsePane->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout *responseLayout = new QVBoxLayout(responsePane);
    responseLayout->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout *responseHeaderLayout = new QHBoxLayout;
    QLabel *responseLabel = new QLabel("Response");
    responseLabel->setAlignment(Qt::AlignCenter);
    responseHeaderLayout->addWidget(responseLabel);
    responseStatusLabel = new QLabel;
    responseStatusLabel->setAlignment(Qt::AlignRight);
    responseHeaderLayout->addWidget(responseStatusLabel);
    responseHeaderLayout->setAlignment(Qt::AlignCenter);
    responseLayout->addLayout(responseHeaderLayout);
    responseTabWidget = new QTabWidget;
    responseTabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    responseHeadersEdit = new QTextEdit;
    responseHeadersEdit->setReadOnly(true);
    responseHeadersEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    responseTabWidget->addTab(responseHeadersEdit, "Headers");

    responseBodyEdit = new QTextEdit(this);
    responseBodyEdit->setReadOnly(true);
    responseBodyEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    //highlighter = new ResponseHighlighter(responseBodyEdit->document(), "text/plain");
    syntaxRepository = new KSyntaxHighlighting::Repository();
    syntaxHighlighter = new KSyntaxHighlighting::SyntaxHighlighter(responseBodyEdit->document());
    responseTabWidget->addTab(responseBodyEdit, "Body");
    responseTabWidget->setCurrentIndex(1);
    responseLayout->addWidget(responseTabWidget);
    splitter->addWidget(responsePane);

    setThemeBasedOnMode();

    splitter->setSizes(QList<int>() << 300 << 200);

    mainLayout->addWidget(contentWidget);

    connect(sendButton, &QPushButton::clicked, this, &MainWindow::sendRequest);
    connect(saveButton, &QPushButton::clicked, this, &MainWindow::saveRequest);
    connect(resetRequestButton, &QPushButton::clicked, this, &MainWindow::newRequest);
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::handleResponse);
    connect(sidebar, &QTreeWidget::itemClicked, this, &MainWindow::loadRequest);
    connect(sidebar, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *item, int) {
        if (item && item->data(0, Qt::UserRole).toString() == "folder") {
            currentFolder = item;
        } else {
            currentFolder = nullptr;
        }
    });
    connect(sidebar, &QTreeWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        QTreeWidgetItem *item = sidebar->itemAt(pos);
        if (!item) return;
        QMenu contentMenu;
        contentMenu.addAction("Rename", this, &MainWindow::renameItem);
        contentMenu.addAction("Delete", this, &MainWindow::deleteItem);
        QString itemType = item->data(0, Qt::UserRole).toString();
        if (itemType == "folder") {
            contentMenu.addAction("Add Folder", this, &MainWindow::addFolder);
        } else if (itemType == "request") {
            contentMenu.addAction("Move to Folder", this, &MainWindow::moveRequestToFolder);
        }
        contentMenu.exec(sidebar->mapToGlobal(pos));
    });

    connect(sidebar, &CustomTreeWidget::itemsReordered, this, &MainWindow::saveRequestsToFile);

    urlFocusShortcut = new QShortcut(QKeySequence("Ctrl+L"), this);
    connect(urlFocusShortcut, &QShortcut::activated, this, &MainWindow::focusUrlField);

    connect(urlLineEdit, &QLineEdit::returnPressed, this, &MainWindow::sendRequest);

    connect(addFolderButton, &QPushButton::clicked, this, &MainWindow::addNewFolder);

    newRequestShortcut = new QShortcut(QKeySequence("Ctrl+N"), this);
    connect(newRequestShortcut, &QShortcut::activated, this, &MainWindow::newRequest);

    QShortcut *searchShortcut = new QShortcut(QKeySequence("Ctrl+F"), this);
    connect(searchShortcut, &QShortcut::activated, this, &MainWindow::showSearchDialog);

    loadSavedRequests();
    setWindowTitle("RestUI");

    QIcon appIcon;
    appIcon.addFile(":/icons/icon.png", QSize(512, 512));
    this->setWindowIcon(appIcon);

    restoreSettings();
}

void MainWindow::setThemeBasedOnMode() {
    bool isDarkMode = checkDarkMode();
    QStringList lightThemes = {"Atom One Light", "ayu Light", "Github Light", "Breeze Light"};
    QStringList darkThemes = {"Vim Dark", "Github Dark"};
    QVector<KSyntaxHighlighting::Theme> themes = syntaxRepository->themes();
    QStringList availableThemes;
    for (const auto &theme : themes) {
        availableThemes.append(theme.name());
    }
    //qDebug() << availableThemes << "\n";
    QString selectedTheme;
    if (isDarkMode) {
        for (const QString &theme : darkThemes) {
            if (availableThemes.contains(theme)) {
                selectedTheme = theme;
                //qDebug() << "selectedTheme: " << selectedTheme << "\n";
                break;
            }
        }
        if (selectedTheme.isEmpty()) {
            selectedTheme = syntaxRepository->defaultTheme(KSyntaxHighlighting::Repository::DarkTheme).name();
        }
    } else {
        for (const QString &theme : lightThemes) {
            if (availableThemes.contains(theme)) {
                selectedTheme = theme;
                //qDebug() << "selectedTheme: " << selectedTheme << "\n";
                break;
            }
        }
        if (selectedTheme.isEmpty()) {
            selectedTheme = syntaxRepository->defaultTheme(KSyntaxHighlighting::Repository::LightTheme).name();
        }
    }

    syntaxHighlighter->setTheme(syntaxRepository->theme(selectedTheme));
    syntaxHighlighter->rehighlight();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (obj == formParamsTable && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Tab) {
            int currentRow = formParamsTable->currentRow();
            int currentColumn = formParamsTable->currentColumn();
            int rowCount = formParamsTable->rowCount();
            if (currentRow == rowCount - 1 && currentColumn == 1) {
                formParamsTable->insertRow(rowCount);
                formParamsTable->setCurrentCell(rowCount, 0);
                formParamsTable->edit(formParamsTable->currentIndex());
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::showFormParamsContextMenu(const QPoint &pos) {
    QMenu contextMenu(this);
    QAction *addRowAction = contextMenu.addAction("Add a new row");
    QAction *deleteRowAction = contextMenu.addAction("Delete row");
    deleteRowAction->setEnabled(formParamsTable->currentRow() != -1);
    QAction *selectedAction = contextMenu.exec(formParamsTable->viewport()->mapToGlobal(pos));
    if (selectedAction == addRowAction) {
        int rowCount = formParamsTable->rowCount();
        formParamsTable->insertRow(rowCount);
    } else if (selectedAction == deleteRowAction) {
        int currentRow = formParamsTable->currentRow();
        if (currentRow != -1 && formParamsTable->rowCount() > 1) {
            formParamsTable->removeRow(currentRow);
        }
    }
}

void MainWindow::showSearchDialog() {
    if (responseTabWidget->currentIndex() != 1) {
        return;
    }

    QDialog *searchDialog = new QDialog(this);
    searchDialog->setWindowTitle("Find in Response Body");
    searchDialog->setModal(false);

    QVBoxLayout *layout = new QVBoxLayout(searchDialog);

    QLineEdit *searchInput = new QLineEdit;
    searchInput->setPlaceholderText("Enter search text...");
    searchInput->setText(lastSearchTerm);
    searchInput->selectAll();
    layout->addWidget(searchInput);

    QCheckBox *caseSensitive = new QCheckBox("Case sensitive");
    layout->addWidget(caseSensitive);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    QPushButton *closeButton = new QPushButton("Close");
    buttonLayout->addWidget(closeButton);
    QPushButton *findPrevButton = new QPushButton("Prev.");
    buttonLayout->addWidget(findPrevButton);
    QPushButton *findNextButton = new QPushButton("Next");
    findNextButton->setDefault(true);
    buttonLayout->addWidget(findNextButton);
    layout->addLayout(buttonLayout);

    connect(findNextButton, &QPushButton::clicked, this, [this, searchInput, caseSensitive]() {
        QString searchText = searchInput->text();
        if (searchText.isEmpty()) return;
        lastSearchTerm = searchText;
        QTextDocument::FindFlags flags;
        if (caseSensitive->isChecked()) {
            flags |= QTextDocument::FindCaseSensitively;
        }
        QTextCursor cursor = responseBodyEdit->textCursor();
        cursor = responseBodyEdit->document()->find(searchText, cursor, flags);
        if (!cursor.isNull()) {
            responseBodyEdit->setTextCursor(cursor);
            responseBodyEdit->ensureCursorVisible();
            QTextCharFormat highlightFormat;
            highlightFormat.setBackground(Qt::yellow);
            cursor.mergeCharFormat(highlightFormat);
        } else {
            QMessageBox::information(this, "Search", "No more occurrences found.");
            cursor = QTextCursor(responseBodyEdit->document());
            cursor = responseBodyEdit->document()->find(searchText, cursor, flags);
            if (!cursor.isNull()) {
                responseBodyEdit->setTextCursor(cursor);
                responseBodyEdit->ensureCursorVisible();
                QTextCharFormat highlightFormat;
                highlightFormat.setBackground(Qt::yellow);
                cursor.mergeCharFormat(highlightFormat);
            }
        }
    });

    connect(findPrevButton, &QPushButton::clicked, this, [this, searchInput, caseSensitive]() {
        QString searchText = searchInput->text();
        if (searchText.isEmpty()) return;
        lastSearchTerm = searchText;
        QTextDocument::FindFlags flags = QTextDocument::FindBackward;
        if (caseSensitive->isChecked()) {
            flags |= QTextDocument::FindCaseSensitively;
        }
        QTextCursor cursor = responseBodyEdit->textCursor();
        cursor = responseBodyEdit->document()->find(searchText, cursor, flags);
        if (!cursor.isNull()) {
            responseBodyEdit->setTextCursor(cursor);
            responseBodyEdit->ensureCursorVisible();
            QTextCharFormat highlightFormat;
            highlightFormat.setBackground(Qt::yellow);
            cursor.mergeCharFormat(highlightFormat);
        } else {
            QMessageBox::information(this, "Search", "No previous occurrences found.");
            cursor = QTextCursor(responseBodyEdit->document());
            cursor.movePosition(QTextCursor::End);
            cursor = responseBodyEdit->document()->find(searchText, cursor, flags);
            if (!cursor.isNull()) {
                responseBodyEdit->setTextCursor(cursor);
                responseBodyEdit->ensureCursorVisible();
                QTextCharFormat highlightFormat;
                highlightFormat.setBackground(Qt::yellow);
                cursor.mergeCharFormat(highlightFormat);
            }
        }
    });

    connect(closeButton, &QPushButton::clicked, searchDialog, [this, searchDialog]() {
        QTextCursor cursor(responseBodyEdit->document());
        cursor.select(QTextCursor::Document);
        QTextCharFormat clearFormat;
        clearFormat.setBackground(Qt::transparent);
        cursor.mergeCharFormat(clearFormat);
        searchDialog->close();
    });

    connect(searchInput, &QLineEdit::returnPressed, findNextButton, &QPushButton::click);

    searchDialog->setLayout(layout);
    searchDialog->resize(300, 150);
    searchDialog->show();
    searchInput->setFocus();
}

void MainWindow::focusUrlField() {
    urlLineEdit->setFocus();
    urlLineEdit->selectAll();
}

void MainWindow::newRequest() {
    urlLineEdit->clear();
    methodComboBox->setCurrentIndex(0);
    headersTextEdit->setText("Content-Type: application/x-www-form-urlencoded");
    bodyTextEdit->clear();
    responseStatusLabel->clear();
    formParamsTable->clearContents();
    formParamsTable->setRowCount(4);
    urlLineEdit->setFocus();
    responseHeadersEdit->clear();
    responseBodyEdit->clear();
    sidebar->selectionModel()->clearSelection();
    lastSearchTerm = "";
}

MainWindow::~MainWindow() {
    saveSettings();
    delete syntaxRepository;
}

void MainWindow::sendRequest() {

    responseStatusLabel->clear();

    QUrl url(urlLineEdit->text().trimmed());
    if (!url.isValid()) {
        QMessageBox::warning(this, "Error", "Invalid URL");
        return;
    }

    QNetworkRequest request(url);

    QString headersText = headersTextEdit->toPlainText();
    QStringList headers = headersText.split("\n", Qt::SkipEmptyParts);
    for (const QString &header : headers) {
        QStringList parts = header.split(":", Qt::SkipEmptyParts);
        if (parts.size() >= 2) {
            request.setRawHeader(parts[0].trimmed().toUtf8(), parts[1].trimmed().toUtf8());
        }
    }

    QString method = methodComboBox->currentText();
    QByteArray body;

    bool useFormParams = (requestTabWidget->currentWidget() == formParamsTable);
    QMap<QString, QString> formParams;
    if (useFormParams && (method == "POST" || method == "PUT")) {
        for (int row = 0; row < formParamsTable->rowCount(); ++row) {
            QTableWidgetItem *keyItem = formParamsTable->item(row, 0);
            QTableWidgetItem *valueItem = formParamsTable->item(row, 1);
            if (keyItem && !keyItem->text().isEmpty()) {
                QString value = valueItem ? valueItem->text() : "";
                formParams[keyItem->text()] = value;
            }
        }
        if (!formParams.isEmpty()) {
            QUrlQuery params;
            for (auto it = formParams.constBegin(); it != formParams.constEnd(); ++it) {
                params.addQueryItem(it.key(), it.value());
            }
            body = params.toString(QUrl::FullyEncoded).toUtf8();
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        }
    }

    if (body.isEmpty() && (method == "POST" || method == "PUT")) {
        body = bodyTextEdit->toPlainText().toUtf8();
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    }

    responseHeadersEdit->setText("Sending request...");
    responseBodyEdit->setText("Sending request...");

    if (method == "GET") {
        networkManager->get(request);
    } else if (method == "POST") {
        networkManager->post(request, body);
    } else if (method == "PUT") {
        networkManager->put(request, body);
    } else if (method == "DELETE") {
        networkManager->deleteResource(request);
    } else if (method == "HEAD") {
        networkManager->head(request);
    }
}

void MainWindow::handleResponse(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray responseData = reply->readAll();
        QString contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString().toLower();

        QString formattedResponse;
        if (contentType.contains("application/json") || contentType.contains("json")) {
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
            if (!jsonDoc.isNull()) {
                formattedResponse = QString(jsonDoc.toJson(QJsonDocument::Indented));
            } else {
                formattedResponse = QString::fromUtf8(responseData);
            }
        } else if (contentType.contains("text/xml") || contentType.contains("application/xml") || contentType.contains("xml")) {
            QDomDocument xmlDoc;
            if (xmlDoc.setContent(responseData)) {
                formattedResponse = xmlDoc.toString(4);
            } else {
                formattedResponse = QString::fromUtf8(responseData);
            }
        } else {
            formattedResponse = QString::fromUtf8(responseData);
        }

        //delete highlighter;
        //highlighter = new ResponseHighlighter(responseBodyEdit->document(), contentType);
        QString definitionName;
        if (contentType.contains("json")) {
            definitionName = "JSON";
        } else if (contentType.contains("xml")) {
            definitionName = "XML";
        } else if (contentType.contains("html")) {
            definitionName = "HTML";
        } else if (contentType.contains("css")) {
            definitionName = "CSS";
        } else if (contentType.contains("javascript") || contentType.contains("js")) {
            definitionName = "JavaScript";
        } else {
            definitionName = "Plain Text";
        }

        KSyntaxHighlighting::Definition def = syntaxRepository->definitionForName(definitionName);
        syntaxHighlighter->setDefinition(def);

        responseBodyEdit->setPlainText(formattedResponse);
        syntaxHighlighter->setDocument(responseBodyEdit->document());

        QStringList headerLines;
        for (const QPair<QByteArray, QByteArray> &pair : reply->rawHeaderPairs()) {
            headerLines << QString("%1: %2").arg(QString(pair.first), QString(pair.second));
        }
        responseHeadersEdit->setText(headerLines.join("\n"));
    } else {
        responseHeadersEdit->setText("Error");
        responseBodyEdit->setPlainText(reply->errorString());
    }

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QString statusMessage = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
    QString statusText = QString("(Status: %1 %2)").arg(statusCode).arg(statusMessage);
    responseStatusLabel->setText(statusText);

    if (statusCode >= 200 && statusCode < 300) {
        responseStatusLabel->setStyleSheet("color: rgb(78,228,78);");
    } else if (statusCode >= 400) {
        responseStatusLabel->setStyleSheet("color: rgba(255, 85, 85, 1);");
    } else {
        responseStatusLabel->setStyleSheet("color: gray;");
    }

    reply->deleteLater();
}

bool MainWindow::checkDarkMode() {
    return QApplication::palette().color(QPalette::Window).value() < 128;
}

void MainWindow::saveRequest() {
    bool ok;
    QString name;
    while (true) {
        name = QInputDialog::getText(this, "Save Request", "Enter request name:", QLineEdit::Normal, "", &ok);
        if (!ok || name.isEmpty()) return;
        QString key = generateUniqueKey(currentFolder ? currentFolder->text(0) : "", name);
        bool nameExists = requests.contains(key);
        if (nameExists) {
            QMessageBox::StandardButton reply = QMessageBox::question(
                this, "Name Conflict", QString("A request named '%1' already exists in this folder. Overwrite it?").arg(name),
                QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                requests.remove(key);
                if (currentFolder) {
                    for (int i = 0; i < currentFolder->childCount(); ++i) {
                        if (currentFolder->child(i)->text(0) == name &&
                            currentFolder->child(i)->data(0, Qt::UserRole).toString() == "request") {
                            delete currentFolder->child(i);
                            break;
                        }
                    }
                } else {
                    for (int i = 0; i < sidebar->topLevelItemCount(); ++i) {
                        if (sidebar->topLevelItem(i)->text(0) == name &&
                            sidebar->topLevelItem(i)->data(0, Qt::UserRole).toString() == "request") {
                            delete sidebar->takeTopLevelItem(i);
                            break;
                        }
                    }
                }
                break;
            }
        } else {
            break;
        }
    }

    RequestData data;
    data.url = urlLineEdit->text().trimmed();
    data.method = methodComboBox->currentText();
    data.headers = headersTextEdit->toPlainText();
    data.folder = currentFolder ? currentFolder->text(0) : "";

    if (requestTabWidget->currentWidget() == formParamsTable) {
        for (int row = 0; row < formParamsTable->rowCount(); ++row) {
            QTableWidgetItem *keyItem = formParamsTable->item(row, 0);
            QTableWidgetItem *valueItem = formParamsTable->item(row, 1);
            if (keyItem && !keyItem->text().isEmpty()) {
                QString value = valueItem ? valueItem->text() : "";
                data.formParams[keyItem->text()] = value;
            }
        }
    } else {
        data.body = bodyTextEdit->toPlainText();
    }

    QString key = generateUniqueKey(data.folder, name);
    requests[key] = data;
    QTreeWidgetItem *item;
    if (currentFolder) {
        item = new QTreeWidgetItem(currentFolder);
    } else {
        item = new QTreeWidgetItem(sidebar);
    }
    item->setText(0, name);
    item->setData(0, Qt::UserRole, "request");
    item->setToolTip(0, QString("%1").arg(name));
    saveRequestsToFile();
}

void MainWindow::loadRequest(QTreeWidgetItem *item, int column) {
    Q_UNUSED(column);
    if (!item || item->childCount() > 0) return;
    QString name = item->text(0);
    QString folder = item->parent() ? item->parent()->text(0) : "";
    QString key = generateUniqueKey(folder, name);

    if (requests.contains(key)) {
        RequestData data = requests[key];
        urlLineEdit->setText(data.url);
        methodComboBox->setCurrentText(data.method);
        headersTextEdit->setText(data.headers);
        formParamsTable->clearContents();
        formParamsTable->setRowCount(5);
        if (!data.formParams.isEmpty()) {
            //requestTabWidget->setCurrentWidget(formParamsTable);
            formParamsTable->clearContents();
            formParamsTable->setRowCount(0);
            for (auto it = data.formParams.constBegin(); it != data.formParams.constEnd(); ++it) {
                int row = formParamsTable->rowCount();
                formParamsTable->insertRow(row);
                formParamsTable->setItem(row, 0, new QTableWidgetItem(it.key()));
                formParamsTable->setItem(row, 1, new QTableWidgetItem(it.value()));
            }
        } else {
            //requestTabWidget->setCurrentWidget(bodyTextEdit);
            bodyTextEdit->setText(data.body);
        }
    }
    responseStatusLabel->clear();
    responseHeadersEdit->clear();
    responseBodyEdit->clear();
}

void MainWindow::renameItem() {
    QTreeWidgetItem *item = sidebar->currentItem();
    if (!item) return;
    bool ok;
    QString oldName = item->text(0);
    QString oldFolder = item->parent() ? item->parent()->text(0) : "";
    QString oldKey = generateUniqueKey(oldFolder, oldName);
    QString newName = QInputDialog::getText(this, "Rename Item", "Enter new name:", QLineEdit::Normal, oldName, &ok);
    if (ok && !newName.isEmpty() && newName != oldName) {
        QString newFolder = item->parent() ? item->parent()->text(0) : "";
        QString newKey = generateUniqueKey(newFolder, newName);
        QString itemType = item->data(0, Qt::UserRole).toString();
        if (itemType == "request" && requests.contains(oldKey)) {
            RequestData data = requests.take(oldKey);
            data.folder = newFolder;
            requests[newKey] = data;
        } else if (itemType == "folder" && folders.contains(oldName)) {
            folders[newName] = folders.take(oldName);
        }
        item->setText(0, newName);
        saveRequestsToFile();
    }
}

void MainWindow::deleteItem() {
    QTreeWidgetItem *item = sidebar->currentItem();
    if (!item) return;
    QString name = item->text(0);
    QString folder = item->parent() ? item->parent()->text(0) : "";
    QString key = generateUniqueKey(folder, name);
    QString itemType = item->data(0, Qt::UserRole).toString();
    if (itemType == "request" && requests.contains(key)) {
        requests.remove(key);
    } else if (itemType == "folder" && folders.contains(name)) {
        folders.remove(name);
    }
    delete item;
    saveRequestsToFile();
}

void MainWindow::addFolder() {
    bool ok;
    QString name = QInputDialog::getText(this, "Add Folder", "Enter folder name:", QLineEdit::Normal, "", &ok);
    if (ok && !name.isEmpty()) {
        QTreeWidgetItem *folder = new QTreeWidgetItem(sidebar);
        folder->setText(0, name);
        folder->setData(0, Qt::UserRole, "folder");
        folders[name] = folder;
        saveRequestsToFile();
    }
}

void MainWindow::moveRequestToFolder() {
    QTreeWidgetItem *item = sidebar->currentItem();
    if (!item || item->data(0, Qt::UserRole).toString() != "request") return;
    QString requestName = item->text(0);
    QString oldFolder = item->parent() ? item->parent()->text(0) : "";
    QString oldKey = generateUniqueKey(oldFolder, requestName);
    QStringList folderNames = folders.keys();
    bool ok;
    QString folderName = QInputDialog::getItem(this, "Move to Folder", "Select folder:", folderNames, 0, false, &ok);
    if (!ok || folderName.isEmpty()) return;
    QTreeWidgetItem *targetFolder = folders[folderName];
    bool nameExists = false;
    QString newKey = generateUniqueKey(folderName, requestName);
    if (requests.contains(newKey)) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Name Conflict", QString("A request named '%1' already exists in folder '%2'. Overwrite it?").arg(requestName, folderName),
            QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            requests.remove(newKey);
            for (int i = 0; i < targetFolder->childCount(); ++i) {
                if (targetFolder->child(i)->text(0) == requestName &&
                    targetFolder->child(i)->data(0, Qt::UserRole).toString() == "request") {
                    delete targetFolder->child(i);
                    break;
                }
            }
        } else {
            return;
        }
    }
    QTreeWidgetItem *newItem = new QTreeWidgetItem(targetFolder);
    newItem->setText(0, requestName);
    newItem->setData(0, Qt::UserRole, "request");
    delete item;
    RequestData data = requests.take(oldKey);
    data.folder = folderName;
    requests[newKey] = data;
    saveRequestsToFile();
}

void MainWindow::addNewFolder() {
    addFolder();
}

void MainWindow::saveRequestsToFile() {
    QDomDocument doc("RestUIRequests");
    QDomElement root = doc.createElement("requests");
    doc.appendChild(root);
    std::function<void(QTreeWidgetItem*, QDomElement&)> addItemToXml = [&](QTreeWidgetItem* item, QDomElement& parentElement) {
        QString itemType = item->data(0, Qt::UserRole).toString();
        if (itemType == "folder") {
            QDomElement folderElement = doc.createElement("folder");
            folderElement.setAttribute("name", item->text(0));
            parentElement.appendChild(folderElement);
            for (int i = 0; i < item->childCount(); ++i) {
                addItemToXml(item->child(i), folderElement);
            }
        } else if (itemType == "request") {
            QString requestName = item->text(0);
            QString folderName = item->parent() ? item->parent()->text(0) : "";
            QString key = generateUniqueKey(folderName, requestName);
            if (requests.contains(key)) {
                RequestData data = requests[key];
                QDomElement requestElement = doc.createElement("request");
                requestElement.setAttribute("name", requestName);
                parentElement.appendChild(requestElement);
                QDomElement urlElement = doc.createElement("url");
                urlElement.appendChild(doc.createTextNode(data.url));
                requestElement.appendChild(urlElement);
                QDomElement methodElement = doc.createElement("method");
                methodElement.appendChild(doc.createTextNode(data.method));
                requestElement.appendChild(methodElement);
                QDomElement headersElement = doc.createElement("headers");
                headersElement.appendChild(doc.createTextNode(data.headers));
                requestElement.appendChild(headersElement);
                QDomElement bodyElement = doc.createElement("body");
                bodyElement.appendChild(doc.createTextNode(data.body));
                requestElement.appendChild(bodyElement);
                QDomElement formParamsElement = doc.createElement("formParams");
                for (auto paramIt = data.formParams.constBegin(); paramIt != data.formParams.constEnd(); ++paramIt) {
                    QDomElement paramElement = doc.createElement("param");
                    paramElement.setAttribute("key", paramIt.key());
                    paramElement.appendChild(doc.createTextNode(paramIt.value()));
                    formParamsElement.appendChild(paramElement);
                }
                requestElement.appendChild(formParamsElement);
            }
        }
    };
    for (int i = 0; i < sidebar->topLevelItemCount(); ++i) {
        addItemToXml(sidebar->topLevelItem(i), root);
    }
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (dataDir.isEmpty()) {
        QMessageBox::warning(this, "Save Error", "Failed to determine data directory. Requests not saved.");
        return;
    }
    QDir dir(dataDir);
    if (!dir.exists()) {
        if (!dir.mkpath(dataDir)) {
            QMessageBox::warning(this, "Save Error", "Failed to create data directory. Requests not saved.");
            return;
        }
    }
    QString filePath = dataDir + "/saved_requests.xml";
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream out(&file);
        out << doc.toString(4);
        file.close();
    } else {
        QMessageBox::warning(this, "Save Error", "Failed to save requests to file.");
    }
}

void MainWindow::loadSavedRequests() {
    QString filePath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/saved_requests.xml";
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return;
    QDomDocument doc;
    if (!doc.setContent(&file)) {
        file.close();
        return;
    }
    file.close();
    QDomElement root = doc.documentElement();
    if (root.tagName() != "requests") return;
    QDomNodeList nodes = root.childNodes();
    for (int i = 0; i < nodes.count(); ++i) {
        QDomElement element = nodes.at(i).toElement();
        if (element.isNull()) continue;
        if (element.tagName() == "folder") {
            QString folderName = element.attribute("name");
            QTreeWidgetItem *folderItem = new QTreeWidgetItem(sidebar);
            folderItem->setText(0, folderName);
            folderItem->setData(0, Qt::UserRole, "folder");
            folders[folderName] = folderItem;
            QDomNodeList childNodes = element.childNodes();
            for (int j = 0; j < childNodes.count(); ++j) {
                QDomElement childElement = childNodes.at(j).toElement();
                if (childElement.tagName() == "request") {
                    QString childName = childElement.attribute("name");
                    RequestData childData;
                    QDomNodeList requestNodes = childElement.childNodes();
                    for (int k = 0; k < requestNodes.count(); ++k) {
                        QDomElement node = requestNodes.at(k).toElement();
                        if (node.tagName() == "url") childData.url = node.text();
                        else if (node.tagName() == "method") childData.method = node.text();
                        else if (node.tagName() == "headers") childData.headers = node.text();
                        else if (node.tagName() == "body") childData.body = node.text();
                        else if (node.tagName() == "formParams") {
                            QDomNodeList paramNodes = node.childNodes();
                            for (int l = 0; l < paramNodes.count(); ++l) {
                                QDomElement param = paramNodes.at(l).toElement();
                                if (param.tagName() == "param") {
                                    childData.formParams[param.attribute("key")] = param.text();
                                }
                            }
                        }
                    }
                    childData.folder = folderName;
                    requests[generateUniqueKey(folderName, childName)] = childData;
                    QTreeWidgetItem *childItem = new QTreeWidgetItem(folderItem);
                    childItem->setText(0, childName);
                    childItem->setData(0, Qt::UserRole, "request");
                    childItem->setToolTip(0, QString("%1").arg(childName));
                }
            }
        } else if (element.tagName() == "request") {
            QString requestName = element.attribute("name");
            RequestData data;
            QDomNodeList requestNodes = element.childNodes();
            for (int j = 0; j < requestNodes.count(); ++j) {
                QDomElement node = requestNodes.at(j).toElement();
                if (node.tagName() == "url") data.url = node.text();
                else if (node.tagName() == "method") data.method = node.text();
                else if (node.tagName() == "headers") data.headers = node.text();
                else if (node.tagName() == "body") data.body = node.text();
                else if (node.tagName() == "formParams") {
                    QDomNodeList paramNodes = node.childNodes();
                    for (int k = 0; k < paramNodes.count(); ++k) {
                        QDomElement param = paramNodes.at(k).toElement();
                        if (param.tagName() == "param") {
                            data.formParams[param.attribute("key")] = param.text();
                        }
                    }
                }
            }
            data.folder = "";
            requests[generateUniqueKey("", requestName)] = data;
            QTreeWidgetItem *item = new QTreeWidgetItem(sidebar);
            item->setText(0, requestName);
            item->setData(0, Qt::UserRole, "request");
            item->setToolTip(0, QString("%1").arg(requestName));
        }
    }
    restoreFolderState(sidebar->invisibleRootItem()); // Restore folder state after loading
}

void MainWindow::saveSettings() {
    QSettings settings("xAI", "RestUI");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("splitterSizes", splitter->saveState());
    saveFolderState(sidebar->invisibleRootItem()); // Save folder state
}

void MainWindow::restoreSettings() {
    QSettings settings("xAI", "RestUI");
    restoreGeometry(settings.value("geometry").toByteArray());
    splitter->restoreState(settings.value("splitterSizes").toByteArray());
    restoreFolderState(sidebar->invisibleRootItem()); // Restore folder state
}

void MainWindow::saveFolderState(QTreeWidgetItem *item) {
    QSettings settings("xAI", "RestUI");
    for (int i = 0; i < item->childCount(); ++i) {
        QTreeWidgetItem *child = item->child(i);
        if (child->data(0, Qt::UserRole).toString() == "folder") {
            QString folderName = child->text(0);
            settings.setValue(QString("folderState/%1").arg(folderName), child->isExpanded());
            saveFolderState(child); // Recursively save state of subfolders
        }
    }
}

void MainWindow::restoreFolderState(QTreeWidgetItem *item) {
    QSettings settings("xAI", "RestUI");
    for (int i = 0; i < item->childCount(); ++i) {
        QTreeWidgetItem *child = item->child(i);
        if (child->data(0, Qt::UserRole).toString() == "folder") {
            QString folderName = child->text(0);
            bool isExpanded = settings.value(QString("folderState/%1").arg(folderName), true).toBool();
            child->setExpanded(isExpanded);
            restoreFolderState(child); // Recursively restore state of subfolders
        }
    }
}

QString MainWindow::generateUniqueKey(const QString &folder, const QString &name) {
    return folder.isEmpty() ? name : folder + "/" + name;
}
