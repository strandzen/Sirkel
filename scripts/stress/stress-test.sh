#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Strandzen <strandzen@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

# Stress test for the Sirkel effect. Runs a headless KWin (--virtual, two
# 1600x900 outputs, no lock screen: it would otherwise follow your real
# session's lock state) on its own D-Bus session with throwaway config folders,
# loads Sirkel from the build folder, opens test windows and drives the
# trigger with simulated input (sirkel-input.c, KWin's fake input protocol).
# Window positions are read back with a small KWin script (probe.js), so the
# scenarios check where windows actually end up, not only that KWin survives.
# The last scenario restarts KWin locked, to check nothing works behind the
# lock screen. Nothing touches your real session or settings.
#
# Usage: scripts/stress/stress-test.sh [--asan] [--rounds N] [--seed N]
#   (STRESS_TRACE=1 traces every step, for debugging the script itself)
#   --asan     build with AddressSanitizer/UBSan (build-asan/) and load that
#   --rounds   sessions per scenario (default 150)
#   --seed     random seed, to repeat a run exactly
#
# Exits with 1 if anything crashed, failed a check, didn't test what it should,
# or logged a warning or sanitizer report (so CI can use it).
#
# Needs: a configured build (cmake --build build), qml6, wayland-scanner,
# kscreen-doctor, qdbus6, dbus-send. Results end up in build/stress/run-<time>/.

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
        export LD_PRELOAD_FOR_KWIN
        LD_PRELOAD_FOR_KWIN=$(gcc -print-file-name=libasan.so)
        export ASAN_OPTIONS="detect_leaks=0:halt_on_error=0:log_path=$RUN/asan"
        export UBSAN_OPTIONS="print_stacktrace=1:log_path=$RUN/ubsan"
    else
        PLUGINS="$ROOT/build/bin"
    fi
    [ -f "$PLUGINS/kwin/effects/plugins/kwin_effect_sirkel.so" ] \
        || { echo "no Sirkel build in $PLUGINS; run cmake --build build first"; exit 1; }

    # Run a plain copy of kwin_wayland. The real one has a file capability
    # (cap_sys_nice): Linux ignores LD_PRELOAD for such programs (--asan), and
    # refuses to start them where the capability can't be granted (e.g. in a
    # CI container). The test KWin doesn't need real-time scheduling.
    cp "$(command -v kwin_wayland)" "$RUN/kwin_wayland"
    export KWIN_BIN="$RUN/kwin_wayland"

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

    # js: the probe script's output.
    exec env XDG_CONFIG_HOME="$RUN/config" XDG_DATA_HOME="$RUN/data" \
        XDG_CACHE_HOME="$RUN/cache" XDG_STATE_HOME="$RUN/state" \
        QT_PLUGIN_PATH="$PLUGINS" KWIN_WAYLAND_NO_PERMISSION_CHECKS=1 \
        QT_LOGGING_RULES="sirkel*.debug=true;js=true" QT_FORCE_STDERR_LOGGING=1 \
        STRESS_TRACE="${STRESS_TRACE:-}" STRESS_RUN="$RUN" STRESS_ROUNDS="$ROUNDS" STRESS_SEED="$SEED" STRESS_INPUT="$OUT/sirkel-input" \
        QT_NO_XDG_DESKTOP_PORTAL=1 KSCREEN_BACKEND_INPROCESS=1 \
        dbus-run-session --config-file="$RUN/session-bus.conf" -- "$0" --inner
fi

# ---- Inside the private D-Bus session --------------------------------------

[ -n "${STRESS_TRACE:-}" ] && set -x
RUN=$STRESS_RUN
ROUNDS=$STRESS_ROUNDS
RANDOM=$STRESS_SEED
LOG="$RUN/kwin.log"
STATUS=0
trap '' PIPE

