#ifndef __LOADER_DEBUGGER_H__
#define __LOADER_DEBUGGER_H__

bool loader_is_debugger_present();
int loader_dprintf(const char* format, ...);
int loader_install_team_debugger(int debuggerTeam, int debuggerPort);
int loader_remove_team_debugger();

#endif // __LOADER_DEBUGGER_H__