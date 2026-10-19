#pragma once
#include <QWidget>
class MappingGenerated : public QWidget {
public:
    explicit MappingGenerated(const QString& profile, QWidget* parent = nullptr);
    QWidget* mappedControl(const QString& id) const;
};
