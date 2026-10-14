#pragma once

#include <QAbstractButton>
#include <QMap>
#include <QPair>
#include <QPixmap>
#include <QStringList>
#include <QTableWidget>
#include <QVariantMap>
#include <QVector>
#include <QWidget>

class QFormLayout;
class QGridLayout;
class QButtonGroup;
class QHBoxLayout;
class QLabel;
class QMouseEvent;
class QPaintEvent;
class QTimer;
class QToolButton;
class QVBoxLayout;

namespace commonWidget {

// Shared implementation boundary for the Figma-owned composite components.
// The complete Figma tuple is retained verbatim and is also exposed as a map,
// so adding an axis in Figma does not silently discard design evidence.
class Z_DesignComponent : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString variantTuple READ variantTuple WRITE setVariantTuple NOTIFY variantChanged)
    Q_PROPERTY(QVariantMap variantProperties READ variantProperties NOTIFY variantChanged)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY contentChanged)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY contentChanged)
    Q_PROPERTY(QString valueText READ valueText WRITE setValueText NOTIFY contentChanged)
    Q_PROPERTY(QStringList items READ items WRITE setItems NOTIFY contentChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(int expandedIndex READ expandedIndex WRITE setExpandedIndex NOTIFY expandedIndexChanged)
    Q_PROPERTY(bool open READ isOpen WRITE setOpen NOTIFY openChanged)

  public:
    explicit Z_DesignComponent(const QString& componentName, QWidget* parent = nullptr);

    QString componentName() const;
    QString variantTuple() const;
    QVariantMap variantProperties() const;
    QString variantValue(const QString& axis) const;
    QString title() const;
    QString description() const;
    QString valueText() const;
    QStringList items() const;
    int currentIndex() const;
    int expandedIndex() const;
    bool isOpen() const;
    QVector<qreal> values() const;
    QPixmap image() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

  public slots:
    void setVariantTuple(const QString& tuple);
    void setVariantProperty(const QString& axis, const QString& value);
    void setTitle(const QString& title);
    void setDescription(const QString& description);
    void setValueText(const QString& value);
    void setItems(const QStringList& items);
    void setCurrentIndex(int index);
    void setExpandedIndex(int index);
    void setOpen(bool open);
    void setValues(const QVector<qreal>& values);
    void setImage(const QPixmap& image);

  signals:
    void variantChanged(const QString& tuple);
    void contentChanged();
    void currentIndexChanged(int index);
    void expandedIndexChanged(int index);
    void openChanged(bool open);
    void activated(int index);
    void actionTriggered();

  protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

  private:
    void updateRuntimeState();
    QString m_componentName;
    QString m_variantTuple;
    QVariantMap m_variantProperties;
    QString m_title;
    QString m_description;
    QString m_valueText;
    QStringList m_items;
    QVector<qreal> m_values;
    QPixmap m_image;
    int m_currentIndex = 0;
    int m_expandedIndex = 0;
    bool m_open = true;
    QTimer* m_animation = nullptr;
    int m_animationPhase = 0;
};

#define Z_DECLARE_CANVAS_COMPONENT(ClassName) \
class ClassName final : public Z_DesignComponent { \
    Q_OBJECT \
  public: \
    explicit ClassName(QWidget* parent = nullptr); \
};

Z_DECLARE_CANVAS_COMPONENT(Z_MetricCard)
Z_DECLARE_CANVAS_COMPONENT(Z_Chart)
Z_DECLARE_CANVAS_COMPONENT(Z_ImagePanel)
Z_DECLARE_CANVAS_COMPONENT(Z_Empty)
Z_DECLARE_CANVAS_COMPONENT(Z_Statistic)
Z_DECLARE_CANVAS_COMPONENT(Z_Card)
Z_DECLARE_CANVAS_COMPONENT(Z_IntegrationCard)
Z_DECLARE_CANVAS_COMPONENT(Z_Divider)
Z_DECLARE_CANVAS_COMPONENT(Z_Collapse)
Z_DECLARE_CANVAS_COMPONENT(Z_OverlayDrawer)
// Canonical SDK source name. Z_OverlayDrawer remains the implementation type
// for source/binary compatibility and must not create a second catalog identity.
using Z_Drawer = Z_OverlayDrawer;
Z_DECLARE_CANVAS_COMPONENT(Z_Alert)
Z_DECLARE_CANVAS_COMPONENT(Z_Message)
Z_DECLARE_CANVAS_COMPONENT(Z_Notification)
Z_DECLARE_CANVAS_COMPONENT(Z_Spin)
Z_DECLARE_CANVAS_COMPONENT(Z_Result)
Z_DECLARE_CANVAS_COMPONENT(Z_Skeleton)

