#include "Z_Select.h"
#include "IconSource.h"
#include "Z_CheckBox.h"
#include <QCache>
#include <QPainter>
#include <QtMath>
#include <QApplication>
#include <QEvent>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QScreen>
#include <QSet>
#include <QSignalBlocker>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>
#include <stdexcept>

namespace commonWidget {
namespace {
constexpr int ValueRole = Qt::UserRole + 1, GroupRole = Qt::UserRole + 2;
class Filter : public QSortFilterProxyModel {
  public:
    using QSortFilterProxyModel::QSortFilterProxyModel;
    QString query;
    bool matches(int row) const {
        auto i = sourceModel()->index(row, 0);
        return i.data().toString().contains(query, Qt::CaseInsensitive) ||
               i.data(ValueRole).toString().contains(query, Qt::CaseInsensitive);
    }
    bool filterAcceptsRow(int row, const QModelIndex&) const override {
        auto i = sourceModel()->index(row, 0);
        if (!i.data(GroupRole).toBool())
            return matches(row);
        for (int r = row + 1; r < sourceModel()->rowCount(); ++r) {
            if (sourceModel()->index(r, 0).data(GroupRole).toBool())
                break;
            if (matches(r))
                return true;
        }
        return false;
    }
};
class RowDelegate : public QStyledItemDelegate {
  public:
    RowDelegate(std::shared_ptr<const field::Tokens> t, QWidget* parent)
        : QStyledItemDelegate(parent), tokens(std::move(t)),
          catalog(std::make_shared<surface::Catalog>(tokens->profile())),
          renderer(new Z_CheckBox(catalog, {}, parent)), cache(16384) {
        renderer->setObjectName("CHECKBOX_ROW_RENDERER");
        renderer->setAttribute(Qt::WA_DontShowOnScreen);
        renderer->hide();
    }
    void setAppearance(const QString& value) {
        if (appearance == value)
            return;
        appearance = value;
        cache.clear();
    }
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        auto s = QStyledItemDelegate::sizeHint(option, index);
        s.setHeight(qMax(tokens->metric("optionHeight"), renderer->sizeHint().height()));
        return s;
    }
    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override {
        if (!index.data(Qt::CheckStateRole).isValid() || index.data(GroupRole).toBool()) {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }
        QStyleOptionViewItem panel(option);
        initStyleOption(&panel, index);
        panel.features &=
            ~(QStyleOptionViewItem::HasCheckIndicator | QStyleOptionViewItem::HasDisplay);
        panel.text.clear();
        auto* style = option.widget ? option.widget->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &panel, painter, option.widget);
        const bool checked = index.data(Qt::CheckStateRole).toInt() == Qt::Checked;
        const bool enabled = option.state.testFlag(QStyle::State_Enabled);
        const bool hover = option.state.testFlag(QStyle::State_MouseOver);
        const int inset = tokens->metric("popupPadding");
        const int width = qMax(1, option.rect.width() - 2 * inset);
        const qreal dpr = painter->device()->devicePixelRatioF();
        const QString key = QString("%1/%2/%3/%4/%5/%6/%7/%8")
                                .arg(width)
                                .arg(option.rect.height())
                                .arg(dpr)
                                .arg(checked)
                                .arg(enabled)
                                .arg(hover)
                                .arg(appearance, index.data().toString());
        QPixmap image;
        if (const auto* cached = cache.object(key)) {
            image = *cached;
        } else {
            renderer->setAppearance(appearance);
            renderer->setEnabled(enabled);
            renderer->setChecked(checked);
            renderer->setText({});
            QEvent state(hover ? QEvent::Enter : QEvent::Leave);
            QApplication::sendEvent(renderer, &state);
            const int overhead =
                renderer->sizeHint().width() + catalog->checkboxMetrics("sm")["spacing"].toInt();
            auto* label = renderer->findChild<QLabel*>("LABEL");
            renderer->setText(label->fontMetrics().elidedText(
                index.data().toString(), Qt::ElideRight, qMax(0, width - overhead)));
            renderer->resize(width, option.rect.height());
            image = QPixmap(qCeil(width * dpr), qCeil(option.rect.height() * dpr));
            image.setDevicePixelRatio(dpr);
            image.fill(Qt::transparent);
            // One real Checkbox supplies all row visuals; the model still owns every value.
            renderer->render(&image);
            cache.insert(key, new QPixmap(image),
                         qMax(1, image.width() * image.height() * 4 / 1024));
        }
        painter->save();
        painter->setClipRect(option.rect);
        painter->drawPixmap(option.rect.topLeft() + QPoint(inset, 0), image);
        painter->restore();
    }

