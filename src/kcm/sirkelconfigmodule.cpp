#include "sirkelconfigmodule.h"
#include "colorresolver.h"
#include "directioninput.h"
#include "placement.h"
#include "shortcutactions.h"

#include <KConfig>
#include <KConfigGroup>
#include <KGlobalAccel>
#include <KPluginFactory>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QDirIterator>
#include <QPropertyAnimation>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QTimer>

SirkelConfigModule::SirkelConfigModule(QObject *parent, const KPluginMetaData &metaData)
    : KQuickManagedConfigModule(parent, metaData)
{
    setButtons(KAbstractConfigModule::Default);
    registerSettings(SirkelConfig::self());

    // Instant apply, debounced so dragging a spin box doesn't write kwinrc
    // and reconfigure KWin on every step.
    m_saveTimer = new QTimer(this);
    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(300);
    connect(m_saveTimer, &QTimer::timeout, this, &SirkelConfigModule::save);
    connect(this, &KAbstractConfigModule::needsSaveChanged, this, [this]() {
        if (needsSave()) {
            m_saveTimer->start();
        }
    });

    SirkelConfig *config = SirkelConfig::self();
    for (auto signal : {&SirkelConfig::OutlineColorChanged,
                        &SirkelConfig::OutlineColorSourceChanged,
                        &SirkelConfig::IndicatorPointerColorChanged,
                        &SirkelConfig::IndicatorPointerColorSourceChanged,
                        &SirkelConfig::IndicatorRingColorChanged,
                        &SirkelConfig::IndicatorRingColorSourceChanged,
                        &SirkelConfig::LinkIndicatorColorsChanged,
                        &SirkelConfig::WallpaperColorPathChanged}) {
        connect(config, signal, this, &SirkelConfigModule::colorsChanged);
    }

    reloadColorSources();
    checkKeyd();
}

SirkelConfig *SirkelConfigModule::settings() const
{
    return SirkelConfig::self();
}

QColor SirkelConfigModule::resolvedOutlineColor() const
{
    return resolveLinkedColor(ColorTarget::Outline);
}

QColor SirkelConfigModule::resolvedPointerColor() const
{
    return resolveLinkedColor(ColorTarget::Pointer);
}

QColor SirkelConfigModule::resolvedRingColor() const
{
    return resolveLinkedColor(ColorTarget::Ring);
}

QStringList SirkelConfigModule::keybindSchemes() const
{
    return ::keybindSchemes();
}

bool SirkelConfigModule::keydOk() const
{
    return m_keydOk;
}

QString SirkelConfigModule::version() const
{
    return metaData().version();
}

QString SirkelConfigModule::triggerId() const
{
    return QString::fromLatin1(kTriggerActionId);
}

bool SirkelConfigModule::triggerIsHyperkey() const
{
    const QKeySequence trigger = shortcut(triggerId());
    return trigger.isEmpty() || trigger == QKeySequence(Qt::Key_F24);
}

QString SirkelConfigModule::triggerText() const
{
    return triggerIsHyperkey() ? i18n("Caps Lock") : shortcut(triggerId()).toString(QKeySequence::NativeText);
}

QVariantList SirkelConfigModule::shortcutActions() const
{
    QVariantList list;
    for (const ShortcutAction &action : ::shortcutActions()) {
        list.append(QVariantMap{{QStringLiteral("id"), QString::fromLatin1(action.id)}, {QStringLiteral("text"), action.label.toString()}});
    }
    return list;
}

int SirkelConfigModule::shortcutsRevision() const
{
    return m_shortcutsRevision;
}

QKeySequence SirkelConfigModule::shortcut(const QString &actionId) const
{
    return KGlobalAccel::self()->globalShortcut(QStringLiteral("kwin"), actionId).value(0);
}

