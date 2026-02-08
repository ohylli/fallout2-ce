#ifndef FALLOUT_OBJECT_SCANNER_H_
#define FALLOUT_OBJECT_SCANNER_H_

namespace fallout {

// Initialize object scanner module
void objectScannerInit();

// Handle scanner navigation keys.
// Returns true if key was handled, false otherwise.
bool objectScannerHandleKey(int key);

} // namespace fallout

#endif /* FALLOUT_OBJECT_SCANNER_H_ */
