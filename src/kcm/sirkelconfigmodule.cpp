// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "sirkelconfigmodule.h"
#include "colorresolver.h"
#include "directioninput.h"
#include "placement.h"
#include "shortcutactions.h"
#include "sirkel_kcm_debug.h"

#include <KConfigGroup>
#include <KConfigWatcher>
#include <KGlobalAccel>
#include <KPluginFactory>
#include <KSharedConfig>

#include <QApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDialog>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QPropertyAnimation>
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
                        &SirkelConfig::IndicatorRingColorSourceChanged}) {
        connect(config, signal, this, &SirkelConfigModule::colorsChanged);
    }

    reloadColorSources();
    // A changed Plasma accent shows up while the settings are open.
    m_globalsWatcher = KConfigWatcher::create(KSharedConfig::openConfig(QStringLiteral("kdeglobals")));
    connect(m_globalsWatcher.data(), &KConfigWatcher::configChanged, this, [this]() {
        reloadColorSources();
        Q_EMIT colorsChanged();
    });

    reloadShortcuts();
    fetchWallpaper();
    QDBusConnection::sessionBus().connect(QStringLiteral("org.kde.plasmashell"),
                                          QStringLiteral("/PlasmaShell"),
                                          QStringLiteral("org.kde.PlasmaShell"),
                                          QStringLiteral("wallpaperChanged"),
                                          this,
                                          SLOT(fetchWallpaper()));
    checkKeyd();
    checkEffect();
}

SirkelConfig *SirkelConfigModule::settings() const
{
    return SirkelConfig::self();
}

QColor SirkelConfigModule::resolvedOutlineColor() const
{
    return resolveColor(ColorTarget::Outline);
}

QColor SirkelConfigModule::resolvedPointerColor() const
{
    return resolveColor(ColorTarget::Pointer);
}

QColor SirkelConfigModule::resolvedRingColor() const
{
    return resolveColor(ColorTarget::Ring);
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

QString SirkelConfigModule::kwinBuildVersion() const
{
    return QStringLiteral(SIRKEL_KWIN_VERSION);
}

QString SirkelConfigModule::kwinInstalledVersion() const
{
    return m_kwinInstalledVersion;
}

QString SirkelConfigModule::effectStatus() const
{
    return m_effectStatus;
}

QString SirkelConfigModule::triggerId() const
{
    return QString::fromLatin1(kTriggerActionId);
}

bool SirkelConfigModule::triggerIsHyperkey() const
{
    return shortcut(triggerId()) == QKeySequence(Qt::Key_F24);
}

QString SirkelConfigModule::triggerKind() const
{
    const QKeySequence trigger = shortcut(triggerId());
    if (trigger.isEmpty()) {
        return QStringLiteral("none");
    }
    return trigger == QKeySequence(Qt::Key_F24) ? QStringLiteral("capslock") : QStringLiteral("combination");
}

QString SirkelConfigModule::triggerText() const
{
    const QString kind = triggerKind();
    if (kind == QLatin1String("none")) {
        return QString();
    }
    return kind == QLatin1String("capslock") ? i18n("Caps Lock") : shortcut(triggerId()).toString(QKeySequence::NativeText);
}

QVariantList SirkelConfigModule::shortcutActions() const
{
    QVariantList list;
    for (const ShortcutAction &action : ::shortcutActions()) {
        list.append(QVariantMap{{QStringLiteral("id"), QString::fromLatin1(action.id)}, {QStringLiteral("text"), translatedText(action.label)}});
    }
    return list;
}

QVariantList SirkelConfigModule::holdKeyActions() const
{
    QVariantList list;
    for (const HoldKeyAction &action : ::holdKeyActions()) {
        list.append(QVariantMap{{QStringLiteral("id"), QString::fromLatin1(action.id)}, {QStringLiteral("text"), translatedText(action.label)}});
    }
    return list;
}

int SirkelConfigModule::shortcutsRevision() const
{
    return m_shortcutsRevision;
}

QKeySequence SirkelConfigModule::shortcut(const QString &actionId) const
{
    return m_shortcuts.value(actionId);
}

void SirkelConfigModule::reloadShortcuts()
{
    // Each lookup is a blocking D-Bus call, so they're read once here (on
    // open and after every change) rather than from every QML binding.
    const auto read = [this](const QString &id) {
        m_shortcuts.insert(id, KGlobalAccel::self()->globalShortcut(QString::fromLatin1(kShortcutComponent), id).value(0));
    };
    m_shortcuts.clear();
    read(triggerId());
    for (const ShortcutAction &action : ::shortcutActions()) {
        read(QString::fromLatin1(action.id));
    }
    for (const HoldKeyAction &action : ::holdKeyActions()) {
        read(QString::fromLatin1(action.id));
    }
}

void SirkelConfigModule::setShortcut(const QString &actionId, const QKeySequence &sequence)
{
    // The actions belong to KWin's process; setForeignShortcut is
    // KGlobalAccel's way to change another process's shortcut. KWin's
    // QAction is updated live.
    QString text;
    if (actionId == triggerId()) {
        text = translatedText(kTriggerActionText);
    } else {
        for (const ShortcutAction &action : ::shortcutActions()) {
            if (actionId == QLatin1String(action.id)) {
                text = translatedText(action.text);
            }
        }
        for (const HoldKeyAction &action : ::holdKeyActions()) {
            if (actionId == QLatin1String(action.id)) {
                text = translatedText(action.text);
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
    message << QStringList{QString::fromLatin1(kShortcutComponent), actionId, QString::fromLatin1(kShortcutComponentName), text} << QVariant::fromValue(keys);
    const QDBusMessage reply = QDBusConnection::sessionBus().call(message);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qCWarning(SIRKEL_KCM) << "couldn't set the shortcut for" << actionId << reply.errorMessage();
    }
    // Re-read all: a reassignment can take another action's keys too.
    reloadShortcuts();
    ++m_shortcutsRevision;
    Q_EMIT shortcutsChanged();
}

void SirkelConfigModule::useHyperkeyTrigger()
{
    setShortcut(triggerId(), QKeySequence(Qt::Key_F24));
}

QUrl SirkelConfigModule::wallpaperUrl() const
{
    return m_wallpaperUrl;
}

namespace
{
// The wallpaper's "Image" setting is a file, or a wallpaper package folder
// (use its largest image).
QUrl wallpaperImage(const QString &image)
{
    const QString path = QUrl::fromUserInput(image).toLocalFile();
    if (path.isEmpty()) {
        return {};
    }
    if (QFileInfo(path).isFile()) {
        return QUrl::fromLocalFile(path);
    }
    QString best;
    qint64 bestSize = -1;
    QDirIterator it(path + QStringLiteral("/contents"),
                    {QStringLiteral("*.jpg"), QStringLiteral("*.jpeg"), QStringLiteral("*.png"), QStringLiteral("*.webp")},
                    QDir::Files,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QFileInfo info(it.next());
        if (info.size() > bestSize) {
            bestSize = info.size();
            best = info.filePath();
        }
    }
    return best.isEmpty() ? QUrl() : QUrl::fromLocalFile(best);
}
}

void SirkelConfigModule::fetchWallpaper()
{
    // Plasma's own API for the wallpaper of a screen (0 = the primary one).
    QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.plasmashell"),
                                                          QStringLiteral("/PlasmaShell"),
                                                          QStringLiteral("org.kde.PlasmaShell"),
                                                          QStringLiteral("wallpaper"));
    message << 0u;
    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(message), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        const QDBusPendingReply<QVariantMap> reply = *watcher;
        if (reply.isError()) {
            qCDebug(SIRKEL_KCM) << "no wallpaper from Plasma:" << reply.error().message();
            return;
        }
        const QVariantMap wallpaper = reply.value();
        // Slideshows, colors and other plugins have no single image.
        const QUrl url = wallpaper.value(QStringLiteral("wallpaperPlugin")) == QLatin1String("org.kde.image")
            ? wallpaperImage(wallpaper.value(QStringLiteral("Image")).toString())
            : QUrl();
        if (url != m_wallpaperUrl) {
            m_wallpaperUrl = url;
            Q_EMIT wallpaperUrlChanged();
        }
    });
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
    return {
        {QStringLiteral("rect"), placementGeometry(direction, QRectF(0, 0, width, height), SirkelConfig::paddingHorizontal(), SirkelConfig::paddingVertical())},
        {QStringLiteral("ringIndex"), p ? p->ringIndex : -1},
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
    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(message), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        if (watcher->isError()) {
            qCWarning(SIRKEL_KCM) << "KWin didn't reload Sirkel's settings:" << watcher->error().message();
        }
    });
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
        qCDebug(SIRKEL_KCM) << "keyd set up:" << ok;
        if (ok != m_keydOk) {
            m_keydOk = ok;
            Q_EMIT keydOkChanged();
        }
    });
    connect(process, &QProcess::errorOccurred, process, &QObject::deleteLater);
    process->start(QStringLiteral("systemctl"), {QStringLiteral("is-active"), QStringLiteral("--quiet"), QStringLiteral("keyd")});
}

