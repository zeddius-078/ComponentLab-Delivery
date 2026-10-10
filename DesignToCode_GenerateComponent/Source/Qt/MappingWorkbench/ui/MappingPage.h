#pragma once
#include <QObject>
#include <QJsonObject>
#include <QMap>
class QWidget;
class QPlainTextEdit;
class QScrollArea;
class QTreeWidget;
class QComboBox;
class QLabel;
class QPushButton;
namespace design_mapping {
class MappingPage : public QObject {
    Q_OBJECT
  public:
    MappingPage(const QString& profile, QWidget* flowPage, QWidget* filesPage, QObject* parent);
    bool applyContract();

  private:
    QString profile;
    QJsonObject current;
    QMap<QString, QByteArray> files;
    QPlainTextEdit* editor;
    QPlainTextEdit* output;
    QScrollArea* previewArea;
    QTreeWidget* tree;
    QTreeWidget* details;
    QComboBox* fileChoice;
    QLabel* status;
    QLabel* interaction;
    QPushButton* exportButton;
    void selectFile();
    void populateTree();
};
} // namespace design_mapping