void SirkelConfigModule::setShortcut(const QString &actionId, const QKeySequence &sequence)
{
    // The actions belong to KWin's process; setForeignShortcut is
    // KGlobalAccel's way to change another process's shortcut. KWin's
    // QAction is updated live.
    QString text;
    if (actionId == triggerId()) {
        text = kTriggerActionText.toString();
    } else {
        for (const ShortcutAction &action : ::shortcutActions()) {
            if (actionId == QLatin1String(action.id)) {
                text = action.text.toString();
            }
        }
    }
    if (text.isEmpty()) {
        return;
    }
    QList<int> keys;
    if (!sequence.isEmpty()) {
        keys.append(sequence[0].toCombined());
    }
    QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kglobalaccel"),
                                                          QStringLiteral("/kglobalaccel"),
                                                          QStringLiteral("org.kde.KGlobalAccel"),
                                                          QStringLiteral("setForeignShortcut"));
    message << QStringList{QStringLiteral("kwin"), actionId, QStringLiteral("KWin"), text} << QVariant::fromValue(keys);
    QDBusConnection::sessionBus().call(message);
    ++m_shortcutsRevision;
    Q_EMIT shortcutsChanged();
}

void SirkelConfigModule::useHyperkeyTrigger()
{
    setShortcut(triggerId(), QKeySequence(Qt::Key_F24));
}

QUrl SirkelConfigModule::wallpaperUrl() const
{
    // Desktop containments live in plasmashell's applets config; prefer the
    // one on the primary screen (lastScreen=0) with an image wallpaper.
    const KConfig applets(QStringLiteral("plasma-org.kde.plasma.desktop-appletsrc"), KConfig::SimpleConfig);
    const KConfigGroup containments(&applets, QStringLiteral("Containments"));
    QString image;
    for (const QString &id : containments.groupList()) {
        const KConfigGroup containment = containments.group(id);
        if (containment.readEntry("wallpaperplugin") != QLatin1String("org.kde.image")) {
            continue;
        }
        const QString candidate = containment.group(QStringLiteral("Wallpaper")).group(QStringLiteral("org.kde.image")).group(QStringLiteral("General")).readEntry("Image");
        if (candidate.isEmpty()) {
            continue;
        }
        if (image.isEmpty() || containment.readEntry("lastScreen", -1) == 0) {
            image = candidate;
        }
        if (containment.readEntry("lastScreen", -1) == 0) {
            break;
        }
    }
    if (image.isEmpty()) {
        return {};
    }
    const QString path = QUrl::fromUserInput(image).toLocalFile();
    if (QFileInfo(path).isFile()) {
        return QUrl::fromLocalFile(path);
    }
    // A wallpaper package: use its largest image.
    QString best;
    qint64 bestSize = -1;
    QDirIterator it(path + QStringLiteral("/contents"), {QStringLiteral("*.jpg"), QStringLiteral("*.jpeg"), QStringLiteral("*.png"), QStringLiteral("*.webp")}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QFileInfo info(it.next());
        if (info.size() > bestSize) {
            bestSize = info.size();
            best = info.filePath();
        }
    }
    return best.isEmpty() ? QUrl() : QUrl::fromLocalFile(best);
}

