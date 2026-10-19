#include "InputGallery.h"
#include "ComponentFactory.h"
#include "FigmaGalleryView.h"
#include "Z_InputField.h"
#include <QFile>
#include <QFontDatabase>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLayout>
#include <QPointer>
#include <QPixmap>
#include <QTimer>
#include <stdexcept>

namespace native_gallery {
namespace {
QRect bounds(const QJsonArray& n) {
    return QRect(qRound(n[3].toDouble()), qRound(n[4].toDouble()), qRound(n[5].toDouble()),
                 qRound(n[6].toDouble()));
}
QString dimension(const QString& name, const QString& key) {
    for (const auto& part : name.split(','))
        if (part.trimmed().startsWith(key + '='))
            return part.section('=', 1).trimmed();
    return {};
}

struct SpecimenTask {
    QPointer<QWidget> group;
    QString setId;
    QString setName;
    QJsonArray variant;
};

class InputGalleryBuilder final : public QObject {
  public:
    InputGalleryBuilder(QVector<SpecimenTask> tasks,
                        std::shared_ptr<commonWidget::ComponentFactory> factory,
                        std::shared_ptr<field::Tokens> actionTokens,
                        FigmaGalleryView* view,
                        QLabel* progress)
        : QObject(view), tasks_(std::move(tasks)), factory_(std::move(factory)),
          actionTokens_(std::move(actionTokens)), view_(view), progress_(progress) {}

    void start() {
        QTimer::singleShot(0, this, [this] { buildNext(); });
    }

  private:
    void buildNext() {
        if (!view_ || next_ >= tasks_.size()) {
            if (view_) {
                view_->setProperty("variantBuiltCount", tasks_.size());
                view_->setProperty("animatedLoadingSpecimens", 0);
                view_->setProperty("variantBuildComplete", true);
            }
            if (progress_)
                progress_->hide();
            deleteLater();
            return;
        }

        const auto task = tasks_[next_];
        try {
            if (task.group)
                createSpecimen(task);
        } catch (const std::exception& error) {
            if (view_) {
                view_->setProperty("variantBuildError",QString::fromUtf8(error.what()));
                view_->setProperty("variantBuildComplete",true);
            }
            if (progress_)
                progress_->setText(QString::fromUtf8("Input 状态样本构建失败：%1")
                                       .arg(QString::fromUtf8(error.what())));
            deleteLater();
            return;
        }
        ++next_;
        if (view_)
            view_->setProperty("variantBuiltCount", next_);
        if (progress_)
            progress_->setText(QString::fromUtf8("正在构建 Input 状态样本 %1 / %2…")
                                   .arg(next_).arg(tasks_.size()));
        // One complete SDK specimen per event-loop turn keeps input, painting
        // and window movement responsive while the large state matrix fills in.
        QTimer::singleShot(1, this, [this] { buildNext(); });
    }

