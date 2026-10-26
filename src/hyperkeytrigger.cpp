// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "hyperkeytrigger.h"
#include "globalshortcuts.h"
#include "shortcutactions.h"
#include "sirkel_debug.h"

#include <effect/effecthandler.h>

#include <KGlobalAccel>

#include <QAction>
#include <QKeyEvent>

HyperkeyTrigger::HyperkeyTrigger(KWin::Effect *effect, std::function<bool()> busy)
    : QObject(effect)
    , m_effect(effect)
    , m_busy(std::move(busy))
{
    m_action = new QAction(this);
    m_action->setObjectName(QString::fromLatin1(kTriggerActionId));
    m_action->setText(kTriggerActionText.toString());
    registerGlobalAction(m_action, {QKeySequence(Qt::Key_F24)});
    connect(m_action, &QAction::triggered, this, &HyperkeyTrigger::onTriggered);

    // Kept up to date locally rather than asked for on every press: the
    // KGlobalAccel service runs inside KWin, so a lookup would be a
    // blocking D-Bus call from the compositor to itself.
    setCombo(KGlobalAccel::self()->shortcut(m_action).value(0));
    connect(KGlobalAccel::self(), &KGlobalAccel::globalShortcutChanged, this, [this](QAction *action, const QKeySequence &sequence) {
        if (action == m_action) {
            setCombo(sequence);
        }
    });
}

HyperkeyTrigger::~HyperkeyTrigger()
{
    releaseGrab();
}

void HyperkeyTrigger::setCombo(const QKeySequence &sequence)
{
    m_combo = sequence.isEmpty() ? QKeyCombination(Qt::Key_F24) : sequence[0];
    qCDebug(SIRKEL) << "trigger is" << QKeySequence(m_combo).toString();
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
        || ((mods & Qt::ControlModifier) && key == Qt::Key_Control) || ((mods & Qt::AltModifier) && key == Qt::Key_Alt)
        || ((mods & Qt::ShiftModifier) && key == Qt::Key_Shift);
}

void HyperkeyTrigger::onTriggered()
{
    // The held key auto-repeats; ignore repeats while a session runs.
    if (m_busy()) {
        return;
    }
    Q_EMIT armRequested();
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