# Evdev key and button codes.
F24=194 LEFT=105 RIGHT=106 UP=103 DOWN=108 ENTER=28 BACKSPACE=14 ESC=1 META=125 SPACE=57 KEY_R=19
BTN_LEFT=272 BTN_RIGHT=273
# Qt key codes, as the key-logging test window prints them.
QT_RETURN=16777220 QT_BACKSPACE=16777219 QT_R=82

CLIENT_ENV=(env QT_QPA_PLATFORM=wayland QT_LOGGING_RULES= QT_FORCE_STDERR_LOGGING= LD_PRELOAD=)
KWIN_STARTS=0

# start_kwin [extra kwin_wayland options]: a fresh test KWin and an input
# helper connected to it.
start_kwin() {
    SOCKET="sirkel-stress-$$-$((++KWIN_STARTS))"
    env ${LD_PRELOAD_FOR_KWIN:+LD_PRELOAD=$LD_PRELOAD_FOR_KWIN} \
        "${KWIN_BIN:-kwin_wayland}" --virtual "$@" --socket "$SOCKET" --output-count 2 --width 1600 --height 900 >>"$LOG" 2>&1 &
    KWIN=$!
    for _ in $(seq 100); do [ -S "$XDG_RUNTIME_DIR/$SOCKET" ] && break; sleep 0.1; done
    sleep 2
    export WAYLAND_DISPLAY=$SOCKET
    coproc INPUT { "$STRESS_INPUT" 2>>"$RUN/input.log"; }
}

stop_kwin() {
    close_windows
    kill "$INPUT_PID" 2>/dev/null; wait "$INPUT_PID" 2>/dev/null
    kill "$KWIN" 2>/dev/null; wait "$KWIN" 2>/dev/null
}

