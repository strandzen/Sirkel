#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# Stress test for the Sirkel effect. Runs a headless KWin (--virtual, two
# 1600x900 outputs, no lock screen: it would otherwise follow your real
# session's lock state) on its own D-Bus session with throwaway config folders,
# loads Sirkel from the build folder, opens test windows and drives the
# trigger with simulated input (sirkel-input.c, KWin's fake input protocol).
# Nothing touches your real session or settings.
#
# Usage: scripts/stress/stress-test.sh [--asan] [--rounds N] [--seed N]
#   (STRESS_TRACE=1 traces every step, for debugging the script itself)
#   --asan     build with AddressSanitizer/UBSan (build-asan/) and load that
#   --rounds   sessions per scenario (default 150)
#   --seed     random seed, to repeat a run exactly
#
# Needs: a configured build (cmake --build build), qml6, wayland-scanner,
# kscreen-doctor, qdbus6. Results end up in build/stress/run-<time>/.

set -u

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
HERE="$ROOT/scripts/stress"

if [ "${1:-}" != "--inner" ]; then
    ASAN=0
    ROUNDS=150
    SEED=$RANDOM
    while [ $# -gt 0 ]; do
        case "$1" in
        --asan) ASAN=1 ;;
        --rounds) ROUNDS=$2; shift ;;
        --seed) SEED=$2; shift ;;
        *) echo "unknown option $1"; exit 2 ;;
        esac
        shift
    done

    OUT="$ROOT/build/stress"
    RUN="$OUT/run-$(date +%Y%m%d-%H%M%S)"
    mkdir -p "$RUN"/{config/QtProject,data,cache,state}

    # The input helper, generated from KWin's fake input protocol.
    PROTO=/usr/share/plasma-wayland-protocols/fake-input.xml
    if [ ! -f "$PROTO" ]; then
        PROTO="$OUT/fake-input.xml"
        [ -f "$PROTO" ] || curl -sfL -o "$PROTO" \
            https://invent.kde.org/libraries/plasma-wayland-protocols/-/raw/master/src/protocols/fake-input.xml \
            || { echo "can't get fake-input.xml (install plasma-wayland-protocols)"; exit 1; }
    fi
    wayland-scanner client-header "$PROTO" "$OUT/fake-input-client-protocol.h"
    wayland-scanner private-code "$PROTO" "$OUT/fake-input-protocol.c"
    gcc -O2 -Wall -I"$OUT" "$HERE/sirkel-input.c" "$OUT/fake-input-protocol.c" \
        $(pkg-config --cflags --libs wayland-client) -o "$OUT/sirkel-input" || exit 1

    if [ "$ASAN" = 1 ]; then
        cmake -S "$ROOT" -B "$ROOT/build-asan" -DCMAKE_BUILD_TYPE=Debug \
            -DECM_ENABLE_SANITIZERS='address;undefined' >/dev/null || exit 1
        cmake --build "$ROOT/build-asan" -j"$(nproc)" >/dev/null || exit 1
        PLUGINS="$ROOT/build-asan/bin"
        # kwin_wayland has a file capability (cap_sys_nice), and Linux ignores
        # LD_PRELOAD for such programs. A plain copy has no capability.
        cp /usr/bin/kwin_wayland "$RUN/kwin_wayland"
        export KWIN_BIN="$RUN/kwin_wayland"
        export LD_PRELOAD_FOR_KWIN
        LD_PRELOAD_FOR_KWIN=$(gcc -print-file-name=libasan.so)
        export ASAN_OPTIONS="detect_leaks=0:halt_on_error=0:log_path=$RUN/asan"
        export UBSAN_OPTIONS="print_stacktrace=1:log_path=$RUN/ubsan"
    else
        PLUGINS="$ROOT/build/bin"
    fi
    [ -f "$PLUGINS/kwin/effects/plugins/kwin_effect_sirkel.so" ] \
        || { echo "no Sirkel build in $PLUGINS; run cmake --build build first"; exit 1; }

    printf '[Plugins]\nkwin_effect_sirkelEnabled=true\n' > "$RUN/config/kwinrc"

    # A private session bus with no service directories: nothing can be
    # auto-started on it. Otherwise the test clients start their own
    # xdg-document-portal, which fights the real session's over
    # /run/user/<uid>/doc and takes it down.
    cat > "$RUN/session-bus.conf" <<'BUS'
