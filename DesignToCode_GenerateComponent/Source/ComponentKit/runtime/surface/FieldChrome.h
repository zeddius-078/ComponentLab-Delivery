#pragma once

#include "Z_PushButton.h"
#include <QColor>
#include <QFrame>
#include <QJsonObject>
#include <QJsonValue>
#include <QPixmap>
#include <memory>

namespace field {

QPixmap iconPixmap(const QIcon&, int logicalSize, qreal dpr, const QColor&);

enum class Family { Input, Select };

struct Recipe {
    QColor background;
    QColor border;
    QColor foreground;
    QColor glyph;
    qreal radius = 0;
    qreal borderWidth = 0;
};

struct ActionRecipe {
    QColor background;
    QColor glyph;
    qreal radius = 0;
    int iconSize = 0;
    int hitTarget = 0;
};

// Direct V2 runtime view. It reads the resolved Qt profile and never consumes
// component-private catalogs, templates, aliases or historical Figma captures.
class Tokens {
  public:
    explicit Tokens(const QString& profile);
    int metric(const QString& key) const;
    qreal scalar(const QString& key) const;
    QJsonObject size(const QString& name, Family family = Family::Input) const;
    QColor color(const QString& key) const;
    QString colorText(const QString& key) const;
    QString family() const;
    QString iconSource(const QString& key) const;
    QString sourceHash() const;
    QString profile() const { return profileName; }
    Recipe recipe(Family family, const QString& appearance, const QString& state,
                  bool open = false) const;
    ActionRecipe actionRecipe(const QString& action, const QString& state) const;
    QString qss(const Recipe&, const QString& size, Family family = Family::Input) const;
    const QJsonObject& data() const { return values; }

  private:
    QJsonObject values;
    QJsonObject metadata;
    QString profileName;
    QJsonValue token(const QString& path) const;
    bool hasToken(const QString& path) const;
};

class Chrome : public QFrame {
    Q_OBJECT
  public:
    explicit Chrome(std::shared_ptr<const Tokens>, QWidget* parent = nullptr,
                    Family family = Family::Input);
    ~Chrome() override;
    void setControlSize(const QString&);
    void setStatus(const QString&);
    void setLoading(bool);
    void setAppearance(const QString&);
    void setOpenCondition(bool);
    void setSnapshotState(const QString&); // preview forcing; no separate Token source
    void setControlHeight(int height);
    int controlHeight() const;
    QString controlSize() const { return sizeName; }
    QString visualState() const;
    const Tokens& tokens() const { return *catalog; }

  signals:
    void visualChanged();

  protected:
    bool event(QEvent*) override;
    void paintEvent(QPaintEvent*) override;

  private:
    std::shared_ptr<const Tokens> catalog;
    Family familyName;
    QString sizeName = "md", status, appearance = "outlined", snapshotState;
    bool hovered = false, loading = false, open = false;
    void refresh();
};

// Input/Select actions reuse the canonical Button runtime and released Spinner.
class Action : public commonWidget::Z_PushButton {
    Q_OBJECT
    Q_PROPERTY(Role role READ role WRITE setRole)
    Q_PROPERTY(InteractionState interactionState READ interactionState NOTIFY interactionStateChanged)
  public:
    enum class Role { Generic, Clear, PasswordToggle };
    Q_ENUM(Role)
    enum class InteractionState { Default, Hover, Pressed, Disabled };
    Q_ENUM(InteractionState)
    explicit Action(std::shared_ptr<const Tokens>, QWidget* parent = nullptr);
    void setBusy(bool);
    void setIconSource(const QString&);
    void setEnabled(bool);
    void setRole(Role);
    Role role() const { return actionRole; }
    InteractionState interactionState() const;
    bool isBusy() const { return property("loading").toBool(); }

  signals:
    void interactionStateChanged(field::Action::InteractionState state);

  protected:
    bool event(QEvent*) override;
    QString stateStyleOverride(const QString& state) const override;

  private:
    std::shared_ptr<const Tokens> catalog;
    Role actionRole = Role::Generic;
    InteractionState lastInteractionState = InteractionState::Default;
    bool stateRefreshQueued = false;
    void publishInteractionState();
};

} // namespace field

Q_DECLARE_METATYPE(field::Action::InteractionState)
