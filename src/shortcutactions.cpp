#include "shortcutactions.h"

const QList<ShortcutAction> &shortcutActions()
{
    using K = WindowAction::Kind;
    static const QList<ShortcutAction> actions{
        {"SirkelSnapLeft", kli18n("Sirkel: Snap to left half"), kli18n("Left half"), {K::Place, SnapDirection::Left}},
        {"SirkelSnapRight", kli18n("Sirkel: Snap to right half"), kli18n("Right half"), {K::Place, SnapDirection::Right}},
        {"SirkelSnapTop", kli18n("Sirkel: Snap to top half"), kli18n("Top half"), {K::Place, SnapDirection::Top}},
        {"SirkelSnapBottom", kli18n("Sirkel: Snap to bottom half"), kli18n("Bottom half"), {K::Place, SnapDirection::Bottom}},
        {"SirkelSnapTopLeft", kli18n("Sirkel: Snap to top-left quarter"), kli18n("Top-left quarter"), {K::Place, SnapDirection::TopLeft}},
        {"SirkelSnapTopRight", kli18n("Sirkel: Snap to top-right quarter"), kli18n("Top-right quarter"), {K::Place, SnapDirection::TopRight}},
        {"SirkelSnapBottomLeft", kli18n("Sirkel: Snap to bottom-left quarter"), kli18n("Bottom-left quarter"), {K::Place, SnapDirection::BottomLeft}},
        {"SirkelSnapBottomRight", kli18n("Sirkel: Snap to bottom-right quarter"), kli18n("Bottom-right quarter"), {K::Place, SnapDirection::BottomRight}},
        {"SirkelMaximize", kli18n("Sirkel: Maximize"), kli18n("Maximize"), {K::Place, SnapDirection::Maximize}},
        {"SirkelMinimize", kli18n("Sirkel: Minimize"), kli18n("Minimize"), {K::Minimize}},
    };
    return actions;
}
