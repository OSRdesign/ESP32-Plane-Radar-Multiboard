#pragma once

namespace menu {

// Blocking modal menu (orientation, re-run setup, back). Call right after the
// 5 s hold was released; returns when the user taps "Back". "Re-run setup"
// clears the settings and restarts, so it does not return.
void run();

}  // namespace menu
