#include <cstdlib>
#include <filesystem>
#include <iostream>
#include "server_prefix.h"

std::string gHaikuPrefix;

bool server_init_prefix()
{
    const char* hPrefixPtr = getenv("HPREFIX");
    if (hPrefixPtr == NULL)
    {
        hPrefixPtr = getenv("HOME");
		if (!hPrefixPtr)
		{
			hPrefixPtr = getenv("USERPROFILE");
		}
		if (!hPrefixPtr)
		{
			std::cerr << "Failed to determine the HyClone prefix." << std::endl;
			std::cerr << "Ensure that the HPREFIX environment variable is set." << std::endl;
			return false;
		}
        gHaikuPrefix = std::filesystem::canonical(std::filesystem::path(hPrefixPtr) / ".hprefix");
    }
    else
    {
        gHaikuPrefix = std::filesystem::canonical(std::filesystem::path(hPrefixPtr));
    }
    return true;
}