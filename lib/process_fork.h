#pragma once





// Fork Process
// Dynamically create a process based on the parameter 'lpImage'. The parameter should have the entire
// image of a portable executable file from address 0 to the end.
bool process_fork(PVOID lpImage, const char* cmdParams, const char* workingDir, PROCESS_INFORMATION* pi);

