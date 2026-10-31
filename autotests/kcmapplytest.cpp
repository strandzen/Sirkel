// SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <KPluginFactory>
#include <KPluginMetaData>
#include <KQuickManagedConfigModule>

#include <QKeySequence>
#include <QStandardPaths>
#include <QTest>

// Loads the settings module and checks the Apply/Defaults states. Never
// saves, and runs without a session bus, so it can't touch the real
// shortcuts: KGlobalAccel then has none, as on a first start.
class KcmApplyTest : public QObject
{
    Q_OBJECT

private:
    KQuickManagedConfigModule *m_kcm = nullptr;

    QKeySequence shortcut(const QString &id)
    {
        QKeySequence result;
        QMetaObject::invokeMethod(m_kcm, "shortcut", Q_RETURN_ARG(QKeySequence, result), Q_ARG(QString, id));
        return result;
    }
    void setShortcut(const QString &id, const QKeySequence &sequence)
    {
        QVERIFY(QMetaObject::invokeMethod(m_kcm, "setShortcut", Q_ARG(QString, id), Q_ARG(QKeySequence, sequence)));
    }

private Q_SLOTS:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        const KPluginMetaData data(QStringLiteral(SIRKEL_KCM_PLUGIN));
        QVERIFY2(data.isValid(), SIRKEL_KCM_PLUGIN);
        auto result = KPluginFactory::instantiatePlugin<KQuickManagedConfigModule>(data, this);
        QVERIFY2(result, qPrintable(result.errorText));
        m_kcm = result.plugin;
        m_kcm->load();
        QVERIFY(!m_kcm->needsSave());
    }

    void editsWaitForApply()
    {
        // A setting: needs Apply, and Reset (load) drops it.
        QObject *settings = m_kcm->property("settings").value<QObject *>();
        QVERIFY(settings);
        const int deadzone = settings->property("deadzone").toInt();
        settings->setProperty("deadzone", deadzone + 10);
        QTRY_VERIFY(m_kcm->needsSave());
        m_kcm->load();
        QCOMPARE(settings->property("deadzone").toInt(), deadzone);
        QTRY_VERIFY(!m_kcm->needsSave());
    }

    void shortcutsWaitForApply()
    {
        const QString trigger = m_kcm->property("triggerId").toString();
        setShortcut(trigger, QKeySequence(QStringLiteral("Meta+Z")));
        // The page shows it at once, but it's only stored on Apply.
        QCOMPARE(shortcut(trigger), QKeySequence(QStringLiteral("Meta+Z")));
        QCOMPARE(m_kcm->property("triggerKind").toString(), QStringLiteral("combination"));
        QVERIFY(m_kcm->needsSave());
        // Back to what's stored: nothing to apply.
        setShortcut(trigger, QKeySequence());
        QVERIFY(!m_kcm->needsSave());
        // Reset drops an edit.
        setShortcut(trigger, QKeySequence(QStringLiteral("Meta+Z")));
        m_kcm->load();
        QCOMPARE(shortcut(trigger), QKeySequence());
        QVERIFY(!m_kcm->needsSave());
    }

    // Keys given to another of Sirkel's actions leave the first one.
    void reassigningMovesTheKeys()
    {
        setShortcut(QStringLiteral("SirkelSnapLeft"), QKeySequence(QStringLiteral("Meta+H")));
        setShortcut(QStringLiteral("SirkelSnapRight"), QKeySequence(QStringLiteral("Meta+H")));
        QCOMPARE(shortcut(QStringLiteral("SirkelSnapLeft")), QKeySequence());
        QCOMPARE(shortcut(QStringLiteral("SirkelSnapRight")), QKeySequence(QStringLiteral("Meta+H")));
        m_kcm->load();
    }

    void defaultsWaitForApply()
    {
        // No stored shortcuts here, so the hold keys aren't at their
        // defaults (Enter, Backspace, R) yet.
        QVERIFY(!m_kcm->representsDefaults());
        m_kcm->defaults();
        QCOMPARE(shortcut(QStringLiteral("SirkelHoldMaximize")), QKeySequence(Qt::Key_Return));
        QCOMPARE(shortcut(QStringLiteral("SirkelHoldMinimize")), QKeySequence(Qt::Key_Backspace));
        QCOMPARE(shortcut(QStringLiteral("SirkelHoldRestore")), QKeySequence(Qt::Key_R));
        // The trigger's default is F24 (Caps Lock via keyd), as registered.
        QCOMPARE(m_kcm->property("triggerKind").toString(), QStringLiteral("capslock"));
        QTRY_VERIFY(m_kcm->representsDefaults());
        QVERIFY(m_kcm->needsSave());
        m_kcm->load();
        QVERIFY(!m_kcm->needsSave());
    }
};

// No session bus: nothing reaches the real KGlobalAccel or KWin.
static const bool noBus = qputenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=/nonexistent");

QTEST_MAIN(KcmApplyTest)

#include "kcmapplytest.moc"
