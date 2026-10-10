#pragma once
#include <QRegularExpression>
#include <QSet>
#include <QString>
#include <stdexcept>

namespace v2 {
// 工程身份必须独立于设计 GUID；无法安全推导时要求维护显式映射。
inline QString engineeringName(const QString& designName, const QString& mappedName) {
    QString name = mappedName;
    if (name.isEmpty()) {
        const auto parts = designName.split(QRegularExpression("[^A-Za-z0-9]+"),
                                            Qt::SkipEmptyParts);
        for (const auto& part : parts)
            name += name.isEmpty() ? part.left(1).toLower() + part.mid(1)
                                  : part.left(1).toUpper() + part.mid(1);
        // 中文名不能通过丢弃文字只留下数字来伪造工程语义。
        if (designName.contains(QRegularExpression("[^\\x00-\\x7F]")))
            throw std::runtime_error("ENGINEERING_NAME_MAPPING_REQUIRED");
    }
    static const QSet<QString> reserved = [] {
        QSet<QString> result;
        const auto words = QStringLiteral(
            "alignas alignof and and_eq asm atomic_cancel atomic_commit atomic_noexcept auto "
            "bitand bitor bool break case catch char char16_t char32_t class compl concept const "
            "constexpr const_cast continue co_await co_return co_yield decltype default delete "
            "do double dynamic_cast else enum explicit export extern false float for friend "
            "goto if inline int long mutable namespace new noexcept not not_eq nullptr operator "
            "or or_eq private protected public register reinterpret_cast requires return short "
            "signed sizeof static static_assert static_cast struct switch synchronized template "
            "this thread_local throw true try typedef typeid typename union unsigned using "
            "virtual void volatile wchar_t while xor xor_eq signals slots emit").split(' ');
        for (const auto& word : words)
            result.insert(word);
        return result;
    }();
    if (!QRegularExpression("^[a-z][A-Za-z0-9]*$").match(name).hasMatch() ||
        reserved.contains(name))
        throw std::runtime_error(("INVALID_ENGINEERING_NAME: " + name).toStdString());
    return name;
}
} // namespace v2
