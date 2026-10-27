#!/bin/bash
# Pocket Pro Tracker PT2 Clone v0.3.45, experimental PortMaster launcher
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
# PPT_SD2_BEGIN: only DISK-OP defaults; game/config/assets stay in GAMEDIR.
ppt_sd2_root() {
  local candidate
  for candidate in "$@"; do
    [ -d "$candidate" ] && [ -r "$candidate" ] && [ -x "$candidate" ] || continue
    if command -v mountpoint >/dev/null 2>&1; then
      mountpoint -q "$candidate" || continue
    else
      awk -v path="$candidate" '$2==path {found=1} END {exit !found}' /proc/mounts || continue
    fi
    printf '%s\n' "$candidate"
    return 0
  done
  return 1
}
ppt_disk_defaults() {
  local config_path="$1" module_path="$2" sample_path="$3"
  [ -f "$config_path" ] || return 1
  local temp_path
  temp_path="$(mktemp "${config_path}.ppt.XXXXXX")" || return 1
  if awk -v moddir="$module_path" -v smpdir="$sample_path" '
    toupper($0) ~ /^[[:space:]]*DEFAULTDIR=/ {next}
    toupper($0) ~ /^[[:space:]]*DEFAULTSMPDIR=/ {next}
    {print}
    END {print "DEFAULTDIR=" moddir; print "DEFAULTSMPDIR=" smpdir}
  ' "$config_path" > "$temp_path" && mv -f "$temp_path" "$config_path"; then
    return 0
  fi
  rm -f "$temp_path"
  return 1
}
# A directory alone is insufficient: require a mounted card.
PPT_SD2_ROOT="$(ppt_sd2_root /mnt/sdcard /sdcard)" || PPT_SD2_ROOT=""
if [ -n "$PPT_SD2_ROOT" ]; then
  PPT_MOD_ROOT="$PPT_SD2_ROOT"
  PPT_SMP_ROOT="$PPT_SD2_ROOT"
else
  PPT_MOD_ROOT="$GAMEDIR/mods"
  PPT_SMP_ROOT="$GAMEDIR/samples"
fi
ppt_disk_defaults "$GAMEDIR/protracker.ini" "$PPT_MOD_ROOT" "$PPT_SMP_ROOT" ||
  echo 'PPT: could not update DISK-OP defaults' >> "$GAMEDIR/launch.log"
printf 'DISK-OP MOD=%s SAMPLE=%s\n' "$PPT_MOD_ROOT" "$PPT_SMP_ROOT" >> "$GAMEDIR/launch.log"
# PPT_SD2_END
# When config is next to executable, PT2 clone uses that one by priority.
export SDL_AUDIODRIVER="${SDL_AUDIODRIVER:-alsa}"
export SDL_RENDER_DRIVER="${SDL_RENDER_DRIVER:-software}"
export SDL_VIDEO_MINIMIZE_ON_FOCUS_LOSS=0
export PPT_DIAG_LOG="$GAMEDIR/ppt_input.log"
export PPT_VIDEO_DIAG_LOG="$GAMEDIR/ppt_video.log"
chmod +x ./pt2-clone 2>/dev/null || true
{
 echo "===== PPT PT2 v0.3.45 $(date) ====="
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
