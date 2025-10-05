#include "customtreewidget.h"
#include <QDropEvent>

CustomTreeWidget::CustomTreeWidget(QWidget *parent) : QTreeWidget(parent) {}

void CustomTreeWidget::dropEvent(QDropEvent *event) {
    QTreeWidget::dropEvent(event);  // Call base class implementation
    emit itemsReordered();          // Emit custom signal after drop
}