void SirkelConfigModule::resizeHostWindow(qreal pageWidth, qreal targetPageWidth, int duration)
{
    if (!qobject_cast<QApplication *>(QCoreApplication::instance())) {
        return;
    }
    // kcmshell6 and the Desktop Effects configure button both host the page
    // in a KCMultiDialog.
    QDialog *dialog = nullptr;
    for (QWidget *window : QApplication::topLevelWidgets()) {
        if (auto *candidate = qobject_cast<QDialog *>(window); candidate && candidate->isVisible()) {
            dialog = candidate;
            break;
        }
    }
    if (!dialog) {
        return;
    }
    // An absolute target (the page at targetPageWidth plus the dialog's own
    // chrome), never "current + delta": the dialog remembers its size between
    // openings, and a relative change would add up every time. One animation
    // at a time, so quick toggles can't fight over the size.
    if (m_resizeAnimation) {
        m_resizeAnimation->stop();
    }
    const int chrome = dialog->width() - qRound(pageWidth);
    const int target = std::max(dialog->minimumWidth(), chrome + qRound(targetPageWidth));
    auto *animation = new QPropertyAnimation(dialog, "size", dialog);
    animation->setDuration(duration);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    animation->setStartValue(dialog->size());
    animation->setEndValue(QSize(target, dialog->height()));
    m_resizeAnimation = animation;
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

QVariantMap SirkelConfigModule::previewPlacement(const QString &name, qreal width, qreal height) const
{
    const SnapDirection direction = directionFromName(name);
    const Placement *p = placementFor(direction);
    const QPoint edges = placementEdges(direction);
    return {
        {QStringLiteral("rect"), placementGeometry(direction, QRectF(0, 0, width, height), SirkelConfig::paddingHorizontal(), SirkelConfig::paddingVertical())},
        {QStringLiteral("ringIndex"), p ? p->ringIndex : -1},
        {QStringLiteral("label"), placementLabel(direction)},
        {QStringLiteral("edgeH"), edges.x()},
        {QStringLiteral("edgeV"), edges.y()},
    };
}

void SirkelConfigModule::openShortcutsSettings()
{
    QProcess::startDetached(QStringLiteral("systemsettings"), {QStringLiteral("kcm_keys")});
}

void SirkelConfigModule::pickColor(const QString &target, const QColor &color)
{
    if (target == QStringLiteral("outline")) {
        SirkelConfig::setOutlineColor(color);
        SirkelConfig::setOutlineColorSource(QStringLiteral("manual"));
    } else if (target == QStringLiteral("pointer")) {
        SirkelConfig::setIndicatorPointerColor(color);
        SirkelConfig::setIndicatorPointerColorSource(QStringLiteral("manual"));
    } else if (target == QStringLiteral("ring")) {
        SirkelConfig::setIndicatorRingColor(color);
        SirkelConfig::setIndicatorRingColorSource(QStringLiteral("manual"));
    }
}

void SirkelConfigModule::resetColors()
{
    SirkelConfig *config = SirkelConfig::self();
    config->outlineColorItem()->setDefault();
    config->outlineColorSourceItem()->setDefault();
    config->indicatorPointerColorItem()->setDefault();
    config->indicatorPointerColorSourceItem()->setDefault();
    config->indicatorRingColorItem()->setDefault();
    config->indicatorRingColorSourceItem()->setDefault();
    config->linkIndicatorColorsItem()->setDefault();
    config->wallpaperColorPathItem()->setDefault();
}

void SirkelConfigModule::save()
{
    m_saveTimer->stop();
    KQuickManagedConfigModule::save();
    // The effect runs inside kwin_wayland with its own cached copy of the
    // config; this makes it re-read kwinrc.
    QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KWin"),
                                                          QStringLiteral("/Effects"),
                                                          QStringLiteral("org.kde.kwin.Effects"),
                                                          QStringLiteral("reconfigureEffect"));
    message << QStringLiteral("kwin_effect_sirkel");
    QDBusConnection::sessionBus().asyncCall(message);
}

void SirkelConfigModule::checkKeyd()
{
    auto *process = new QProcess(this);
    connect(process, &QProcess::finished, this, [this, process](int exitCode) {
        process->deleteLater();
        bool ok = exitCode == 0;
        if (ok) {
            // A capslock line mentioning f24 anywhere, e.g. "capslock = f24"
            // or "capslock = overload(control, f24)".
            static const QRegularExpression mapping(QStringLiteral("^\\s*capslock\\s*=.*\\bf24\\b"), QRegularExpression::CaseInsensitiveOption);
            const QDir keydDir(QStringLiteral("/etc/keyd"));
            bool anyReadable = false;
            bool mapped = false;
            for (const QString &fileName : keydDir.entryList({QStringLiteral("*.conf")}, QDir::Files)) {
                QFile file(keydDir.filePath(fileName));
                if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    continue;
                }
                anyReadable = true;
                while (!file.atEnd() && !mapped) {
                    const QString line = QString::fromUtf8(file.readLine()).trimmed();
                    mapped = !line.startsWith(QLatin1Char('#')) && mapping.match(line).hasMatch();
                }
            }
            // Unreadable config isn't evidence of a problem.
            ok = mapped || !anyReadable;
        }
        if (ok != m_keydOk) {
            m_keydOk = ok;
            Q_EMIT keydOkChanged();
        }
    });
    connect(process, &QProcess::errorOccurred, process, &QObject::deleteLater);
    process->start(QStringLiteral("systemctl"), {QStringLiteral("is-active"), QStringLiteral("--quiet"), QStringLiteral("keyd")});
}

K_PLUGIN_CLASS_WITH_JSON(SirkelConfigModule, "kwin_effect_sirkel_config.json")

#include "sirkelconfigmodule.moc"