#undef Z_DECLARE_CANVAS_COMPONENT

class Z_InputField;

// Figma public set ComponentLab/Navbar (380:113). The root is a presentation
// container; search, notification and profile actions own keyboard focus.
class Z_Navbar final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString variantTuple READ variantTuple WRITE setVariantTuple NOTIFY variantChanged)
    Q_PROPERTY(QVariantMap variantProperties READ variantProperties NOTIFY variantChanged)
    Q_PROPERTY(QString density READ density WRITE setDensity NOTIFY densityChanged)
    Q_PROPERTY(QString profileName READ profileName WRITE setProfileName NOTIFY profileNameChanged)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)

  public:
    explicit Z_Navbar(QWidget* parent = nullptr);
    explicit Z_Navbar(const QString& profile, QWidget* parent = nullptr);
    QString variantTuple() const;
    QVariantMap variantProperties() const;
    QString density() const;
    QString profileName() const;
    QString searchText() const;
    bool focusIndicatorEnabled() const;
    Z_InputField* searchInput() const;
    QToolButton* notificationButton() const;
    QToolButton* profileButton() const;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

  public slots:
    void setVariantTuple(const QString& tuple);
    void setVariantProperty(const QString& axis, const QString& value);
    void setDensity(const QString& value);
    void setProfileName(const QString& value);
    void setSearchText(const QString& value);
    void setFocusIndicatorEnabled(bool enabled);

  signals:
    void variantChanged(const QString& tuple);
    void densityChanged(const QString& value);
    void profileNameChanged(const QString& value);
    void searchTextChanged(const QString& value);
    void notificationRequested();
    void profileRequested();
    void focusIndicatorEnabledChanged(bool enabled);

  private:
    void refreshGeometryAndStyle();
    QString m_profile;
    QString m_variantTuple;
    QVariantMap m_variantProperties;
    QString m_density = QStringLiteral("Comfortable");
    QString m_profileName = QStringLiteral("Alex Morgan");
    QHBoxLayout* m_layout = nullptr;
    QLabel* m_brandLogo = nullptr;
    QLabel* m_brandTitle = nullptr;
    Z_InputField* m_search = nullptr;
    QToolButton* m_notification = nullptr;
    QToolButton* m_profileAction = nullptr;
};

// Figma public set ComponentLab/Sidebar (392:216). Internal NavItem variants
// remain implementation anatomy and do not create another SDK identity.
class Z_Sidebar final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString variantTuple READ variantTuple WRITE setVariantTuple NOTIFY variantChanged)
    Q_PROPERTY(QVariantMap variantProperties READ variantProperties NOTIFY variantChanged)
    Q_PROPERTY(QString density READ density WRITE setDensity NOTIFY densityChanged)
    Q_PROPERTY(bool collapsed READ isCollapsed WRITE setCollapsed NOTIFY collapsedChanged)
    Q_PROPERTY(QString activeRoute READ activeRoute WRITE setActiveRoute NOTIFY activeRouteChanged)
    Q_PROPERTY(QStringList routeIds READ routeIds WRITE setRouteIds NOTIFY routesChanged)
    Q_PROPERTY(QStringList routeLabels READ routeLabels WRITE setRouteLabels NOTIFY routesChanged)

  public:
    explicit Z_Sidebar(QWidget* parent = nullptr);
    explicit Z_Sidebar(const QString& profile, QWidget* parent = nullptr);
    QString variantTuple() const;
    QVariantMap variantProperties() const;
    QString density() const;
    bool isCollapsed() const;
    QString activeRoute() const;
    QStringList routeIds() const;
    QStringList routeLabels() const;
    bool focusIndicatorEnabled() const;
    QToolButton* collapseButton() const;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

  public slots:
    void setVariantTuple(const QString& tuple);
    void setVariantProperty(const QString& axis, const QString& value);
    void setDensity(const QString& value);
    void setCollapsed(bool value);
    void setActiveRoute(const QString& value);
    void setRouteIds(const QStringList& values);
    void setRouteLabels(const QStringList& values);
    void setFocusIndicatorEnabled(bool enabled);

  signals:
    void variantChanged(const QString& tuple);
    void densityChanged(const QString& value);
    void collapsedChanged(bool value);
    void activeRouteChanged(const QString& value);
    void routesChanged();
    void routeRequested(const QString& routeId);
    void collapsedChangeRequested(bool collapsed);
    void focusIndicatorEnabledChanged(bool enabled);

  private:
    void rebuildRoutes();
    void refreshGeometryAndStyle();
    void updateSelection();
    QString m_profile;
    QString m_variantTuple;
    QVariantMap m_variantProperties;
    QString m_density = QStringLiteral("Comfortable");
    bool m_collapsed = false;
    QString m_activeRoute = QStringLiteral("dashboard");
    QStringList m_routeIds;
    QStringList m_routeLabels;
    QLabel* m_brandLogo = nullptr;
    QLabel* m_brandTitle = nullptr;
    QToolButton* m_collapseAction = nullptr;
    QWidget* m_routeHost = nullptr;
    QVBoxLayout* m_routeLayout = nullptr;
    QButtonGroup* m_routeGroup = nullptr;
    QVector<QToolButton*> m_routeButtons;
};

