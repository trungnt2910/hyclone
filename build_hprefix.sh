#!/bin/bash
set -e


HAIKU_ADDITIONAL_PACKAGES=""
HAIKU_ADDITIONAL_SYSPACKAGES=""

# Loop through all arguments and process them
for arg in "$@"
do
    case $arg in
        -a=*|--arch=*)
        HAIKU_ARCH="${arg#*=}"
        shift # Remove --arch from processing
        ;;
        -s=*|--source=*)
        HAIKU_BUILD_SOURCE_DIRECTORY="${arg#*=}"
        shift # Remove --source from processing
        ;;
        -p=*|--prefix=*)
        HPREFIX="${arg#*=}"
        shift # Remove --prefix from processing
        ;;
        -P=*|--system-prefix=*)
        HYCLONE_INSTALL_PREFIX="${arg#*=}"
        shift # Remove --system-prefix from processing
        ;;
        -f|--force)
        HPKG_FORCE=1
        shift # Remove --force from processing
        ;;
        -A=*|--additional-packages=*)
        HAIKU_ADDITIONAL_PACKAGES="${arg#*=}"
        shift # Remove --additional-packages from processing
        ;;
        -S=*|--additional-syspackages=*)
        HAIKU_ADDITIONAL_SYSPACKAGES="${arg#*=}"
        shift # Remove --additional-syspackages from processing
        ;;
        -h|--help)
        echo "Usage: build_hprefix.sh [options]"
        echo "Options:"
        echo "  -a, --arch=ARCH                         Architecture to build for (default: x86_64)"
        echo "  -s, --source=DIR                        Haiku source directory (default: \$HAIKU_BUILD_SOURCE_DIRECTORY,"
        echo "                                          or \$SCRIPT_DIR/../haiku if this environment is not set)"
        echo "  -p, --prefix=DIR                        Haiku prefix directory (default: \$HPREFIX,"
        echo "                                          or ~/.hprefix if this environment is not set)"
        echo "  -P, --system-prefix=DIR                 The prefix to which HyClone is installed (default: "
        echo "                                          \$HYCLONE_INSTALL_PREFIX, or "
        echo "                                          \$(realpath \$(dirname \$(which hyclone_server))/..) if this"
        echo "                                          environment is not set)"
        echo "  -A, --additional-packages=PACKAGES      Comma-separated list of additional packages to install"
        echo "  -S, --additional-syspackages=PACKAGES   Comma-separated list of additional syspackages to install"
        echo "  -f, --force                             Overwrite existing packages"
        echo "  -h, --help                              Show this help"
        exit 0
        ;;
        *)
        echo "Unknown option: $arg"
        echo "Run build_hprefix.sh --help for usage"
        exit 1
        ;;
    esac
done

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

HAIKU_UNIX_ARCH=${HAIKU_UNIX_ARCH:-"$(uname -m)"}

case $HAIKU_UNIX_ARCH in
    "aarch64")
        HAIKU_ARCH="arm64"
        ;;
    *)
        HAIKU_ARCH=$HAIKU_UNIX_ARCH
        ;;
esac

HAIKU_BUILD_ENVIRONMENT_ROOT=${HAIKU_BUILD_ENVIRONMENT_ROOT:-"$SCRIPT_DIR/.."}
HAIKU_BUILD_SOURCE_DIRECTORY=${HAIKU_BUILD_SOURCE_DIRECTORY:-"$HAIKU_BUILD_ENVIRONMENT_ROOT/haiku"}
HPREFIX=${HPREFIX:-"$HOME/.hprefix"}
HPREFIX_PACKAGES=$"$HPREFIX/boot/system/packages"
HAIKU_DEPOT_BASE_URL="https://depot.haiku-os.org/__api/v2/pkg/get-pkg"
HAIKU_HPKG_BASE_URL="https://eu.hpkg.haiku-os.org/haiku/master/$HAIKU_ARCH/current"
HAIKU_SYSPACKAGES="haiku"
HAIKU_PACKAGES="bash bzip2 ca_root_certificates coreutils expat fontconfig freetype gawk gcc_syslibs gettext_libintl graphite2 gzip harfbuzz icu74 intel_wifi_firmwares libedit libiconv libpng16 libsolv libxml2 ncurses6 noto noto_sans_cjk_jp openssl ralink_wifi_firmwares readline realtek_wifi_firmwares tar unzip zlib zstd"
HPKG_FORCE=${HPKG_FORCE:-"0"}

