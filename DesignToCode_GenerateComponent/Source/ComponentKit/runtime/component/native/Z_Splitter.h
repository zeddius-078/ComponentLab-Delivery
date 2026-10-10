#pragma once

#include <QSplitter>

namespace commonWidget {

// Project-owned component identity for Qt's QSplitter behavior.
class Z_Splitter : public QSplitter {
    Q_OBJECT
  public:
    explicit Z_Splitter(QWidget* parent = nullptr);
};

} // namespace commonWidget