  private:
    std::shared_ptr<const field::Tokens> tokens;
    std::shared_ptr<const surface::Catalog> catalog;
    Z_CheckBox* renderer;
    QString appearance = "solid";
    mutable QCache<QString, QPixmap> cache;
};
} // namespace
struct Z_Select::Impl {
    std::shared_ptr<const field::Tokens> tokens;
    Mode mode = Mode::Single;
    QStringList values;
    QMap<QString, QString> labels;
    QVector<Option> options;
    bool controlled = false, allowClear = true, searchEnabled = true, readOnly = false,
         loading = false;
    bool open = false, syncing = false, tagResizePending = false;
    int maxCount = -1, maxTagCount = 2;
    quint64 requestGeneration = 0;
    QString separators = ",;", placeholder, tagSignature, prefixText;
    QIcon prefixIcon;
    field::Chrome* chrome = nullptr;
    QLabel *label = nullptr, *hint = nullptr, *empty = nullptr, *prefix = nullptr;
    QLineEdit *display = nullptr, *search = nullptr;
    QWidget* tags = nullptr;
    field::Action *clear = nullptr, *arrow = nullptr;
    QVBoxLayout* outer = nullptr;
    QHBoxLayout *row = nullptr, *tagLayout = nullptr;
    QFrame* popup = nullptr;
    QListView* view = nullptr;
    QStandardItemModel* model = nullptr;
    Filter* filter = nullptr;
    RowDelegate* delegate = nullptr;
};
Z_Select::Z_Select(std::shared_ptr<const field::Tokens> tokens, QWidget* parent)
    : QWidget(parent), d(new Impl) {
    d->tokens = std::move(tokens);
    setObjectName("SELECT_ROOT");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    d->outer = new QVBoxLayout(this);
    d->outer->setContentsMargins(0, 0, 0, 0);
    d->outer->setSpacing(d->tokens->metric("labelGap"));
    d->label = new QLabel(this);
    d->label->setObjectName("LABEL");
    d->label->setTextFormat(Qt::PlainText);
    d->label->setWordWrap(true);
    d->hint = new QLabel(this);
    d->hint->setObjectName("HINT");
    d->hint->setTextFormat(Qt::PlainText);
    d->hint->setWordWrap(true);
    d->label->setProperty("fieldPart", "LABEL");
    d->hint->setProperty("fieldPart", "HINT");
    d->chrome = new field::Chrome(d->tokens, this, field::Family::Select);
    d->row = new QHBoxLayout(d->chrome);
    d->row->setSpacing(d->tokens->metric("gap"));
    d->prefix = new QLabel(d->chrome);
    d->prefix->setObjectName("PREFIX");
    d->prefix->setProperty("fieldPart", "PREFIX");
    d->prefix->setTextFormat(Qt::PlainText);
    d->display = new QLineEdit(d->chrome);
    d->display->setObjectName("DISPLAY");
    d->display->setProperty("fieldPart", "EDITOR");
    d->display->setReadOnly(true);
    d->display->setMinimumWidth(0);
    d->display->installEventFilter(this);
    d->tags = new QWidget(d->chrome);
    d->tags->setObjectName("CONTENT");
    d->tags->installEventFilter(this);
    d->tagLayout = new QHBoxLayout(d->tags);
    d->tagLayout->setContentsMargins(0, 0, 0, 0);
    d->tagLayout->setSpacing(d->tokens->metric("tagGap"));
    d->clear = new field::Action(d->tokens, d->chrome);
    d->clear->setObjectName("CLEAR");
    d->clear->setIconSource(d->tokens->iconSource("clear"));
    d->clear->setToolTip(tr("Clear selection"));
    d->clear->setAccessibleName(d->clear->toolTip());
    d->arrow = new field::Action(d->tokens, d->chrome);
    d->arrow->setObjectName("SUFFIX");
    d->arrow->setIconSource(d->tokens->iconSource("arrow"));
    d->arrow->setToolTip(tr("Open options"));
    d->arrow->setAccessibleName(d->arrow->toolTip());
    d->row->addWidget(d->prefix);
    d->row->addWidget(d->display, 1);
    d->row->addWidget(d->tags, 1);
    d->row->addWidget(d->clear);
    d->row->addWidget(d->arrow);
    d->outer->addWidget(d->label);
    d->outer->addWidget(d->chrome);
    d->outer->addWidget(d->hint);
    d->label->setBuddy(d->display);
    setFocusProxy(d->display);
    d->popup = new QFrame(this, Qt::Popup | Qt::FramelessWindowHint);
    d->popup->setObjectName("POPUP");
    d->popup->installEventFilter(this);
    auto* layout = new QVBoxLayout(d->popup);
    int pad = d->tokens->metric("popupPadding");
    layout->setContentsMargins(pad, pad, pad, pad);
    layout->setSpacing(d->tokens->metric("gap"));
    d->search = new QLineEdit(d->popup);
    d->search->setObjectName("SEARCH");
    d->search->setPlaceholderText(tr("Search options"));
    d->search->setAccessibleName(tr("Search options"));
    d->search->installEventFilter(this);
    d->view = new QListView(d->popup);
    d->view->setObjectName("LIST");
    d->view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    d->view->setSelectionMode(QAbstractItemView::SingleSelection);
    d->view->setUniformItemSizes(true);
    d->view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    d->view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->view->setTextElideMode(Qt::ElideRight);
    d->view->installEventFilter(this);
    d->model = new QStandardItemModel(this);
    d->filter = new Filter(this);
    d->filter->setSourceModel(d->model);
    d->view->setModel(d->filter);
    d->delegate = new RowDelegate(d->tokens, d->view);
    d->view->setItemDelegate(d->delegate);
    d->view->setMouseTracking(true);
    d->empty = new QLabel(d->popup);
    d->empty->setObjectName("EMPTY");
    d->empty->setAlignment(Qt::AlignCenter);
    d->empty->setTextFormat(Qt::PlainText);
    layout->addWidget(d->search);
    layout->addWidget(d->view, 1);
    layout->addWidget(d->empty);
    connect(d->search, &QLineEdit::textEdited, this, &Z_Select::setQuery);
    connect(d->view, &QListView::clicked, this, [this](const QModelIndex& i) {
        d->view->setCurrentIndex(i);
        activateCurrent();
    });
    connect(d->arrow, &QAbstractButton::clicked, this, [this] { setOpen(!d->open); });
    connect(d->clear, &QAbstractButton::clicked, this, [this] { propose({}); });
    connect(d->chrome, &field::Chrome::visualChanged, this, [this] {
        if (!d->syncing && d->view)
            sync();
    });
    syncOpenCondition();
    sync();
}
Z_Select::~Z_Select() {
    d->popup->removeEventFilter(this);
    d->display->removeEventFilter(this);
    d->search->removeEventFilter(this);
    d->view->removeEventFilter(this);
    for (auto* child : findChildren<QObject*>())
        disconnect(child, nullptr, this, nullptr);
    d->popup->hide();
}
QStringList Z_Select::values() const {
    return d->values;
}
QStringList Z_Select::labels() const {
    QStringList r;
    for (const auto& v : d->values)
        r << d->labels.value(v, v);
    return r;
}
bool Z_Select::isOpen() const {
    return d->open;
}
field::Chrome* Z_Select::control() const {
    return d->chrome;
}
QListView* Z_Select::optionView() const {
    return d->view;
}
QLineEdit* Z_Select::searchEdit() const {
    return d->search;
}
QWidget* Z_Select::popupWidget() const {
    return d->popup;
}
QSize Z_Select::sizeHint() const {
    return QSize(d->tokens->metric("fieldPreferredWidth"), d->outer->sizeHint().height());
}
void Z_Select::setOptions(const QVector<Option>& options) {
    QSet<QString> seen;
    for (const auto& o : options) {
        if (o.value.isEmpty() || seen.contains(o.value))
            throw std::invalid_argument("Select option values must be nonempty and unique");
        seen.insert(o.value);
    }
    d->options = options;
    d->model->clear();
    QString group;
    for (const auto& o : options) {
        if (group != o.group) {
            group = o.group;
            if (!group.isEmpty()) {
                auto* item = new QStandardItem(group);
                item->setData(true, GroupRole);
                item->setFlags(Qt::ItemIsEnabled);
                d->model->appendRow(item);
            }
        }
        d->labels[o.value] = o.label;
        auto* item = new QStandardItem(o.label);
        item->setData(o.value, ValueRole);
        item->setToolTip(o.label);
        item->setFlags(o.disabled ? Qt::NoItemFlags : Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        d->model->appendRow(item);
    }
    sync();
    moveCurrent(1, true);
}
void Z_Select::setValues(const QStringList& values) {
    QStringList next;
    for (const auto& v : values)
        if (!v.isEmpty() && !next.contains(v))
            next << v;
    if (d->mode == Mode::Single && next.size() > 1)
        throw std::invalid_argument("Single select accepts at most one value");
    bool changed = next != d->values;
    d->values = next;
    sync();
    if (changed)
        emit valuesChanged(next);
}
void Z_Select::propose(const QStringList& values) {
    if (!isEnabled() || d->readOnly || d->loading)
        return;
    if (d->maxCount >= 0 && values.size() > d->maxCount && values.size() > d->values.size()) {
        emit limitReached(d->maxCount);
        return;
    }
    if (!d->controlled)
        setValues(values);
    emit valuesChangeRequested(values);
    sync();
}
void Z_Select::setMode(Mode mode) {
    d->mode = mode;
    if (mode == Mode::Single && d->values.size() > 1)
        setValues({d->values.first()});
    else
        sync();
}
void Z_Select::setControlled(bool on) {
    d->controlled = on;
}
void Z_Select::setControlSize(const QString& v) {
    d->chrome->setControlSize(v);
    sync();
}
void Z_Select::setAppearance(const QString& v) {
    d->tokens->recipe(field::Family::Select, v, QStringLiteral("default"), false);
    d->chrome->setAppearance(v);
    sync();
}
void Z_Select::setStatus(const QString& v) {
    d->chrome->setStatus(v);
}
void Z_Select::setLabel(const QString& v) {
    d->label->setText(v);
    d->display->setAccessibleName(v);
    sync();
}
void Z_Select::setHint(const QString& v) {
    d->hint->setText(v);
    d->display->setAccessibleDescription(v);
    sync();
}
void Z_Select::setPlaceholder(const QString& v) {
    d->placeholder = v;
    d->display->setPlaceholderText(v);
}
void Z_Select::setPrefix(const QString& v) {
    d->prefixText = v;
    d->prefixIcon = {};
    sync();
}
void Z_Select::setPrefixIconSource(const QString& v) {
    d->prefixIcon = IconSource::load(v);
    d->prefixText.clear();
    sync();
}
void Z_Select::setSuffixIconSource(const QString& v) {
    d->arrow->setIconSource(v.isEmpty() ? d->tokens->iconSource("arrow") : v);
}
void Z_Select::setAllowClear(bool on) {
    d->allowClear = on;
    sync();
}
void Z_Select::setSearchEnabled(bool on) {
    d->searchEnabled = on;
    sync();
}
void Z_Select::setReadOnly(bool on) {
    d->readOnly = on;
    if (on)
        setOpen(false);
    sync();
}
void Z_Select::setLoading(bool on) {
    if (on)
        setOpen(false);
    d->loading = on;
    d->chrome->setLoading(on);
    d->arrow->setBusy(on);
    sync();
}
void Z_Select::setMaxCount(int n) {
    if (n < -1)
        throw std::invalid_argument("Invalid maxCount");
    d->maxCount = n;
}
void Z_Select::setMaxTagCount(int n) {
    if (n < 0)
        throw std::invalid_argument("Invalid maxTagCount");
    d->maxTagCount = n;
    syncTags();
}
void Z_Select::setTokenSeparators(const QString& v) {
    d->separators = v;
}
quint64 Z_Select::beginOptionsRequest() {
    setLoading(true);
    return ++d->requestGeneration;
}
bool Z_Select::completeOptionsRequest(quint64 id, const QVector<Option>& options) {
    if (id != d->requestGeneration)
        return false;
    setOptions(options);
    setLoading(false);
    return true;
}
void Z_Select::setQuery(const QString& query) {
    if (d->search->text() != query) {
        QSignalBlocker b(d->search);
        d->search->setText(query);
    }
    if (d->mode == Mode::Tags && !query.isEmpty() && d->separators.contains(query.back())) {
        QStringList next = d->values;
        QString token;
        for (auto c : query) {
            if (d->separators.contains(c)) {
                if (!token.trimmed().isEmpty() && !next.contains(token.trimmed()))
                    next << token.trimmed();
                token.clear();
            } else
                token += c;
        }
        propose(next);
        QSignalBlocker b(d->search);
        d->search->setText(token);
        d->filter->query = token;
    } else
        d->filter->query = query;
    d->filter->invalidate();
    moveCurrent(1, true);
    sync();
    emit searchRequested(d->filter->query);
}
void Z_Select::setOpen(bool on) {
    if (on && (!isEnabled() || d->readOnly || d->loading))
        return;
    if (on == d->open)
        return;
    d->open = on;
    syncOpenCondition();
    if (on) {
        sync();
        positionPopup();
        d->popup->show();
        (d->searchEnabled ? static_cast<QWidget*>(d->search) : d->view)->setFocus();
        moveCurrent(1, true);
    } else {
        d->popup->hide();
        d->display->setFocus();
        sync();
    }
    emit openChanged(on);
}
void Z_Select::syncOpenCondition() {
    setProperty("selectOpen", d->open);
    d->chrome->setOpenCondition(d->open);
    d->arrow->setProperty("selectOpen", d->open);
}
void Z_Select::moveCurrent(int direction, bool end) {
    int count = d->filter->rowCount();
    if (!count)
        return;
    int row = end ? (direction > 0 ? -1 : count) : d->view->currentIndex().row();
    for (int r = row + direction; r >= 0 && r < count; r += direction) {
        auto i = d->filter->index(r, 0);
        if (!(i.flags() & Qt::ItemIsEnabled) || !(i.flags() & Qt::ItemIsSelectable))
            continue;
        d->view->setCurrentIndex(i);
        d->view->scrollTo(i);
        return;
    }
}
void Z_Select::activateCurrent() {
    if (d->loading || d->readOnly)
        return;
    auto i = d->view->currentIndex();
    QString value;
    if (d->mode == Mode::Tags && !d->search->text().trimmed().isEmpty() &&
        (!i.isValid() ||
         i.data().toString().compare(d->search->text().trimmed(), Qt::CaseInsensitive) != 0))
        value = d->search->text().trimmed();
    else {
        if (!i.isValid() || !(i.flags() & Qt::ItemIsEnabled) || !(i.flags() & Qt::ItemIsSelectable))
            return;
        value = i.data(ValueRole).toString();
    }
    auto next = d->values;
    if (d->mode == Mode::Single)
        next = QStringList{value};
    else if (next.contains(value))
        next.removeAll(value);
    else
        next << value;
    propose(next);
    if (d->mode == Mode::Single)
        setOpen(false);
    else {
        setQuery({});
        d->search->setFocus();
    }
}
void Z_Select::positionPopup() {
    auto* screen = QGuiApplication::screenAt(d->chrome->mapToGlobal(d->chrome->rect().center()));
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    if (!screen)
        return;
    auto bounds = screen->availableGeometry();
    const auto& t = *d->tokens;
    int width = qBound(t.metric("popupMinWidth"), d->chrome->width(), bounds.width());
    int rows = qMax(1, d->filter->rowCount());
    int h = qMin(t.metric("popupMaxHeight"),
                 rows * t.metric("optionHeight") + 2 * t.metric("popupPadding") +
                     (d->searchEnabled ? t.metric("actionSize") + t.metric("gap") : 0));
    auto bottom = d->chrome->mapToGlobal(QPoint(0, d->chrome->height()));
    auto top = d->chrome->mapToGlobal(QPoint(0, 0));
    int below = bounds.bottom() - bottom.y() - t.metric("popupGap") + 1,
        above = top.y() - bounds.top() - t.metric("popupGap");
    bool down = below >= h || below >= above;
    h = qMin(h, qMax(0, down ? below : above));
    d->popup->setGeometry(
        qBound(bounds.left(), top.x(), bounds.right() - width + 1),
        down ? bottom.y() + t.metric("popupGap") : top.y() - h - t.metric("popupGap"), width, h);
}
bool Z_Select::eventFilter(QObject* watched, QEvent* e) {
    if (watched == d->tags && e->type() == QEvent::Resize && !d->tagResizePending) {
        d->tagResizePending = true;
        QTimer::singleShot(0, this, [this] {
            d->tagResizePending = false;
            syncTags();
        });
    }
    if (watched == d->popup && e->type() == QEvent::Hide && d->open) {
        d->open = false;
        syncOpenCondition();
        sync();
        emit openChanged(false);
    }
    if (watched == d->display && e->type() == QEvent::MouseButtonPress) {
        setOpen(!d->open);
        return true;
    }
    if (e->type() == QEvent::KeyPress) {
        auto* key = static_cast<QKeyEvent*>(e);
        if (key->key() == Qt::Key_Escape) {
            setOpen(false);
            return true;
        }
        if (key->key() == Qt::Key_Tab || key->key() == Qt::Key_Backtab) {
            if (d->open) {
                setOpen(false);
                focusNextPrevChild(key->key() == Qt::Key_Tab);
                return true;
            }
        }
        if (key->key() == Qt::Key_Down || key->key() == Qt::Key_Up) {
            if (!d->open)
                setOpen(true);
            else
                moveCurrent(key->key() == Qt::Key_Down ? 1 : -1);
            return true;
        }
        if (watched == d->view && (key->key() == Qt::Key_Home || key->key() == Qt::Key_End)) {
            moveCurrent(key->key() == Qt::Key_Home ? 1 : -1, true);
            return true;
        }
        if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter ||
            (watched == d->display && key->key() == Qt::Key_Space)) {
            if (d->open)
                activateCurrent();
            else
                setOpen(true);
            return true;
        }
        if (watched == d->display && key->key() == Qt::Key_Backspace && d->mode != Mode::Single &&
            !d->values.isEmpty()) {
            auto next = d->values;
            next.removeLast();
            propose(next);
            return true;
        }
    }
    return QWidget::eventFilter(watched, e);
}
void Z_Select::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    syncTags();
    if (d->open)
        positionPopup();
}
void Z_Select::changeEvent(QEvent* e) {
    QWidget::changeEvent(e);
    if (d && d->view && e->type() == QEvent::EnabledChange) {
        if (!isEnabled())
            setOpen(false);
        sync();
    }
}
void Z_Select::syncTags() {
    if (!d->tags)
        return;
    const auto signature = QString("%1/%2/%3/%4/%5/%6/%7/%8")
                               .arg(int(d->mode))
                               .arg(d->tags->width())
                               .arg(d->maxTagCount)
                               .arg(d->readOnly)
                               .arg(d->loading)
                               .arg(isEnabled())
                               .arg(d->chrome->controlSize(),
                                    d->values.join(QChar(0x1f)) + labels().join(QChar(0x1e)));
    if (signature == d->tagSignature)
        return;
    d->tagSignature = signature;
    while (auto* item = d->tagLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    if (d->mode == Mode::Single || d->values.isEmpty())
        return;
    const auto& t = *d->tokens;
    QFont font(t.family());
    font.setPixelSize(t.size(d->chrome->controlSize(), field::Family::Select)["fontSize"].toInt());
    QFontMetrics fm(font);
    int remaining = qMax(0, d->tags->width()), shown = 0;
    const int actionWidth = d->clear->sizeHint().width();
    const int tagHeight = qMax(t.metric("tagHeight"), d->clear->sizeHint().height());
    const int reserve = fm.horizontalAdvance(QString("+%1").arg(d->values.size())) +
                        t.metric("tagPadding") * 2 + t.metric("tagGap");
    for (const auto& value : d->values) {
        if (shown >= d->maxTagCount)
            break;
        const auto text = d->labels.value(value, value);
        int desired = fm.horizontalAdvance(text) + t.metric("tagPadding") * 2 + actionWidth;
        int limit = remaining - (shown + 1 < d->values.size() ? reserve : 0);
        if (limit < actionWidth + t.metric("tagPadding") * 3)
            break;
        int width = qMin(desired, limit);
        auto* tag = new QFrame(d->tags);
        tag->setObjectName("TAG");
        tag->setFixedSize(width, tagHeight);
        auto* row = new QHBoxLayout(tag);
        row->setContentsMargins(t.metric("tagPadding"), 0, 0, 0);
        row->setSpacing(0);
        auto* label = new QLabel(
            fm.elidedText(text, Qt::ElideRight, width - actionWidth - t.metric("tagPadding")), tag);
        label->setObjectName("TAG_LABEL");
        label->setTextFormat(Qt::PlainText);
        label->setFont(font);
        label->setToolTip(text);
        auto* remove = new field::Action(d->tokens, tag);
        remove->setObjectName("TAG_REMOVE");
        remove->setIconSource(d->tokens->iconSource("clear"));
        remove->setToolTip(tr("Remove %1").arg(text));
        remove->setAccessibleName(remove->toolTip());
        remove->setEnabled(!d->readOnly && !d->loading);
        connect(remove, &QAbstractButton::clicked, this, [this, value] {
            auto next = d->values;
            next.removeAll(value);
            // Delay rebuilding the tag row until this clicked sender returns.
            QTimer::singleShot(0, this, [this, next] { propose(next); });
        });
        row->addWidget(label, 1);
        row->addWidget(remove);
        d->tagLayout->addWidget(tag);
        remaining -= width + t.metric("tagGap");
        ++shown;
    }
    if (shown < d->values.size()) {
        auto* label = new QLabel(QString("+%1").arg(d->values.size() - shown), d->tags);
        label->setObjectName("TAG_OVERFLOW");
        label->setFont(font);
        label->setToolTip(labels().mid(shown).join("\n"));
        d->tagLayout->addWidget(label);
    }
    d->tagLayout->addStretch();
}
void Z_Select::sync() {
    if (d->syncing)
        return;
    d->syncing = true;
    const auto& t = *d->tokens;
    auto m = t.size(d->chrome->controlSize(), field::Family::Select);
    const QString appearance = d->chrome->property("fieldAppearance").toString();
    const QString css = t.qss(
        t.recipe(field::Family::Select, appearance, d->chrome->visualState(), d->open),
        d->chrome->controlSize(), field::Family::Select);
    if (styleSheet() != css)
        setStyleSheet(css);
    int pad = m["paddingInline"].toInt() + t.metric("borderWidth");
    d->row->setContentsMargins(pad, 0, pad, 0);
    d->display->setFixedHeight(m["lineHeight"].toInt());
    d->prefix->setVisible(!d->prefixText.isEmpty() || !d->prefixIcon.isNull());
    if (d->prefixIcon.isNull())
        d->prefix->setText(d->prefixText);
    else
        d->prefix->setPixmap(field::iconPixmap(d->prefixIcon, t.metric("iconSize"),
                                               devicePixelRatioF(),
                                               t.color(isEnabled() ? "text" : "disabledText")));
    d->display->setText(d->mode == Mode::Single ? labels().join(QString()) : QString());
    bool tags = d->mode != Mode::Single && !d->values.isEmpty();
    d->display->setVisible(!tags);
    d->tags->setVisible(tags);
    d->clear->setVisible(d->allowClear && !d->values.isEmpty() && !d->readOnly && isEnabled());
    d->clear->setEnabled(!d->loading);
    d->arrow->setEnabled(!d->readOnly && !d->loading);
    d->label->setVisible(!d->label->text().isEmpty());
    d->hint->setVisible(!d->hint->text().isEmpty());
    d->search->setVisible(d->searchEnabled);
    auto p = d->display->palette();
    p.setColor(QPalette::PlaceholderText, t.color("placeholder"));
    d->display->setPalette(p);
    for (auto* w : {d->label, d->hint}) {
        QFont f(t.family());
        f.setPixelSize(t.metric(w == d->label ? "labelFontSize" : "hintFontSize"));
        w->setFont(f);
        auto pal = w->palette();
        pal.setColor(QPalette::WindowText, t.color(!isEnabled()    ? "disabledText"
                                                   : w == d->label ? "text"
                                                                   : "muted"));
        w->setPalette(pal);
    }
    const int actionSafeHeight = d->clear->isVisible()
        ? d->clear->sizeHint().height() + 4 * t.metric("borderWidth")
        : d->chrome->controlHeight();
    d->chrome->setControlHeight(
        tags ? qMax(t.metric("tagHeight"), actionSafeHeight) +
                   2 * m["paddingBlock"].toInt()
             : actionSafeHeight);
    QFont f(t.family());
    f.setPixelSize(m["fontSize"].toInt());
    d->popup->setFont(f);
    d->search->setMinimumHeight(t.metric("actionSize"));
    // Multiple-select rows consume the independently governed Checkbox
    // appearance; the Select field appearance must not leak into it.
    d->delegate->setAppearance(QStringLiteral("solid"));
    for (int row = 0; row < d->model->rowCount(); ++row) {
        auto* item = d->model->item(row);
        if (item->data(GroupRole).toBool())
            continue;
        item->setData(d->mode == Mode::Single
                          ? QVariant()
                          : QVariant(d->values.contains(item->data(ValueRole).toString())
                                         ? Qt::Checked
                                         : Qt::Unchecked),
                      Qt::CheckStateRole);
    }
    bool empty = d->filter->rowCount() == 0 || d->loading;
    d->empty->setVisible(empty);
    d->view->setVisible(!empty);
    d->empty->setText(d->loading ? tr("Loading...")
                      : d->mode == Mode::Tags && !d->search->text().trimmed().isEmpty()
                          ? tr("Create: %1").arg(d->search->text().trimmed())
                          : tr("No options"));
    d->empty->setMinimumHeight(t.metric("optionHeight"));
    setMinimumWidth(t.metric("fieldMinimumWidth"));
    syncTags();
    d->syncing = false;
    updateGeometry();
    if (d->open)
        positionPopup();
}
} // namespace commonWidget