// Figma public set ComponentLab/DesktopBottomNavigation (1312:96).
// This is a real Qt composition: text-only QPushButtons in the upper tier,
// separators, and icon-plus-text QToolButtons in the lower tier.
class Z_DesktopBottomNavigation final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString variantTuple READ variantTuple WRITE setVariantTuple NOTIFY variantChanged)
    Q_PROPERTY(QVariantMap variantProperties READ variantProperties NOTIFY variantChanged)
    Q_PROPERTY(QStringList primaryItems READ primaryItems WRITE setPrimaryItems NOTIFY itemsChanged)
    Q_PROPERTY(QStringList primaryIconSources READ primaryIconSources WRITE setPrimaryIconSources NOTIFY itemsChanged)
    Q_PROPERTY(QStringList secondaryItems READ secondaryItems WRITE setSecondaryItems NOTIFY itemsChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(int secondaryIndex READ secondaryIndex WRITE setSecondaryIndex NOTIFY secondaryIndexChanged)
    Q_PROPERTY(QString iconPlacement READ iconPlacement WRITE setIconPlacement NOTIFY variantChanged)

  public:
    explicit Z_DesktopBottomNavigation(QWidget* parent = nullptr);
    explicit Z_DesktopBottomNavigation(const QString& profile, QWidget* parent = nullptr);
    QString variantTuple() const;
    QVariantMap variantProperties() const;
    QStringList primaryItems() const;
    QStringList primaryIconSources() const;
    QStringList secondaryItems() const;
    int currentIndex() const;
    int secondaryIndex() const;
    QString iconPlacement() const;
    bool focusIndicatorEnabled() const;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

  public slots:
    void setVariantTuple(const QString& tuple);
    void setVariantProperty(const QString& axis, const QString& value);
    void setPrimaryItems(const QStringList& items);
    void setPrimaryIconSources(const QStringList& sources);
    void setSecondaryItems(const QStringList& items);
    void setCurrentIndex(int index);
    void setSecondaryIndex(int index);
    void setIconPlacement(const QString& value);
    void setFocusIndicatorEnabled(bool enabled);

  signals:
    void variantChanged(const QString& tuple);
    void itemsChanged();
    void currentIndexChanged(int index);
    void secondaryIndexChanged(int index);
    void primaryActivated(int index);
    void secondaryActivated(int index);
    void focusIndicatorEnabledChanged(bool enabled);

  private:
    void rebuildSecondaryTier();
    void rebuildPrimaryTier();
    void updatePlacement();
    void updateSelection();
    void refreshStyle();
    QString m_profile;
    QString m_variantTuple;
    QVariantMap m_variantProperties;
    QStringList m_primaryItems;
    QStringList m_primaryIconSources;
    QStringList m_secondaryItems;
    QVector<QToolButton*> m_primaryButtons;
    QVector<QAbstractButton*> m_secondaryButtons;
    QWidget* m_secondaryHost = nullptr;
    QWidget* m_primaryHost = nullptr;
    QHBoxLayout* m_secondaryLayout = nullptr;
    QHBoxLayout* m_primaryLayout = nullptr;
    QButtonGroup* m_primaryGroup = nullptr;
    QButtonGroup* m_secondaryGroup = nullptr;
    int m_currentIndex = 0;
    int m_secondaryIndex = 0;
};

