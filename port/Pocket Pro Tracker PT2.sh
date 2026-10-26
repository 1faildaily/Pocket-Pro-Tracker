#!/bin/bash
# Pocket Pro Tracker PT2 Clone v0.3.15, experimental PortMaster launcher
# No external evdev mapper: PPT input module uses saved RG35XX Pro raw keycodes.
set -u
XDG_DATA_HOME="${XDG_DATA_HOME:-$HOME/.local/share}"
for dir in /opt/system/Tools/PortMaster /opt/tools/PortMaster "$XDG_DATA_HOME/PortMaster" /mmc/MUOS/PortMaster; do
  if [ -f "$dir/control.txt" ]; then
    source "$dir/control.txt"
    [ -n "${CFW_NAME:-}" ] && [ -f "$dir/mod_${CFW_NAME}.txt" ] && source "$dir/mod_${CFW_NAME}.txt"
    if type get_controls >/dev/null 2>&1; then get_controls; fi
    break
  fi
done
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
GAMEDIR=""
if [ -n "${directory:-}" ]; then
  ROOT="/${directory#/}"
  [ -f "$ROOT/ports/pocket_pro_tracker_pt2/pt2-clone" ] && GAMEDIR="$ROOT/ports/pocket_pro_tracker_pt2"
fi
if [ -z "$GAMEDIR" ]; then
  for path in "$SCRIPT_DIR/pocket_pro_tracker_pt2" "$SCRIPT_DIR/../../ports/pocket_pro_tracker_pt2" /mnt/mmc/ports/pocket_pro_tracker_pt2 /mnt/sdcard/ports/pocket_pro_tracker_pt2 /mmc/ports/pocket_pro_tracker_pt2 /sdcard/ports/pocket_pro_tracker_pt2; do
    if [ -f "$path/pt2-clone" ]; then GAMEDIR="$path"; break; fi
  done
fi
if [ -z "$GAMEDIR" ]; then echo 'PPT PT2 ERROR: pt2-clone executable not found' >&2; exit 1; fi
cd "$GAMEDIR" || exit 1
mkdir -p mods samples saves
export HOME="$GAMEDIR/home"
mkdir -p "$HOME/.config/protracker"
# Upstream changes directory to $HOME after initializing, so keep MODs accessible there too.
ln -sfn "$GAMEDIR/mods" "$HOME/mods"
ln -sfn "$GAMEDIR/samples" "$HOME/samples"
# When config is next to executable, PT2 clone uses that one by priority.
export SDL_AUDIODRIVER="${SDL_AUDIODRIVER:-alsa}"
export SDL_RENDER_DRIVER="${SDL_RENDER_DRIVER:-software}"
export SDL_VIDEO_MINIMIZE_ON_FOCUS_LOSS=0
export PPT_DIAG_LOG="$GAMEDIR/ppt_input.log"
export PPT_VIDEO_DIAG_LOG="$GAMEDIR/ppt_video.log"
chmod +x ./pt2-clone 2>/dev/null || true
{
 echo "===== PPT PT2 v0.3.15 $(date) ====="
 echo "System: $(uname -m)"
 echo "SDL_VIDEODRIVER=${SDL_VIDEODRIVER:-autodetect} SDL_AUDIODRIVER=$SDL_AUDIODRIVER SDL_RENDER_DRIVER=$SDL_RENDER_DRIVER"
 echo "Binary: $(ls -l ./pt2-clone 2>&1)"
 echo "Input: $(ls /dev/input/event* 2>/dev/null | tr '\n' ' ')"
 if command -v ldd >/dev/null 2>&1; then ldd ./pt2-clone || true; fi
} >> launch.log 2>&1
./pt2-clone >> launch.log 2>&1
EXIT=$?
echo "PT2 exit code: $EXIT" >> launch.log
if type pm_finish >/dev/null 2>&1; then pm_finish; fi
exit "$EXIT"
