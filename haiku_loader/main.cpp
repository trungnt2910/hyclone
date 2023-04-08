#include "BeDefs.h"
#include "extended_commpage.h"
#include "haiku_fcntl.h"
#include "haiku_loader.h"
#include "loader_commpage.h"
#include "loader_exec.h"
#include "loader_requests.h"
#include "loader_runtime.h"
#include "loader_registration.h"
#include "loader_servercalls.h"
#include "loader_spawn_thread.h"
#include "loader_tls.h"
#include "loader_vchroot.h"
#include "runtime_loader.h"
#include "thread_defs.h"

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

rld_export **__gRuntimeLoaderPtr;
const char* hPrefix;

size_t loader_build_path(char* buffer, size_t bufferSize)
{
	std::stringstream result;
	result << "PATH=";
	// Probably something that came from an execve.
	const char* hpath_expanded = getenv("_HPATH_EXPANDED");
	if (hpath_expanded != NULL && strcmp(hpath_expanded, "1") == 0)
	{
		result << getenv("PATH");
	}
	else
	{
		const char* hpath = getenv("HPATH");
		if (hpath != NULL)
			result << hpath << ":";
		const char* path = getenv("PATH");
		if (path != NULL)
		{
			do
			{
				const char* end = strchr(path, ':');
				if (end == NULL)
				{
					end = strchr(path, '\0');
				}
				if (end - path > 0)
				{
					char* tmp = new char[end - path + 1];
					strncpy(tmp, path, end - path);
					tmp[end - path] = 0;
					size_t unexpandLength = loader_vchroot_unexpand(tmp, NULL, 0);
					char* unexpanded = new char[unexpandLength + 1];
					unexpanded[unexpandLength] = 0;
					loader_vchroot_unexpand(tmp, unexpanded, unexpandLength + 1);
					result << unexpanded << ":";
					delete[] tmp;
					delete[] unexpanded;
				}
				path = end;
			}
			while (*(path++) != '\0');
		}
	}

	std::string resultString = result.str();
	if (resultString.size() && resultString.back() == ':')
	{
		resultString.pop_back();
	}
	if (buffer != NULL)
	{
		strncpy(buffer, resultString.c_str(), bufferSize);
	}
	return resultString.size();
}

size_t loader_build_home(char* buffer, size_t bufferSize)
{
// Haiku is currently a single user OS,
// so the only "Home" is in /boot/home (no username)
#define HOME_VALUE "HOME=/boot/home"
	if (buffer != NULL)
	{
		strncpy(buffer, HOME_VALUE, bufferSize);
	}
	return sizeof(HOME_VALUE) - 1;
}
#undef HOME_VALUE

void loader_build_args(uint8*& mem, user_space_program_args &args, char **argv, char **env)
{
	size_t argCnt = 0;
	while (argv[argCnt] != NULL) argCnt++;
	size_t envCnt = 0;
	while (env[envCnt] != NULL) envCnt++;
	size_t argSize = 0;
	for (size_t i = 0; i < argCnt; ++i)
    {
		argSize += strlen(argv[i]) + 1;
	}
	size_t pathArgSize = loader_build_path(NULL, 0) + 1;
	size_t homeArgSize = loader_build_home(NULL, 0) + 1;
	for (size_t i = 0; i < envCnt; ++i)
    {
		if (strncmp(env[i], "PATH=", sizeof("PATH=") - 1) == 0)
		{
			argSize += pathArgSize;
		}
		else if (strncmp(env[i], "HOME=", sizeof("HOME=") - 1) == 0)
		{
			argSize += homeArgSize;
		}
		else
		{
			argSize += strlen(env[i]) + 1;
		}
	}

	++envCnt; // One for _HPATH_EXPANDED
	argSize += sizeof("_HPATH_EXPANDED=1") - 1; // Argument of _HPATH_EXPANDED

	mem = new uint8[sizeof(void*)*(argCnt + envCnt + 2) + argSize];
	char **outArgs = (char**)&mem[0];
	char *outChars = (char*)&mem[sizeof(void*)*(argCnt + envCnt + 2)];
	for (size_t i = 0; i < argCnt; ++i)
    {
		size_t len = strlen(argv[i]) + 1;
		*outArgs = outChars; outArgs++;
		memcpy(outChars, argv[i], len);
		outChars += len;
	}
	*outArgs = NULL; outArgs++;
	for (size_t i = 0; i < envCnt && env[i]; ++i)
    {
		if (strncmp(env[i], "PATH=", sizeof("PATH=") - 1) == 0)
		{
			*outArgs = outChars; outArgs++;
			outChars += loader_build_path(outChars, pathArgSize);
			*outChars = '\0';
			++outChars;
		}
		else if (strncmp(env[i], "HOME=", sizeof("HOME=") - 1) == 0)
		{
			*outArgs = outChars; outArgs++;
			outChars += loader_build_home(outChars, homeArgSize);
			*outChars = '\0';
			++outChars;
		}
		else
		{
			size_t len = strlen(env[i]) + 1;
			*outArgs = outChars; outArgs++;
			memcpy(outChars, env[i], len);
			outChars += len;
		}
	}
	*outArgs = outChars; outArgs++;
	memcpy(outChars, "_HPATH_EXPANDED=1", sizeof("_HPATH_EXPANDED=1") - 1);

	*outArgs = NULL; outArgs++;

	char buf[B_PATH_NAME_LENGTH];
	strcpy(buf, argv[0]);
	strcpy(args.program_name, basename(buf));
	strcpy(args.program_path, argv[0]);

	args.error_port = -1;
	args.arg_count = argCnt;
	args.env_count = envCnt;
	args.args = (char**)&mem[0];
	args.env = (char**)&mem[0] + (argCnt + 1);
}