    void createSpecimen(const SpecimenTask& task) {
        const auto variant = task.variant;
        const auto name = variant[1].toString();
        QWidget* specimen = nullptr;
        bool loadingMotion = false;
        if (task.setId == QStringLiteral("1179:666")) {
            auto* action = new field::Action(actionTokens_, task.group);
            // Anatomy specimens use the authored Figma hit target; the
            // canonical Button runtime must not replace it with its native
            // icon-only size during subsequent visual-state syncs.
            action->setProperty("figmaSnapshotGeometry", true);
            // Snapshot anatomy owns the authored rectangle.  Do not let the
            // runtime field-action hit-target minimum participate in the
            // gallery specimen's parent layout height.
            action->setMinimumHeight(0);
            const auto glyph = dimension(name, "Glyph");
            if (glyph.startsWith("Loading")) {
                action->setBusy(true);
                loadingMotion = true;
            } else if (glyph.startsWith("Close"))
                action->setIconSource(actionTokens_->iconSource("clear"));
            else if (glyph.startsWith("Search"))
                action->setIconSource(actionTokens_->iconSource("search"));
            else if (glyph.startsWith("EyeOff"))
                action->setIconSource(actionTokens_->iconSource("hidePassword"));
            else
                action->setIconSource(actionTokens_->iconSource("showPassword"));
            action->setAccessibleName(glyph);
            action->setToolTip(glyph);
            action->setProperty("internalAnatomy", true);
            action->setProperty("publicAdapterImplemented", false);
            action->setEnabled(!glyph.endsWith("Disabled"));
            specimen = action;
        } else {
            auto* input = qobject_cast<commonWidget::Z_InputField*>(
                factory_->create("commonWidget::Z_InputField", task.group));
            if (!input)
                throw std::runtime_error("Input SDK identity missing");
            using Kind = commonWidget::Z_InputField::Kind;
            const bool password = task.setName.contains("Password"),
                       search = task.setName.contains("Search"),
                       area = task.setName.contains("TextArea");
            input->setKind(password ? Kind::Password
                           : search ? Kind::Search
                           : area   ? Kind::TextArea
                                    : Kind::Text);
            input->setAppearance("outlined");
            const auto size = dimension(name, "Size");
            input->setControlSize(size == "Small" ? "sm" : size == "Large" ? "lg" : "md");
            input->setAutoSize(false, 2, 5);
            input->setPlaceholder(password ? "Password"
                                  : search ? "Search"
                                  : area   ? QString::fromUtf8("Describe the context…")
                                           : "Input text");
            const auto state = dimension(name, "State");
            const bool filled = state == "Filled" || dimension(name, "Content") == "Filled";
            input->setValue(task.setId == "1161:16084" ? "secret42"
                            : !filled                   ? ""
                            : search                    ? "ComponentLab"
                            : area                      ? "First line\nSecond line"
                                                        : "Entered value");
            input->setPasswordVisible(dimension(name, "Visibility") == "Revealed");
            input->setAllowClear(search);
            loadingMotion = task.setName.contains("SearchLoading");
            input->setLoading(loadingMotion);
            input->setStatus(state == "Error" ? "error" : "");
            const auto previewState =
                QStringList{"Default", "Hover", "Focus", "Focused", "Error", "Disabled"}
                        .contains(state)
                    ? state
                    : QStringLiteral("Default");
            input->setSnapshotState(previewState);
            input->setEnabled(state != "Disabled");
            specimen = input;
        }
        specimen->setProperty("figmaNodeId", variant[0].toString());
        specimen->setProperty("figmaVariant", name);
        specimen->setProperty("figmaDesignBounds", bounds(variant));
        if (auto* input = qobject_cast<commonWidget::Z_InputField*>(specimen)) {
            // All field-action slots belong to the runtime surface, but this
            // gallery specimen is a fixed Figma rectangle.  Remove their
            // layout minimums here; runtime instances retain the 32px hit
            // target through Action's normal constructor contract.
            for (auto* action : input->findChildren<field::Action*>()) {
                action->setProperty("figmaSnapshotGeometry", true);
                action->setMinimumHeight(0);
                action->setFixedHeight(24);
            }
        }
        specimen->setFixedSize(bounds(variant).size());
        if (auto* input = qobject_cast<commonWidget::Z_InputField*>(specimen))
            input->control()->setControlHeight(bounds(variant).height());
        specimen->move(bounds(variant).topLeft());
        // These children are added after their parent canvas is already shown;
        // Qt does not implicitly show late-created child widgets.
        specimen->show();
        if (auto* input = qobject_cast<commonWidget::Z_InputField*>(specimen)) {
            // The specimen canvas is fixed to Figma's authored rectangle. A
            // runtime action's minimum hit target must not expand that canvas
            // during the first layout pass.
            const int authoredHeight = bounds(variant).height();
            input->control()->setMinimumHeight(0);
            input->control()->setMaximumHeight(authoredHeight);
            input->control()->setFixedHeight(authoredHeight);
            QTimer::singleShot(0, input, [input, authoredHeight] {
                input->control()->setMinimumHeight(0);
                input->control()->setMaximumHeight(authoredHeight);
                input->control()->setFixedHeight(authoredHeight);
            });
        }
        if (auto* action = qobject_cast<field::Action*>(specimen)) {
            const auto authored = bounds(variant);
            QTimer::singleShot(0, action, [action, authored] {
                action->setGeometry(authored);
            });
        }
        if (loadingMotion) {
            // The overview documents the loading appearance but owns no live
            // motion. Capture one authored frame for every loading variant and
            // hide the real SDK source so its animation timer is suspended.
            specimen->ensurePolished();
            if (specimen->layout())
                specimen->layout()->activate();
            const QPixmap frame = specimen->grab();
            if (auto* action = qobject_cast<field::Action*>(specimen))
                action->setBusy(false);
            if (auto* input = qobject_cast<commonWidget::Z_InputField*>(specimen))
                input->setLoading(false);
            auto* snapshot = new QLabel(task.group);
            snapshot->setObjectName("inputStaticMotionSnapshot");
            snapshot->setProperty("figmaNodeId",variant[0].toString());
            snapshot->setProperty("figmaVariant",name);
            snapshot->setProperty("figmaDesignBounds",bounds(variant));
            snapshot->setGeometry(specimen->geometry());
            snapshot->setPixmap(frame);
            snapshot->setAttribute(Qt::WA_TransparentForMouseEvents);
            snapshot->show();
            specimen->setProperty("figmaStaticMotionSource",true);
            specimen->hide();
        }
    }