<!DOCTYPE busconfig PUBLIC "-//freedesktop//DTD D-Bus Bus Configuration 1.0//EN"
 "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<busconfig>
  <type>session</type>
  <keep_umask/>
  <listen>unix:tmpdir=/tmp</listen>
  <auth>EXTERNAL</auth>
  <policy context="default">
    <allow send_destination="*" eavesdrop="true"/>
    <allow eavesdrop="true"/>
    <allow own="*"/>
  </policy>
</busconfig>
BUS
    echo "Run folder: $RUN (seed $SEED, $ROUNDS rounds per scenario, asan=$ASAN)"

    exec env XDG_CONFIG_HOME="$RUN/config" XDG_DATA_HOME="$RUN/data" \
        XDG_CACHE_HOME="$RUN/cache" XDG_STATE_HOME="$RUN/state" \
        QT_PLUGIN_PATH="$PLUGINS" KWIN_WAYLAND_NO_PERMISSION_CHECKS=1 \
        QT_LOGGING_RULES="sirkel*.debug=true" QT_FORCE_STDERR_LOGGING=1 \
        STRESS_TRACE="${STRESS_TRACE:-}" STRESS_RUN="$RUN" STRESS_ROUNDS="$ROUNDS" STRESS_SEED="$SEED" STRESS_INPUT="$OUT/sirkel-input" \
        QT_NO_XDG_DESKTOP_PORTAL=1 KSCREEN_BACKEND_INPROCESS=1 \
        dbus-run-session --config-file="$RUN/session-bus.conf" -- "$0" --inner
fi

# ---- Inside the private D-Bus session --------------------------------------

[ -n "${STRESS_TRACE:-}" ] && set -x
RUN=$STRESS_RUN
ROUNDS=$STRESS_ROUNDS
RANDOM=$STRESS_SEED
SOCKET=sirkel-stress-$$
LOG="$RUN/kwin.log"
trap '' PIPE

# Evdev key codes.
F24=194 LEFT=105 RIGHT=106 UP=103 DOWN=108 ENTER=28 BACKSPACE=14 ESC=1

env ${LD_PRELOAD_FOR_KWIN:+LD_PRELOAD=$LD_PRELOAD_FOR_KWIN} \
    "${KWIN_BIN:-kwin_wayland}" --virtual --no-lockscreen --socket "$SOCKET" --output-count 2 --width 1600 --height 900 >"$LOG" 2>&1 &
KWIN=$!
for _ in $(seq 100); do [ -S "$XDG_RUNTIME_DIR/$SOCKET" ] && break; sleep 0.1; done
sleep 2

export WAYLAND_DISPLAY=$SOCKET
CLIENT_ENV=(env QT_QPA_PLATFORM=wayland QT_LOGGING_RULES= QT_FORCE_STDERR_LOGGING= LD_PRELOAD=)

coproc INPUT { "$STRESS_INPUT" 2>>"$RUN/input.log"; }

send() { echo "$*" >&"${INPUT[1]}" 2>/dev/null; }
sync_input() {
    send sync
    local line
    read -t 15 -r line <&"${INPUT[0]}" 2>/dev/null
}
alive() { kill -0 "$KWIN" 2>/dev/null; }
rss() { ps -o rss= -p "$KWIN" 2>/dev/null | tr -d ' '; }
qd() { timeout 10 qdbus6 "$@" >/dev/null 2>&1; }
setting() { kwriteconfig6 --file kwinrc --group Effect-sirkel --key "$1" "$2"; }
reconfigure() { qd org.kde.KWin /Effects org.kde.kwin.Effects.reconfigureEffect kwin_effect_sirkel; }
rnd() { echo $(( RANDOM % ($2 - $1 + 1) + $1 )); }