HAIKU_ADDITIONAL_SYSPACKAGES=${HAIKU_ADDITIONAL_SYSPACKAGES//,/ }
HAIKU_ADDITIONAL_PACKAGES=${HAIKU_ADDITIONAL_PACKAGES//,/ }

HAIKU_SYSPACKAGES+=" $HAIKU_ADDITIONAL_SYSPACKAGES"
HAIKU_PACKAGES+=" $HAIKU_ADDITIONAL_PACKAGES"

CURL_RETRY_COUNT=${CURL_RETRY_COUNT:-"16"}

mkdir -p $HPREFIX_PACKAGES

if pidof -q hyclone_server; then
    echo "hyclone_server is running."
    echo "Please terminate your HyClone instance (killall -9 hyclone_server haiku_loader) and try again."
    exit 1
fi

if [ -z "$HYCLONE_INSTALL_PREFIX" ]; then
    if [ -z "$(which hyclone_server)" ]; then
        echo "hyclone_server not found in \$PATH."
        echo "Make sure you have installed HyClone (sudo make install) and add it to your \$PATH."
        echo "Alternatively, you can specify the system prefix with the --system-prefix option or the \$HYCLONE_INSTALL_PREFIX environment variable."
        exit 1
    fi
    HYCLONE_INSTALL_PREFIX=${HYCLONE_INSTALL_PREFIX:-"$(realpath $(dirname $(which hyclone_server))/..)"}
fi

if [ ! -d "$HYCLONE_INSTALL_PREFIX" ]; then
    echo "System prefix $HYCLONE_INSTALL_PREFIX does not exist."
    echo "Please check your arguments and the \$HYCLONE_INSTALL_PREFIX environment variable and try again."
    exit 1
fi

if [ ! -f "$HYCLONE_INSTALL_PREFIX/bin/hyclone_server" ]; then
    echo "hyclone_server not found at $HYCLONE_INSTALL_PREFIX/bin/hyclone_server."
    echo "Have you installed HyClone (sudo make install)?"
    exit 1
fi

echo "Downloading Haiku packages"
read -ra array <<<"$HAIKU_SYSPACKAGES"
HAIKU_SYSPACKAGES_HREV=$(curl --retry $CURL_RETRY_COUNT --retry-all-errors -Ls $HAIKU_HPKG_BASE_URL | sed -n 's/^.*version: "\([^"]*\)".*$/\1/p')
for package in "${array[@]}"; do
    # Check if package already exists
    if [ "$HPKG_FORCE" == "0" ] && [ -f "$HPREFIX_PACKAGES/$package-$HAIKU_SYSPACKAGES_HREV-1-$HAIKU_ARCH.hpkg" ]; then
        echo "$package already exists"
    else
        if ls $HPREFIX_PACKAGES/$package-*-$HAIKU_ARCH.hpkg 1> /dev/null 2>&1; then
            echo "Removing old $package packages"
            rm -fv $HPREFIX_PACKAGES/$package-*-$HAIKU_ARCH.hpkg
        fi
        echo "Downloading $package-$HAIKU_SYSPACKAGES_HREV-1-$HAIKU_ARCH.hpkg"
        curl --retry $CURL_RETRY_COUNT --retry-all-errors \
            -Lo "$HPREFIX_PACKAGES/$package-$HAIKU_SYSPACKAGES_HREV-1-$HAIKU_ARCH.hpkg" \
            "$HAIKU_HPKG_BASE_URL/packages/$package-$HAIKU_SYSPACKAGES_HREV-1-$HAIKU_ARCH.hpkg"
    fi
done
read -ra array <<<"$HAIKU_PACKAGES"
for package in "${array[@]}"; do
    # API documented here: https://github.com/haiku/haikudepotserver/blob/master/haikudepotserver-api2/src/main/resources/api2/pkg.yaml#L60
    # The schema here: https://github.com/haiku/haikudepotserver/blob/master/haikudepotserver-api2/src/main/resources/api2/pkg.yaml#L598
    hpkgDownloadUrl="$(curl --retry $CURL_RETRY_COUNT --retry-all-errors -Ls --request POST \
        --data '{"name":"'"$package"'","repositorySourceCode":"haikuports_'$HAIKU_ARCH'","versionType":"LATEST","naturalLanguageCode":"en"}' \
        --header 'Content-Type:application/json' "$HAIKU_DEPOT_BASE_URL" | sed -n 's/^.*hpkgDownloadURL":"\([^"]*\)".*$/\1/p')"
    hpkgVersion="$(echo "$hpkgDownloadUrl" | sed -n 's/^.*\/[^\/]*-\([^-]*\-[^-]*\)-[^-]*\.hpkg$/\1/p')"
    if [ "$HPKG_FORCE" == "0" ] && [ -f "$HPREFIX_PACKAGES/$package-$hpkgVersion-$HAIKU_ARCH.hpkg" ]; then
        echo "$package already exists"
    else
        if ls $HPREFIX_PACKAGES/$package-*-$HAIKU_ARCH.hpkg 1> /dev/null 2>&1; then
            echo "Removing old $package packages"
            rm -fv $HPREFIX_PACKAGES/$package-*-$HAIKU_ARCH.hpkg
        fi
        echo "Downloading $package-$hpkgVersion-$HAIKU_ARCH.hpkg"
        curl --retry $CURL_RETRY_COUNT --retry-all-errors \
            -Lo "$HPREFIX_PACKAGES/$package-$hpkgVersion-$HAIKU_ARCH.hpkg" "$hpkgDownloadUrl"
    fi
done

echo "Setup HyClone prefix"
$HYCLONE_INSTALL_PREFIX/bin/hyclone_server
killall -9 hyclone_server haiku_loader

echo "Copy additional required files"
# Required for bash prompt to work like Haiku.
cp -rfv $HAIKU_BUILD_SOURCE_DIRECTORY/data/etc $HPREFIX/boot/system
# Disables some faulty services that are enabled by default.
mkdir -p $HPREFIX/boot/system/non-packaged/data/launch
cp -rfv $SCRIPT_DIR/data/launch $HPREFIX/boot/system/non-packaged/data
# Some apps require this link to work properly.
if [ ! -L "$HPREFIX/system" ]; then
    ln -s boot/system $HPREFIX/system
fi

echo "Done"
