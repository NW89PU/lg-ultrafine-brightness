#pragma once

// Windows autostart via HKCU\Software\Microsoft\Windows\CurrentVersion\Run.
// The current .exe path (GetModuleFileNameW) is written; renaming or moving
// the executable invalidates the entry — toggle off/on after such a move.
namespace autostart {

bool isEnabled();
void setEnabled(bool enabled);

} // namespace autostart
