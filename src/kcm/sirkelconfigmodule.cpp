#include "sirkelconfigmodule.h"
#include "colorresolver.h"
#include "directioninput.h"

#include <KPluginFactory>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDir>
#include <QFile>
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
