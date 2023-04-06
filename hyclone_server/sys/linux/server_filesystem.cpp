#include <filesystem>
#include <fstream>

#include "server_prefix.h"

bool server_setup_settings()
{
    std::filesystem::path systemPath = std::filesystem::path(gHaikuPrefix) / "boot" / "system";
    std::filesystem::path settingsPath = systemPath / "settings";
    std::filesystem::path networkSettingsPath = settingsPath / "network";

    std::filesystem::create_directories(networkSettingsPath);

    std::filesystem::copy_file("/etc/hostname", networkSettingsPath / "hostname",
        std::filesystem::copy_options::overwrite_existing);
    std::filesystem::copy_file("/etc/hosts", networkSettingsPath / "hosts",
        std::filesystem::copy_options::overwrite_existing);
    std::filesystem::copy_file("/etc/resolv.conf", networkSettingsPath / "resolv.conf",
        std::filesystem::copy_options::overwrite_existing);

    std::filesystem::path etcPath = systemPath / "etc";
    auto oldSystemPerms = std::filesystem::status(systemPath).permissions();
    std::filesystem::permissions(systemPath, oldSystemPerms | std::filesystem::perms::owner_write);
    std::filesystem::create_directories(etcPath);
    std::filesystem::permissions(systemPath, oldSystemPerms);

    std::filesystem::copy_file("/etc/passwd", etcPath / "passwd",
        std::filesystem::copy_options::overwrite_existing);
    std::filesystem::copy_file("/etc/group", etcPath / "group",
        std::filesystem::copy_options::overwrite_existing);
    std::fstream shadow((etcPath / "shadow").c_str(), std::ios::out | std::ios::app);

    return true;
}
