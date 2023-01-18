#ifndef __FCNTL_CONVERSION_H__
#define __FCNTL_CONVERSION_H__

int OFlagsBToLinux(int flags);
int OFlagsLinuxToB(int flags);

int SeekTypeBToLinux(int seekType);

int FcntlBToLinux(int fcntl);

#endif // __FCNTL_CONVERSION_H__