void SirkelConfigModule::checkEffect()
{
    QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KWin"),
                                                          QStringLiteral("/Effects"),
                                                          QStringLiteral("org.kde.kwin.Effects"),
                                                          QStringLiteral("isEffectLoaded"));
    message << QStringLiteral("kwin_effect_sirkel");
    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(message), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        const QDBusPendingReply<bool> reply = *watcher;
        const bool loaded = reply.isValid() && reply.value();

        // The installed KWin, which is what the next login runs.
        auto *process = new QProcess(this);
        const auto decide = [this, process, loaded]() {
            process->deleteLater();
            // "kwin 6.8.1"; empty if kwin_wayland couldn't be run.
            m_kwinInstalledVersion = QString::fromUtf8(process->readAllStandardOutput()).trimmed().section(QLatin1Char(' '), -1);
            const bool enabled =
                KConfigGroup(KSharedConfig::openConfig(QStringLiteral("kwinrc")), QStringLiteral("Plugins")).readEntry("kwin_effect_sirkelEnabled", false);
            qCDebug(SIRKEL_KCM) << "built for KWin" << kwinBuildVersion() << "installed" << m_kwinInstalledVersion << "loaded" << loaded << "enabled"
                                << enabled;
            if (!m_kwinInstalledVersion.isEmpty() && m_kwinInstalledVersion != kwinBuildVersion()) {
                m_effectStatus = QStringLiteral("rebuild");
            } else if (loaded) {
                m_effectStatus = QStringLiteral("running");
            } else if (!enabled) {
                m_effectStatus = QStringLiteral("disabled");
            } else {
                m_effectStatus = QStringLiteral("relogin");
            }
            Q_EMIT effectStatusChanged();
        };
        connect(process, &QProcess::finished, this, decide);
        connect(process, &QProcess::errorOccurred, this, [decide](QProcess::ProcessError error) {
            if (error == QProcess::FailedToStart) {
                decide();
            }
        });
        process->start(QStringLiteral("kwin_wayland"), {QStringLiteral("--version")});
    });
}

K_PLUGIN_CLASS_WITH_JSON(SirkelConfigModule, "kwin_effect_sirkel_config.json")

#include "sirkelconfigmodule.moc"
