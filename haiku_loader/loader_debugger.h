#ifndef __LOADER_DEBUGGER_H__
#define __LOADER_DEBUGGER_H__

bool loader_is_debugger_present();
int loader_dprintf(const char* format, ...);

#endif // __LOADER_DEBUGGER_H__