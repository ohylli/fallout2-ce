#ifndef FALLOUT_TOLK_H_
#define FALLOUT_TOLK_H_

namespace fallout {

// Initialize Tolk (loads DLL dynamically). Returns 0 on success, -1 on failure.
// If Tolk.dll is not found, returns 0 but tolkIsActive() will return false.
int tolkInit();

// Shutdown Tolk and unload DLL
void tolkExit();

// Speak text to screen reader
// If interrupt is true, stops current speech before speaking
void tolkSpeak(const char* text, bool interrupt = false);

// Check if Tolk is loaded and a screen reader is active
bool tolkIsActive();

} // namespace fallout

#endif /* FALLOUT_TOLK_H_ */
