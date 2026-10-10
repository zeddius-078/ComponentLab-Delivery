#include "CheckBoxGroupController.h"
#include <QEvent>
#include <QTimer>

CheckBoxGroupController::CheckBoxGroupController(commonWidget::Z_CheckBox* s,
                                                 const QVector<commonWidget::Z_CheckBox*>& input,
                                                 QObject* parent)
    : QObject(parent), summary(s) {
    s->setControlled(true);
    for (auto* item : input) {
        items.append(item);
        item->installEventFilter(this);
        connect(item, &QAbstractButton::toggled, this, [this] {
            if (!updating)
                synchronize();
        });
        connect(item, &QObject::destroyed, this,
                [this] { QTimer::singleShot(0, this, [this] { synchronize(); }); });
    }
    connect(s, &commonWidget::Z_CheckBox::checkedChangeRequested, this, [this](bool checked) {
        updating = true;
        for (auto item : items)
            if (item && item->isEnabled())
                item->setChecked(checked);
        updating = false;
        synchronize();
    });
    synchronize();
}
QStringList CheckBoxGroupController::values() const {
    QStringList out;
    for (auto item : items)
        if (item && item->isChecked())
            out << item->objectName();
    return out;
}
void CheckBoxGroupController::synchronize() {
    if (!summary)
        return;
    int editable = 0, selected = 0;
    for (auto item : items)
        if (item && item->isEnabled()) {
            ++editable;
            if (item->isChecked())
                ++selected;
        }
    summary->setEnabled(editable > 0);
    summary->setChecked(editable > 0 && selected == editable);
    summary->setIndeterminate(selected > 0 && selected < editable);
    emit valuesChanged(values());
}
bool CheckBoxGroupController::eventFilter(QObject*, QEvent* e) {
    if (e->type() == QEvent::EnabledChange)
        QTimer::singleShot(0, this, [this] { synchronize(); });
    return false;
}