# Sets Sirkel's trigger on the test KWin's KGlobalAccel (Qt key code with
# modifiers, e.g. Meta+Space = 0x10000020).
set_trigger() {
    dbus-send --session --print-reply --dest=org.kde.kglobalaccel /kglobalaccel \
        org.kde.KGlobalAccel.setForeignShortcut \
        array:string:sirkel,SirkelSnap,Sirkel,"Sirkel: hold to snap window" "array:int32:$1" >/dev/null 2>&1
}

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
close_windows() {
    # Never a bare `wait`: with no ids it waits for KWin itself.
    if [ ${#WINDOWS[@]} -gt 0 ]; then
        kill "${WINDOWS[@]}" 2>/dev/null; wait "${WINDOWS[@]}" 2>/dev/null
    fi
    WINDOWS=()
}
respawn_windows() {
    close_windows
    spawn window-normal.qml; spawn window-fixed.qml; spawn window-normal.qml
}

# probe_line <window title>: the window's line from probe.js
# ("title|x|y|w|h|tiled"), nothing if there's no such window. Runs probe.js
# in the test KWin.
probe_line() {
    local token="p$(date +%s%N)" id
    sed "s/TOKEN/$token/" "$HERE/probe.js" > "$RUN/probe.js"
    id=$(timeout 10 qdbus6 org.kde.KWin /Scripting org.kde.kwin.Scripting.loadScript "$RUN/probe.js" sirkelprobe 2>/dev/null)
    [ -n "$id" ] || return 0
    qd org.kde.KWin "/Scripting/Script$id" org.kde.kwin.Script.run
    sleep 0.3
    qd org.kde.KWin /Scripting org.kde.kwin.Scripting.unloadScript sirkelprobe
    grep "SIRKEL-PROBE $token $1|" "$LOG" | tail -1
}
# probe_all <window title>: every such window's "x y w h", one per line.
probe_all() {
    local line
    probe_line "$1" >/dev/null
    grep "SIRKEL-PROBE $(grep -o 'SIRKEL-PROBE p[0-9]*' "$LOG" | tail -1 | cut -d' ' -f2) $1|" "$LOG" | awk -F'|' '{print $2, $3, $4, $5}'
}
# top_half_in <"x y w h;..."> : one of them fills the top half of the first
# output.
top_half_in() { printf '%s' "$1" | tr ';' '\n' | awk '$1 <= 2 && $2 <= 2 && $3 >= 1590 && $4 >= 440 && $4 <= 453 { found = 1 } END { exit !found }'; }
# probe <window title>: the window's frame geometry as "x y w h".
probe() { probe_line "$1" | awk -F'|' '{print $2, $3, $4, $5}'; }
# tile_state <window title>: "tiled" if it's in one of KWin's own tiles,
# otherwise "untiled".
tile_state() { probe_line "$1" | awk -F'|' '{print $6}'; }

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

# snap <dx> <dy>: one session from the middle of the first output, moving by
# dx/dy, then time for the window to take its new size.
snap() {
    send abs 800 450; send key $F24 1; send sleep 50
    send move "$1" "$2"; send sleep 100
    send key $F24 0; sync_input
    sleep 0.8
}

# keysnap <key> <presses> [hold ms]: hold the trigger, press a direction key
# that many times (the last one held for `hold` ms, to get key repeats), let
# go of the trigger, then time for the window to take its new size.
keysnap() {
    send abs 800 450; send key $F24 1; send sleep 50
    for i in $(seq "$2"); do
        send key "$1" 1
        if [ "$i" = "$2" ] && [ -n "${3:-}" ]; then send sleep "$3"; else send sleep 30; fi
        send key "$1" 0; send sleep 30
    done
    send key $F24 0; sync_input
    sleep 0.8
}

# drag <title> <dx> [esc]: Meta + left button drag of the window from its
# middle by dx (the usual KWin move), optionally cancelled with Esc.
drag() {
    local geo x y w h
    geo=$(probe "$1")
    [ -n "$geo" ] || return
    read -r x y w h <<<"$geo"
    send abs "$(awk -v x="$x" -v w="$w" 'BEGIN { print int(x + w / 2) }')" "$(awk -v y="$y" -v h="$h" 'BEGIN { print int(y + h / 2) }')"
    send key $META 1; send sleep 50
    send button $BTN_LEFT 1; send sleep 50
    # The move keeps going while the button is held; letting go of Meta
    # makes a later Esc a plain Esc, which is what cancels a move.
    send key $META 0
    for _ in 1 2 3 4 5 6; do send move $(( $2 / 6 )) 0; send sleep 40; done
    if [ "${3:-}" = esc ]; then
        send key $ESC 1; send key $ESC 0; send sleep 50
    fi
    send button $BTN_LEFT 0; sync_input
    sleep 0.8
}

RESULTS=()
FAILS=()
SCENARIO=""
EXPECT=""
count() { grep -c "$1" "$LOG"; }

# check <description> <awk condition on x y w h> <x y w h>: records a failed
# check (also when the window wasn't found).
check() {
    local desc=$1 cond=$2
    shift 2
    if [ $# -lt 4 ] || ! awk -v x="$1" -v y="$2" -v w="$3" -v h="$4" "BEGIN { exit !($cond) }"; then
        FAILS+=("$desc; got: ${*:-no window}")
    fi
}
# check_true <description> <command>: records a failed check if it fails.
check_true() {
    local desc=$1
    shift
    "$@" || FAILS+=("$desc")
}

# begin <name> [log line the scenario must produce, or it didn't test what
# it's meant to]. Every scenario starts with fresh windows: once a window
# closes, this bare KWin (no Plasma shell) doesn't activate another one.
begin() {
    SCENARIO=$1
    EXPECT=${2:-}
    FAILS=()
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
        if [ ${#FAILS[@]} -gt 0 ]; then
            RESULTS+=("FAILED      $SCENARIO")
            local f
            for f in "${FAILS[@]}"; do RESULTS+=("              - $f"); done
            STATUS=1
        elif [ -n "$EXPECT" ] && [ $(( $(count "$EXPECT") - EXPECT_BEFORE )) -eq 0 ]; then
            RESULTS+=("NOT TESTED  $SCENARIO (never saw: $EXPECT)")
            STATUS=1
        else
            [ -n "$EXPECT" ] && note=", hit $(( $(count "$EXPECT") - EXPECT_BEFORE ))x"
            RESULTS+=("ok          $SCENARIO ($started sessions, $refused refused$note)")
        fi
    else
        RESULTS+=("CRASH       $SCENARIO")
        echo "KWin died during: $SCENARIO"
        STATUS=1
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
    [ "$warnings" -gt 0 ] && STATUS=1
    [ "$asan" -gt 0 ] && STATUS=1
    {
        echo
        echo "Results (seed $STRESS_SEED):"
        printf '  %s\n' "${RESULTS[@]}"
        echo "Sessions started: $started, snapped: $snapped, ended without a snap: $cancelled"
        echo "KWin memory (RSS): start ${RSS_START:-?} kB, end ${RSS_END:-$(rss || echo '-')} kB"
        echo "Sirkel/QML warnings in the log: $warnings"
        echo "Sanitizer reports: $asan"
        echo "Log: $LOG"
        [ "$STATUS" = 0 ] && echo "PASSED" || echo "FAILED"
    } | tee "$RUN/summary.txt"
    stop_kwin
    exit "$STATUS"
}

start_kwin --no-lockscreen
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

begin "key mashing (arrows, Enter, Esc, occasional Backspace and R)"
for _ in $(seq "$ROUNDS"); do
    send key $F24 1
    for _ in $(seq "$(rnd 3 12)"); do
        keys=($LEFT $RIGHT $UP $DOWN $LEFT $RIGHT $UP $DOWN $ENTER $ESC)
        [ $(rnd 0 30) = 0 ] && keys+=($BACKSPACE)
        [ $(rnd 0 15) = 0 ] && keys+=($KEY_R)
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

begin "key combination trigger (Meta+Space, releasing either key)" 'trigger is "Meta+Space"'
set_trigger $((0x10000020))
sleep 0.5
for i in $(seq "$ROUNDS"); do
    send abs "$(rnd 300 1300)" "$(rnd 200 700)"
    send key $META 1; send key $SPACE 1; send sleep "$(rnd 0 30)"
    send move "$(rnd -220 220)" "$(rnd -220 220)"; send sleep "$(rnd 0 30)"
    # Alternate which key is let go first; either must end the session.
    if [ $((i % 2)) = 0 ]; then
        send key $META 0; send sleep "$(rnd 0 20)"; send key $SPACE 0
    else
        send key $SPACE 0; send sleep "$(rnd 0 20)"; send key $META 0
    fi
    send sleep "$(rnd 20 60)"
done
sync_input
set_trigger $((0x01000047))
sleep 0.5
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

# ---- Where windows end up (geometry checks; screen 1 is 0,0 1600x900) ------

begin "same direction key again steps the size (edges only)" "same direction again"
send abs 800 450; sync_input
spawn window-drag.qml
# Left: 1 press = 1/2, then 1/3, 1/4, 2/3 and round to 1/2 (1600 px wide).
widths=(0 800 533 400 1067 800)
for presses in 1 2 3 4 5; do
    keysnap $LEFT $presses
    check "Left pressed $presses times: ${widths[$presses]} px wide at the left edge" \
        "x <= 2 && w >= ${widths[$presses]} - 3 && w <= ${widths[$presses]} + 3" $(probe "sirkel-stress drag")
done
keysnap $UP 2
check "Up pressed twice: a third of the height (300 px) at the top" "y <= 2 && h >= 297 && h <= 303 && w >= 1597" $(probe "sirkel-stress drag")
# A corner takes two keys: hold Left, press Up twice. Corners don't step (yet).
send abs 800 450; send key $F24 1; send sleep 50; send key $LEFT 1; send sleep 30
send key $UP 1; send sleep 30; send key $UP 0; send sleep 30; send key $UP 1; send sleep 30; send key $UP 0; send sleep 30
send key $LEFT 0; send key $F24 0; sync_input
sleep 0.8
check "a corner pressed again stays a plain quarter (800 x 450)" "x <= 2 && y <= 2 && w >= 797 && w <= 803 && h >= 447 && h <= 453" $(probe "sirkel-stress drag")
# Holding the key: its automatic repeats must not keep stepping.
steps=$(count "same direction again")
keysnap $LEFT 2 1500
check "Left pressed, then held for 1.5 s: one step (533 px)" "x <= 2 && w >= 530 && w <= 536" $(probe "sirkel-stress drag")
check_true "held key stepped once, not on every repeat" [ $(( $(count "same direction again") - steps )) -eq 1 ]
end

begin "Enter picks maximize, applied when the trigger is let go" "snapping to \"maximize\""
send abs 800 450; sync_input
spawn window-drag.qml
snaps=$(count "sirkel: snapping to")
send abs 800 450; send key $F24 1; send sleep 50; send move -300 0; send sleep 50
send key $ENTER 1; send key $ENTER 0; sync_input
sleep 0.5
check_true "nothing applied while the trigger is still held" [ "$(count "sirkel: snapping to")" -eq "$snaps" ]
send key $F24 0; sync_input
sleep 0.8
check "maximized after letting go of the trigger" "x <= 2 && y <= 2 && w >= 1597 && h >= 897" $(probe "sirkel-stress drag")
# Changing your mind: Enter, then Left, then let go: the window goes left.
send abs 800 450; send key $F24 1; send sleep 50
send key $ENTER 1; send key $ENTER 0; send sleep 50; send key $LEFT 1; send key $LEFT 0
send key $F24 0; sync_input
sleep 0.8
check "Enter then Left picks the left half" "x <= 2 && w >= 797 && w <= 803" $(probe "sirkel-stress drag")
end

begin "R and the Restore shortcut put a snapped window back" "restoring"
send abs 800 450; sync_input
spawn window-drag.qml
geo0=$(probe "sirkel-stress drag")
read -r x0 y0 w0 h0 <<<"$geo0"
back="x >= ${x0:-0} - 2 && x <= ${x0:-0} + 2 && y >= ${y0:-0} - 2 && y <= ${y0:-0} + 2 && w >= ${w0:-0} - 2 && w <= ${w0:-0} + 2 && h >= ${h0:-0} - 2 && h <= ${h0:-0} + 2"
snap -300 0
check "snapped to the left half first" "x <= 2 && w >= 797 && w <= 803" $(probe "sirkel-stress drag")
send abs 800 450; send key $F24 1; send sleep 50
send key $KEY_R 1; send key $KEY_R 0; send sleep 50; send key $F24 0; sync_input
sleep 0.8
check "trigger + R: back where it was before the snap (${geo0:-?})" "$back" $(probe "sirkel-stress drag")
snap 300 0
qd org.kde.kglobalaccel /component/sirkel org.kde.kglobalaccel.Component.invokeShortcut SirkelRestore
sleep 0.8
check "the Restore shortcut: back where it was" "$back" $(probe "sirkel-stress drag")
# Restored windows are forgotten: restoring again leaves it alone.
qd org.kde.kglobalaccel /component/sirkel org.kde.kglobalaccel.Component.invokeShortcut SirkelRestore
sleep 0.5
check "restoring again does nothing" "$back" $(probe "sirkel-stress drag")
end

begin "a click snaps at once, a right-click cancels" "clicked: snapping now"
send abs 800 450; sync_input
spawn window-drag.qml
snaps=$(count "sirkel: snapping to")
sessions=$(count "sirkel: session started")
send abs 800 450; send key $F24 1; send sleep 50; send move -300 0; send sleep 50
send button $BTN_LEFT 1; send sleep 30; send button $BTN_LEFT 0; sync_input
sleep 0.8
check "left click: on the left half while the trigger is still held" "x <= 2 && w >= 797 && w <= 803" $(probe "sirkel-stress drag")
# Still held well past KWin's key repeat delay: its repeats mustn't start
# another session.
send sleep 1500; send key $F24 0; sync_input
sleep 0.3
check_true "letting go of the trigger after the click doesn't snap again" [ $(( $(count "sirkel: snapping to") - snaps )) -eq 1 ]
check_true "holding the trigger after the click didn't start another session" [ $(( $(count "sirkel: session started") - sessions )) -eq 1 ]
before=$(probe "sirkel-stress drag")
send abs 800 450; send key $F24 1; send sleep 50; send move 300 0; send sleep 50
send button $BTN_RIGHT 1; send sleep 30; send button $BTN_RIGHT 0; send sleep 50; send key $F24 0; sync_input
sleep 0.8
after=$(probe "sirkel-stress drag")
check_true "right click: the window didn't move (before: ${before:-?}, after: ${after:-?})" [ -n "$before" -a "$before" = "$after" ]
end

begin "a window in KWin's own tiling leaves its tile when Sirkel moves it"
send abs 800 450; sync_input
spawn window-drag.qml
qd org.kde.kglobalaccel /component/kwin org.kde.kglobalaccel.Component.invokeShortcut "Window Quick Tile Left"
sleep 0.8
check_true "KWin quick-tiled it first (otherwise this tests nothing)" [ "$(tile_state "sirkel-stress drag")" = tiled ]
snap 300 0
check "moved to the right half" "x >= 798 && x <= 802 && w >= 797 && w <= 803" $(probe "sirkel-stress drag")
check_true "no longer in KWin's left tile" [ "$(tile_state "sirkel-stress drag")" = untiled ]
end

begin "windows with a minimum size stay on screen"
send abs 800 450; sync_input
spawn window-minsize.qml
for round in 1 2 3; do
    snap 300 0
    check "right half (round $round): on screen, at least 1000 px wide, against the right edge" \
        "x >= -1 && x + w <= 1601 && w >= 999 && x + w >= 1598" $(probe "sirkel-stress minsize")
    snap -300 0
    check "left half (round $round): on screen, at least 1000 px wide, against the left edge" \
        "x >= -1 && x <= 2 && w >= 999" $(probe "sirkel-stress minsize")
done
end

begin "dragged away gets its size back" "dragged away: restoring"
send abs 800 450; sync_input
spawn window-drag.qml
read -r _ _ w0 h0 <<<"$(probe "sirkel-stress drag")"
for round in 1 2 3; do
    snap -300 0
    check "snapped to the left half (round $round)" "x <= 2 && w >= 790 && w <= 810" $(probe "sirkel-stress drag")
    drag "sirkel-stress drag" 300
    geo=$(probe "sirkel-stress drag")
    check "size from before the snap after dragging (round $round, was ${w0:-?}x${h0:-?})" \
        "w >= ${w0:-0} - 2 && w <= ${w0:-0} + 2 && h >= ${h0:-0} - 2 && h <= ${h0:-0} + 2" $geo
    # The drag starts from the snapped window's middle (400, 450) and moves
    # 300 px: the cursor must still be on the window.
    check "cursor still on the window after the drag (round $round)" "x <= 700 && x + w >= 700 && y <= 450 && y + h >= 450" $geo
done
snap -300 0
drag "sirkel-stress drag" 300 esc
check "a cancelled drag leaves it snapped" "x <= 2 && w >= 790 && w <= 810" $(probe "sirkel-stress drag")
drag "sirkel-stress drag" 300
check "and the next drag still restores the size" "w >= ${w0:-0} - 2 && w <= ${w0:-0} + 2" $(probe "sirkel-stress drag")
end

begin "Enter, Backspace and R reach apps outside a session"
: > "$RUN/keys.log"
env QT_QPA_PLATFORM=wayland QT_LOGGING_RULES= QT_FORCE_STDERR_LOGGING=1 LD_PRELOAD= \
    qml6 "$HERE/window-keys.qml" >>"$RUN/keys.log" 2>&1 &
WINDOWS+=($!)
sleep 1
send key $ENTER 1; send key $ENTER 0; send key $BACKSPACE 1; send key $BACKSPACE 0; send key $KEY_R 1; send key $KEY_R 0; sync_input
sleep 0.5
check_true "Enter reached the app" grep -q "SIRKEL-KEY $QT_RETURN" "$RUN/keys.log"
check_true "Backspace reached the app" grep -q "SIRKEL-KEY $QT_BACKSPACE" "$RUN/keys.log"
check_true "R reached the app" grep -qw "SIRKEL-KEY $QT_R" "$RUN/keys.log"
# While a session holds the keyboard, Enter belongs to Sirkel (it maximizes
# and ends the session) and must not reach the app.
enters=$(grep -c "SIRKEL-KEY $QT_RETURN" "$RUN/keys.log")
send abs 800 450; send key $F24 1; send move 100 0; send sleep 50
send key $ENTER 1; send key $ENTER 0; send key $F24 0; sync_input
sleep 0.5
check_true "Enter during a session stayed with Sirkel" [ "$(grep -c "SIRKEL-KEY $QT_RETURN" "$RUN/keys.log")" -eq "$enters" ]
end

begin "move the window under the mouse" "session started"
send abs 800 450; sync_input
spawn window-drag.qml
# The active window to the right half first, so the cursor can be put over
# another window (the normal ones, centered) without it.
snap 300 0
check "active window snapped to the right half first" "x >= 798 && x <= 802" $(probe "sirkel-stress drag")
# Directions from where the cursor starts, as in the usual setup.
setting TargetWindowUnderCursor true; setting IndicatorFollowsMouse true; reconfigure; sleep 0.3
drag_before=$(probe "sirkel-stress drag")
# Over a normal window, left of the active one: that one moves.
send abs 560 450; send key $F24 1; send sleep 50
send move 0 -300; send sleep 100
send key $F24 0; sync_input
sleep 0.8
check_true "the active window stayed (before: ${drag_before:-?}, after: $(probe "sirkel-stress drag"))" [ -n "$drag_before" -a "$drag_before" = "$(probe "sirkel-stress drag")" ]
normals=$(probe_all "sirkel-stress normal" | tr '\n' ';')
check_true "a normal window under the mouse went to the top half: ${normals:-no window}" top_half_in "$normals"
# Over the active window: it moves.
send abs 1200 600; send key $F24 1; send sleep 50
send move -300 0; send sleep 100
send key $F24 0; sync_input
sleep 0.8
check "the window under the mouse (the active one) went to the left half" "x <= 2 && w >= 797 && w <= 803 && h >= 890" $(probe "sirkel-stress drag")
# Over no window at all: the active window moves.
send abs 1500 700; send key $F24 1; send sleep 50
send move 300 0; send sleep 100
send key $F24 0; sync_input
sleep 0.8
check "nothing under the mouse: the active window went to the right half" "x >= 798 && x <= 802 && h >= 890" $(probe "sirkel-stress drag")
setting TargetWindowUnderCursor false; setting IndicatorFollowsMouse false; reconfigure
end

# ---- Locked screen: a KWin started locked; nothing may start or move -------

RSS_END=$(rss)
stop_kwin
start_kwin --lockscreen
alive || { RESULTS+=("CRASH       starting KWin locked"); STATUS=1; finish; }

begin "locked screen (nothing starts, nothing moves)" "screen is locked"
spawn window-drag.qml
before=$(probe "sirkel-stress drag")
for _ in 1 2 3 4 5; do snap 300 0; done
for n in SirkelSnapLeft SirkelSnapRight SirkelMaximize SirkelMinimize SirkelRestore; do
    qd org.kde.kglobalaccel /component/sirkel org.kde.kglobalaccel.Component.invokeShortcut "$n"
done
sleep 0.5
after=$(probe "sirkel-stress drag")
check_true "no session started" [ $(( $(count "sirkel: session started") - STARTED_BEFORE )) -eq 0 ]
check_true "the window didn't move (before: ${before:-?}, after: ${after:-?})" [ -n "$before" -a "$before" = "$after" ]
end

finish
