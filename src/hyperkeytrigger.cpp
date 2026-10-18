#include "hyperkeytrigger.h"
#include "sirkelconfig.h"

#include <effect/effecthandler.h>

#include <KGlobalAccel>
#include <KLocalizedString>

#include <QAction>
#include <QKeyEvent>
#include <QLoggingCategory>
#include <QTimer>

Q_DECLARE_LOGGING_CATEGORY(SIRKEL)

HyperkeyTrigger::HyperkeyTrigger(KWin::Effect *effect, std::function<bool()> busy)
    : QObject(effect)
    , m_effect(effect)
    , m_busy(std::move(busy))
{
    m_action = new QAction(this);
    m_action->setObjectName(QStringLiteral("SirkelSnap"));
    m_action->setText(i18n("Sirkel: hold to snap window"));
    KGlobalAccel::self()->setDefaultShortcut(m_action, {QKeySequence(Qt::Key_F24)});
    // Autoloading (the default): a user's own rebinding wins over F24.
    KGlobalAccel::self()->setShortcut(m_action, {QKeySequence(Qt::Key_F24)});
    connect(m_action, &QAction::triggered, this, &HyperkeyTrigger::onTriggered);

    m_delayTimer = new QTimer(this);
    m_delayTimer->setSingleShot(true);
    connect(m_delayTimer, &QTimer::timeout, this, [this]() {
        m_pending = false;
        Q_EMIT armRequested();
    });
}

HyperkeyTrigger::~HyperkeyTrigger()
{
    releaseGrab();
}

int HyperkeyTrigger::releaseKey() const
{
    const QList<QKeySequence> shortcuts = KGlobalAccel::self()->shortcut(m_action);
    if (!shortcuts.isEmpty() && !shortcuts.first().isEmpty()) {
        return shortcuts.first()[0].key();
    }
    return Qt::Key_F24;
}

bool HyperkeyTrigger::isPending() const
{
    return m_pending;
}

void HyperkeyTrigger::onTriggered()
{
    // The held key auto-repeats; ignore repeats of a running or pending one.
    if (m_pending || m_busy()) {
        return;
    }

    const int delay = SirkelConfig::triggerDelay();
    if (SirkelConfig::triggerMode() == QStringLiteral("delay") && delay > 0) {
        // The grab is needed now to see an early release.
        if (!ensureGrab()) {
            qCWarning(SIRKEL) << "keyboard grab failed, ignoring trigger";
            return;
        }
        m_pending = true;
        m_delayTimer->start(delay);
        return;
    }
    Q_EMIT armRequested();
}

bool HyperkeyTrigger::handlePendingKey(QKeyEvent *event)
{
    if (!m_pending) {
        return false;
    }
    // Releasing early or pressing Esc abandons the trigger.
    if ((event->key() == Qt::Key_Escape && event->type() == QEvent::KeyPress)
        || (event->key() == releaseKey() && event->type() == QEvent::KeyRelease)) {
        cancelPending();
    }
    return true;
}

void HyperkeyTrigger::cancelPending()
{
    m_delayTimer->stop();
    m_pending = false;
    releaseGrab();
}

bool HyperkeyTrigger::ensureGrab()
{
    if (!m_grabbed) {
        m_grabbed = KWin::effects->grabKeyboard(m_effect);
    }
    return m_grabbed;
}

void HyperkeyTrigger::releaseGrab()
{
    if (m_grabbed) {
        m_grabbed = false;
        KWin::effects->ungrabKeyboard();
    }
}