class Z_Switch final : public QAbstractButton {
    Q_OBJECT
    Q_PROPERTY(QString variantTuple READ variantTuple WRITE setVariantTuple NOTIFY variantChanged)
    Q_PROPERTY(QVariantMap variantProperties READ variantProperties NOTIFY variantChanged)

  public:
    explicit Z_Switch(QWidget* parent = nullptr);
    QString variantTuple() const;
    QVariantMap variantProperties() const;
    QSize sizeHint() const override;

  public slots:
    void setVariantTuple(const QString& tuple);
    void setVariantProperty(const QString& axis, const QString& value);

  signals:
    void variantChanged(const QString& tuple);

  protected:
    void paintEvent(QPaintEvent* event) override;

  private:
    QString m_variantTuple;
    QVariantMap m_variantProperties;
};

class Z_Chip final : public QAbstractButton {
    Q_OBJECT
    Q_PROPERTY(QString variantTuple READ variantTuple WRITE setVariantTuple NOTIFY variantChanged)
    Q_PROPERTY(QVariantMap variantProperties READ variantProperties NOTIFY variantChanged)

  public:
    explicit Z_Chip(QWidget* parent = nullptr);
    QString variantTuple() const;
    QVariantMap variantProperties() const;
    QSize sizeHint() const override;

  public slots:
    void setVariantTuple(const QString& tuple);
    void setVariantProperty(const QString& axis, const QString& value);

  signals:
    void variantChanged(const QString& tuple);

  protected:
    void paintEvent(QPaintEvent* event) override;

  private:
    QString m_variantTuple;
    QVariantMap m_variantProperties;
};

class Z_DataTable final : public QTableWidget {
    Q_OBJECT
    Q_PROPERTY(QString variantTuple READ variantTuple WRITE setVariantTuple NOTIFY variantChanged)
    Q_PROPERTY(QVariantMap variantProperties READ variantProperties NOTIFY variantChanged)

  public:
    explicit Z_DataTable(QWidget* parent = nullptr);
    QString variantTuple() const;
    QVariantMap variantProperties() const;

  public slots:
    void setVariantTuple(const QString& tuple);
    void setVariantProperty(const QString& axis, const QString& value);

  signals:
    void variantChanged(const QString& tuple);

  private:
    void updateRuntimeState();
    QString m_variantTuple;
    QVariantMap m_variantProperties;
};

class Z_Form final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString variantTuple READ variantTuple WRITE setVariantTuple NOTIFY variantChanged)
    Q_PROPERTY(QVariantMap variantProperties READ variantProperties NOTIFY variantChanged)

  public:
    explicit Z_Form(QWidget* parent = nullptr);
    QString variantTuple() const;
    QVariantMap variantProperties() const;
    void addRow(const QString& label, QWidget* field);

  public slots:
    void setVariantTuple(const QString& tuple);
    void setVariantProperty(const QString& axis, const QString& value);

  signals:
    void variantChanged(const QString& tuple);

  private:
    void updateRuntimeState();
    QFormLayout* m_layout = nullptr;
    QString m_variantTuple;
    QVariantMap m_variantProperties;
};

class Z_Descriptions final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString variantTuple READ variantTuple WRITE setVariantTuple NOTIFY variantChanged)
    Q_PROPERTY(QVariantMap variantProperties READ variantProperties NOTIFY variantChanged)

  public:
    explicit Z_Descriptions(QWidget* parent = nullptr);
    QString variantTuple() const;
    QVariantMap variantProperties() const;
    void addItem(const QString& label, const QString& value);
    void clearItems();

  public slots:
    void setVariantTuple(const QString& tuple);
    void setVariantProperty(const QString& axis, const QString& value);

  signals:
    void variantChanged(const QString& tuple);

  private:
    void rebuild();
    QGridLayout* m_layout = nullptr;
    QList<QPair<QString, QString>> m_items;
    QString m_variantTuple;
    QVariantMap m_variantProperties;
};

} // namespace commonWidget