WINDOWS=()
spawn() { "${CLIENT_ENV[@]}" qml6 "$HERE/$1" >/dev/null 2>&1 & WINDOWS+=($!); sleep 0.6; }
respawn_windows() {
    # Never a bare `wait`: with no ids it waits for KWin itself.
    if [ ${#WINDOWS[@]} -gt 0 ]; then
        kill "${WINDOWS[@]}" 2>/dev/null; wait "${WINDOWS[@]}" 2>/dev/null
    fi
    WINDOWS=()
    spawn window-normal.qml; spawn window-fixed.qml; spawn window-normal.qml
}

# One session: press the trigger, a few moves, release.
session() {
    send abs "$(rnd 300 1300)" "$(rnd 200 700)"
    send key $F24 1
    send sleep "$(rnd 0 40)"
    for _ in $(seq "$(rnd 1 4)"); do
        send move "$(rnd -220 220)" "$(rnd -220 220)"
        send sleep "$(rnd 0 30)"
    done
    send key $F24 0
    send sleep "$(rnd 0 60)"
}

RESULTS=()
SCENARIO=""
EXPECT=""
count() { grep -c "$1" "$LOG"; }
# begin <name> [log line the scenario must produce, or it didn't test what
# it's meant to]. Every scenario starts with fresh windows: once a window
# closes, this bare KWin (no Plasma shell) doesn't activate another one.
begin() {
    SCENARIO=$1
    EXPECT=${2:-}
    echo "== $1"
    respawn_windows
    sync_input
    STARTED_BEFORE=$(count "sirkel: session started")
    REFUSED_BEFORE=$(count "sirkel: not starting")
    EXPECT_BEFORE=0
    [ -n "$EXPECT" ] && EXPECT_BEFORE=$(count "$EXPECT")
}
end() {
    sync_input
    if alive; then
        local started refused note=""
        started=$(( $(count "sirkel: session started") - STARTED_BEFORE ))
        refused=$(( $(count "sirkel: not starting") - REFUSED_BEFORE ))
        if [ -n "$EXPECT" ] && [ $(( $(count "$EXPECT") - EXPECT_BEFORE )) -eq 0 ]; then
            RESULTS+=("NOT TESTED  $SCENARIO (never saw: $EXPECT)")
        else
            [ -n "$EXPECT" ] && note=", hit $(( $(count "$EXPECT") - EXPECT_BEFORE ))x"
            RESULTS+=("ok          $SCENARIO ($started sessions, $refused refused$note)")
        fi
    else
        RESULTS+=("CRASH       $SCENARIO")
        echo "KWin died during: $SCENARIO"
        finish
    fi
}

finish() {
    local started snapped cancelled warnings asan
    started=$(grep -c "sirkel: session started" "$LOG")
    snapped=$(grep -c "sirkel: snapping to" "$LOG")
    cancelled=$(grep -c "sirkel: session ended without a snap" "$LOG")
    warnings=$(grep -c "^sirkel.*:.*\(warning\|failed\)\|qt.qml.*\(Sirkel\|Overlay\|Outline\|Indicator\)" "$LOG")
    asan=$(cat "$RUN"/asan* "$RUN"/ubsan* 2>/dev/null | grep -cE "ERROR: AddressSanitizer|runtime error:")
    {
        echo
        echo "Results (seed $STRESS_SEED):"
        printf '  %s\n' "${RESULTS[@]}"
        echo "Sessions started: $started, snapped: $snapped, ended without a snap: $cancelled"
        echo "KWin memory (RSS): start ${RSS_START:-?} kB, end $(rss || echo '-') kB"
        echo "Sirkel/QML warnings in the log: $warnings"
        echo "Sanitizer reports: $asan"
        echo "Log: $LOG"
    } | tee "$RUN/summary.txt"
    kill "$KWIN" ${WINDOWS[@]+"${WINDOWS[@]}"} 2>/dev/null
    exit 0
}

respawn_windows
sync_input
alive || { echo "KWin didn't start, see $LOG"; exit 1; }
for _ in $(seq 20); do session; done
sync_input
RSS_START=$(rss)

begin "rapid sessions ($ROUNDS)"
for _ in $(seq "$ROUNDS"); do session; done
end

begin "interrupted animations (tap and re-press with 0-15 ms gaps)"
for _ in $(seq "$ROUNDS"); do
    send key $F24 1; send move "$(rnd -200 200)" "$(rnd -200 200)"; send key $F24 0
    send sleep "$(rnd 0 15)"
done
end

begin "key mashing (arrows, Enter, Esc, occasional Backspace)"
for _ in $(seq "$ROUNDS"); do
    send key $F24 1
    for _ in $(seq "$(rnd 3 12)"); do
        keys=($LEFT $RIGHT $UP $DOWN $LEFT $RIGHT $UP $DOWN $ENTER $ESC)
        [ $(rnd 0 30) = 0 ] && keys+=($BACKSPACE)
        k=${keys[$(rnd 0 $((${#keys[@]} - 1)))]}
        send key "$k" 1; send sleep "$(rnd 0 10)"; send key "$k" 0
    done
    send key $F24 0
    send sleep "$(rnd 0 30)"
    [ $((RANDOM % 40)) = 0 ] && { sync_input; respawn_windows; }
done
end

begin "window closes mid-session" "target window closed"
for _ in $(seq $((ROUNDS / 5))); do
    "${CLIENT_ENV[@]}" qml6 "$HERE/window-normal.qml" >/dev/null 2>&1 &
    victim=$!
    sleep 0.5
    send abs 800 450; send key $F24 1; send move 200 0; sync_input
    kill -9 $victim 2>/dev/null
    send sleep "$(rnd 0 100)"; send move 0 200; send key $F24 0; sync_input
    wait $victim 2>/dev/null
done
end

begin "effect unloaded and reloaded mid-session" "trigger is"
for _ in $(seq $((ROUNDS / 10))); do
    send key $F24 1; send move 200 200; sync_input
    qd org.kde.KWin /Effects org.kde.kwin.Effects.unloadEffect kwin_effect_sirkel
    send key $F24 0; sync_input
    qd org.kde.KWin /Effects org.kde.kwin.Effects.loadEffect kwin_effect_sirkel
    sleep 0.3
done
end

begin "settings reloaded during sessions"
( while :; do reconfigure; sleep 0.02; done ) &
storm=$!
for _ in $(seq "$ROUNDS"); do session; done
sync_input
kill $storm 2>/dev/null; wait $storm 2>/dev/null
end

begin "extreme settings"
for combo in \
    "IndicatorSize=400 IndicatorRingWidth=60 Deadzone=1 OutlineAnimationDuration=0 PickingMode=angle CenterRadius=5" \
    "IndicatorSize=60 IndicatorRingWidth=2 Deadzone=500 OutlineAnimationDuration=1000 IndicatorFollowsMouse=true" \
    "ShowIndicator=false ShowOutline=false ScrollResizeEnabled=true PaddingHorizontal=120 PaddingVertical=120" \
    "OutlineBlur=true OutlineBorderWidth=0 OutlineFillOpacity=100 IndicatorCornerRadius=0 Easing=overshoot"; do
    for kv in $combo; do setting "${kv%%=*}" "${kv#*=}"; done
    reconfigure
    for _ in $(seq $((ROUNDS / 4))); do session; done
    sync_input
done
rm -f "$RUN/config/kwinrc.lock"
printf '[Plugins]\nkwin_effect_sirkelEnabled=true\n' > "$RUN/config/kwinrc"
reconfigure
end

begin "monitor removed mid-session" "target screen removed"
setting SnapToCursorScreen true; reconfigure
# The output the pointer is moved onto (x 2400): the one starting at x 1600.
# kscreen-doctor's listing order doesn't follow the layout.
second=$(kscreen-doctor -o 2>/dev/null | sed 's/\x1b\[[0-9;]*m//g' \
    | awk '/^Output:/ {name = $3} /Geometry: 1600,/ {print name}')
if [ -n "$second" ]; then
    for _ in $(seq $((ROUNDS / 15 + 1))); do
        send abs 2400 450; send key $F24 1; send move 200 0; sync_input
        timeout 10 kscreen-doctor "output.$second.disable" >/dev/null 2>&1
        send move 0 200; send key $F24 0; sync_input
        timeout 10 kscreen-doctor "output.$second.enable" >/dev/null 2>&1
        sleep 0.5
    done
else
    echo "   (couldn't find a second output; skipped)"
fi
setting SnapToCursorScreen false; reconfigure
end

begin "auto-cancel while held (waits about 11 s)" "session idle for"
send key $F24 1; send move 200 0; sync_input
sleep 11.5
send key $F24 0
end

begin "direct shortcuts invoked rapidly" "sirkel: shortcut"
names=$(timeout 10 qdbus6 org.kde.kglobalaccel /component/sirkel org.kde.kglobalaccel.Component.shortcutNames 2>/dev/null | grep -v '^SirkelSnap$\|^SirkelHold')
for _ in $(seq "$ROUNDS"); do
    for n in $names; do qd org.kde.kglobalaccel /component/sirkel org.kde.kglobalaccel.Component.invokeShortcut "$n"; done
    [ $((RANDOM % 20)) = 0 ] && respawn_windows
done
end

begin "fullscreen window (sessions must be refused)" "not starting"
"${CLIENT_ENV[@]}" qml6 "$HERE/window-fullscreen.qml" >/dev/null 2>&1 &
full=$!
sleep 1
for _ in $(seq $((ROUNDS / 5))); do session; done
sync_input
kill $full 2>/dev/null; wait $full 2>/dev/null
end

begin "Overview opened and closed during sessions"
for _ in $(seq $((ROUNDS / 10))); do
    send key $F24 1; send move 200 0; sync_input
    qd org.kde.kglobalaccel /component/kwin org.kde.kglobalaccel.Component.invokeShortcut Overview
    sleep 0.3
    send key $F24 0; sync_input
    qd org.kde.kglobalaccel /component/kwin org.kde.kglobalaccel.Component.invokeShortcut Overview
    sleep 0.3
    session
done
end

finish