    QVector<SpecimenTask> tasks_;
    std::shared_ptr<commonWidget::ComponentFactory> factory_;
    std::shared_ptr<field::Tokens> actionTokens_;
    QPointer<FigmaGalleryView> view_;
    QPointer<QLabel> progress_;
    int next_ = 0;
};
} // namespace
QWidget* createInputGallery(const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                            QWidget* parent) {
    QFile file(":/figma/gallery-layouts.json");
    if (!file.open(QIODevice::ReadOnly))
        throw std::runtime_error("Input gallery evidence missing");
    QJsonArray gallery;
    for (const auto& page : QJsonDocument::fromJson(file.readAll()).object()["pages"].toArray())
        if (page.toObject()["id"] == "339:2")
            gallery = page.toObject()["galleries"].toArray()[0].toArray();
    if (gallery.isEmpty())
        throw std::runtime_error("Input gallery missing");
    QFontDatabase::addApplicationFont(":/figma/fonts/Inter-SemiBold.otf");
    auto* canvas = new QWidget;
    canvas->setObjectName("figmaGalleryCanvas");
    canvas->setFixedSize(bounds(gallery).size());
    canvas->setProperty("galleryNodeId", gallery[0].toString());
    canvas->setProperty("componentSetCount", 7);
    canvas->setStyleSheet("QWidget#figmaGalleryCanvas {background:white;border-radius:16px;}");
    auto actionTokens = std::make_shared<field::Tokens>("standard-light");
    QVector<SpecimenTask> tasks;
    for (const auto& child : gallery[7].toArray()) {
        const auto node = child.toArray();
        auto* group = new QWidget(canvas);
        group->setGeometry(bounds(node));
        if (node[1] == "Variant key") {
            group->setObjectName("variantKey");
            group->setStyleSheet("QWidget#variantKey {background:#f7f9fc;border-radius:10px;}");
            for (const auto& item : node[7].toArray()) {
                const auto key = item.toArray();
                auto* label = new QLabel(key[7].toString(), group);
                label->setGeometry(bounds(key));
                label->setStyleSheet("background:transparent;border:0;color:#505868;font-family:'"
                                     "Inter';font-size:12px;");
                QFont font("HarmonyOS Sans");
                font.setPixelSize(12);
                font.setWeight(QFont::DemiBold);
                label->setFont(font);
            }
            continue;
        }
        group->setProperty("figmaComponentSetId", node[0].toString());
        group->setObjectName("inputSet_" + node[0].toString().replace(':', '_'));
        for (const auto& entry : node[7].toArray())
            tasks.append({group, node[0].toString(), node[1].toString(), entry.toArray()});
    }
    canvas->setProperty("variantSpecimenCount", tasks.size());
    auto* progress = new QLabel(canvas);
    progress->setObjectName("inputGalleryBuildProgress");
    progress->setGeometry(24, 24, 340, 36);
    progress->setAlignment(Qt::AlignCenter);
    progress->setStyleSheet("background:rgba(255,255,255,230);border:1px solid #d9d9e3;"
                            "border-radius:8px;color:#444;padding:6px;");
    progress->setText(QString::fromUtf8("正在构建 Input 状态样本 0 / %1…").arg(tasks.size()));
    progress->raise();
    auto* view = new FigmaGalleryView(canvas, parent);
    view->setProperty("variantBuiltCount", 0);
    view->setProperty("variantBuildComplete", false);
    view->setProperty("variantBuildError", QString());
    view->setProperty("variantBuildBatchSize", 1);
    view->setProperty("animatedLoadingSpecimens", 0);
    (new InputGalleryBuilder(std::move(tasks), factory, actionTokens, view, progress))->start();
    return view;
}
} // namespace native_gallery