void loader_at_exit(int retVal)
{
	// std::cout << "Program exited with return code: " << retVal << std::endl;
	// Ensure that the loader's variables are properly cleaned,
	// instead of directly using the syscall.
	std::exit(retVal);
}

int main(int argc, char** argv, char** envp)
{
	if (argc <= 1)
	{
		std::cout << "Usage: " << argv[0]
			<< " [--umask <umask>] [--error-port <error_port>] [--error-token <error_token>] [--] "
			<< "<path to haiku executable> [args]" << std::endl;
		return 1;
	}

	++argv; --argc;

	hPrefix = getenv("HPREFIX");
	if (!loader_init_vchroot(hPrefix))
	{
		std::cout << "Failed to initialize vchroot" << std::endl;
		return 1;
	}

    void* commpage = loader_allocate_commpage();

	// std::cout << "Connecting to HyClone server" << std::endl;
	if (!loader_init_servercalls())
	{
		std::cout << "Failed to connect to HyClone server" << std::endl;
		loader_free_commpage(commpage);
		return 1;
	}

    // std::cout << "Loading runtime_loader" << std::endl;
	if (!loader_load_runtime())
	{
		std::cout << "Failed to load Haiku runtime_loader" << std::endl;
		loader_free_commpage(commpage);
		return 1;
	}

	if (!loader_init_requests())
	{
		std::cout << "Failed to initialize requests" << std::endl;
		loader_free_commpage(commpage);
		return 1;
	}

	auto runtime_loader = gRuntimeLoaderInfo.entry_point;
	__gRuntimeLoaderPtr = gRuntimeLoaderInfo.gRuntimeLoaderPtr;

	// Hook test_executable as we want to also be able to execve
	// binaries of the host system.
	loader_haiku_test_executable = (*__gRuntimeLoaderPtr)->test_executable;
	(*__gRuntimeLoaderPtr)->test_executable = loader_test_executable;

    uint8* user_args_memory = NULL;
	user_space_program_args args;

	args.umask = -1;
	args.error_port = -1;
	args.error_token = 0;

	while (strncmp(argv[0], "--", 2) == 0)
	{
		const char* currentArg = argv[0];
		++argv; --argc;
		try
		{
			if (strcmp(currentArg, "--umask") == 0)
			{
				if (argc <= 0)
				{
					std::cout << "Missing value for --umask flag.";
					return 1;
				}
				args.umask = std::stoll(argv[0]);
				++argv; --argc;
			}
			else if (strcmp(currentArg, "--error-port") == 0)
			{
				if (argc <= 0)
				{
					std::cout << "Missing value for --error-port flag.";
					return 1;
				}
				args.error_port = std::stoll(argv[0]);
				++argv; --argc;
			}
			else if (strcmp(currentArg, "--error-token") == 0)
			{
				if (argc <= 0)
				{
					std::cout << "Missing value for --error-token flag.";
					return 1;
				}
				args.error_token = std::stoll(argv[0]);
				++argv; --argc;
			}
			else if (strcmp(currentArg, "--") == 0)
			{
				break;
			}
			else
			{
				std::cout << "Unknown flag: " << currentArg << std::endl;
				return 1;
			}
		}
		catch(const std::exception& e)
		{
			std::cerr << "Error parsing value for " << currentArg << ": " << e.what() << std::endl;
			return 1;
		}
	}

    loader_build_args(user_args_memory, args, argv, envp);
	loader_init_tls();

	loader_register_process(args.arg_count, args.args);
	loader_register_thread(-1, NULL, false);
	loader_register_builtin_areas(commpage, args.args);
	loader_register_existing_fds();

	std::filesystem::path cwd = std::filesystem::current_path();
	char emulatedCwd[PATH_MAX];
	size_t emulatedCwdLength = loader_vchroot_unexpand(cwd.c_str(), emulatedCwd, sizeof(emulatedCwd));
	loader_hserver_call_setcwd(HAIKU_AT_FDCWD, emulatedCwd, emulatedCwdLength);

	((hostcalls*)(((uint8_t*)commpage) + EXTENDED_COMMPAGE_HOSTCALLS_OFFSET))
		->at_exit = &loader_at_exit;

    int retVal = runtime_loader(&args, commpage);

    std::cout << "Runtime loader returned: " << retVal << std::endl;

	loader_unload_runtime();
	loader_free_commpage(commpage);

	delete[] user_args_memory;
    return retVal;
}
