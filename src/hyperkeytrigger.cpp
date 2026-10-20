#include "hyperkeytrigger.h"
#include "shortcutactions.h"
#include "sirkelconfig.h"

#include <effect/effecthandler.h>

#include <KGlobalAccel>

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
    m_action->setObjectName(QString::fromLatin1(kTriggerActionId));
    m_action->setText(kTriggerActionText.toString());
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

bool HyperkeyTrigger::isRelease(const QKeyEvent *event) const
{
    if (event->type() != QEvent::KeyRelease) {
        return false;
    }
    const int key = event->key();
    if (key == m_combo.key()) {
        return true;
    }
    const Qt::KeyboardModifiers mods = m_combo.keyboardModifiers();
    return ((mods & Qt::MetaModifier) && (key == Qt::Key_Meta || key == Qt::Key_Super_L || key == Qt::Key_Super_R))
        || ((mods & Qt::ControlModifier) && key == Qt::Key_Control)
        || ((mods & Qt::AltModifier) && key == Qt::Key_Alt)
        || ((mods & Qt::ShiftModifier) && key == Qt::Key_Shift);
}

void HyperkeyTrigger::onTriggered()
{
    // The held key auto-repeats; ignore repeats of a running or pending one.
    if (m_pending || m_busy()) {
        return;
    }
    // Once per press rather than on every key release: KGlobalAccel's
    // service runs inside KWin, so each lookup is a D-Bus call to ourselves.
    const QList<QKeySequence> shortcuts = KGlobalAccel::self()->globalShortcut(QStringLiteral("kwin"), QString::fromLatin1(kTriggerActionId));
    m_combo = (!shortcuts.isEmpty() && !shortcuts.first().isEmpty()) ? shortcuts.first()[0] : QKeyCombination(Qt::Key_F24);

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
        || isRelease(event)) {
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
