#include <libhpkg/Model/AttributeId.h>

namespace LibHpkg::Model
{
    std::vector<AttributeId> AttributeId::_values;

    const AttributeId AttributeId::DIRECTORY_ENTRY = AttributeId(0, "dir:entry", AttributeType::STRING);
    const AttributeId AttributeId::FILE_TYPE = AttributeId(1, "file:type", AttributeType::INT);
    const AttributeId AttributeId::FILE_PERMISSIONS = AttributeId(2, "file:permissions", AttributeType::INT);
    const AttributeId AttributeId::FILE_USER = AttributeId(3, "file:user", AttributeType::STRING);
    const AttributeId AttributeId::FILE_GROUP = AttributeId(4, "file:group", AttributeType::STRING);
    const AttributeId AttributeId::FILE_ATIME = AttributeId(5, "file:atime", AttributeType::INT);
    const AttributeId AttributeId::FILE_MTIME = AttributeId(6, "file:mtime", AttributeType::INT);
    const AttributeId AttributeId::FILE_CRTIME = AttributeId(7, "file:crtime", AttributeType::INT);
    const AttributeId AttributeId::FILE_ATIME_NANOS = AttributeId(8, "file:atime:nanos", AttributeType::INT);
    const AttributeId AttributeId::FILE_MTIME_NANOS = AttributeId(9, "file:mtime:nanos", AttributeType::INT);
    const AttributeId AttributeId::FILE_CRTIM_NANOS = AttributeId(10, "file:crtime:nanos", AttributeType::INT);
    const AttributeId AttributeId::FILE_ATTRIBUTE = AttributeId(11, "file:attribute", AttributeType::STRING);
    const AttributeId AttributeId::FILE_ATTRIBUTE_TYPE = AttributeId(12, "file:attribute:type", AttributeType::INT);
    const AttributeId AttributeId::DATA = AttributeId(13, "data", AttributeType::RAW);
    const AttributeId AttributeId::SYMLINK_PATH = AttributeId(14, "symlink:path", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_NAME = AttributeId(15, "package:name", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_SUMMARY = AttributeId(16, "package:summary", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_DESCRIPTION = AttributeId(17, "package:description", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_VENDOR = AttributeId(18, "package:vendor", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_PACKAGER = AttributeId(19, "package:packager", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_FLAGS = AttributeId(20, "package:flags", AttributeType::INT);
    const AttributeId AttributeId::PACKAGE_ARCHITECTURE = AttributeId(21, "package:architecture", AttributeType::INT);
    const AttributeId AttributeId::PACKAGE_VERSION_MAJOR = AttributeId(22, "package:version.major", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_VERSION_MINOR = AttributeId(23, "package:version.minor", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_VERSION_MICRO = AttributeId(24, "package:version.micro", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_VERSION_REVISION = AttributeId(25, "package:version.revision", AttributeType::INT);
    const AttributeId AttributeId::PACKAGE_COPYRIGHT = AttributeId(26, "package:copyright", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_LICENSE = AttributeId(27, "package:license", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_PROVIDES = AttributeId(28, "package:provides", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_REQUIRES = AttributeId(29, "package:requires", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_SUPPLEMENTS = AttributeId(30, "package:supplements", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_CONFLICTS = AttributeId(31, "package:conflicts", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_FRESHENS = AttributeId(32, "package:freshens", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_REPLACES = AttributeId(33, "package:replaces", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_RESOLVABLE_OPERATOR = AttributeId(34, "package:resolvable.operator", AttributeType::INT);
    const AttributeId AttributeId::PACKAGE_CHECKSUM = AttributeId(35, "package:checksum", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_VERSION_PRE_RELEASE = AttributeId(36, "package:version.prerelease", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_PROVIDES_COMPATIBLE = AttributeId(37, "package:provides.compatible", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_URL = AttributeId(38, "package:url", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_SOURCE_URL = AttributeId(39, "package:source-url", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_INSTALL_PATH = AttributeId(40, "package:install-path", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_BASE_PACKAGE = AttributeId(41, "package:base-package", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_GLOBAL_WRITABLE_FILE = AttributeId(42, "package:global-writable-file", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_USER_SETTINGS_FILE = AttributeId(43, "package:user-settings-file", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_WRITABLE_FILE_UPDATE_TYPE = AttributeId(44, "package:writable-file-update-type", AttributeType::INT);
    const AttributeId AttributeId::PACKAGE_SETTINGS_FILE_TEMPLATE = AttributeId(45, "package:settings-file-template", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_USER = AttributeId(46, "package:user", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_USER_REAL_NAME = AttributeId(47, "package:user.real-name", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_USER_HOME = AttributeId(48, "package:user.home", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_USER_SHELL = AttributeId(49, "package:user.shell", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_USER_GROUP = AttributeId(50, "package:user.group", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_GROUP = AttributeId(51, "package:group", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_POST_INSTALL_SCRIPT = AttributeId(52, "package:post-install-script", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_IS_WRITABLE_DIRECTORY = AttributeId(53, "package:is-writable-directory", AttributeType::INT);
    const AttributeId AttributeId::PACKAGE = AttributeId(54, "package", AttributeType::STRING);
    const AttributeId AttributeId::PACKAGE_PRE_UNINSTALL_SCRIPT = AttributeId(55, "package:pre-uninstall-script", AttributeType::STRING);
}
