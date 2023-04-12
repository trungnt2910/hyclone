#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <unordered_set>

#include "ksyscalls.h"
#include "haiku_debugger.h"

#define MONIKA_SYSCALL_PREFIX "_moni_"
#define KENREL_SYSCALL_PREFIX "_kern_"

#define STRINGSIZE(x) (sizeof(x) - 1)

std::string GetParameterType(int size)
{
    switch (size)
    {
        case 0:
            return "void";
        case 1:
            return "uint8_t";
        case 2:
            return "uint16_t";
        case 4:
            return "uint32_t";
        case 8:
            return "uint64_t";
        default:
            assert(!"Unsupported size");
    }
}

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::cout << "Usage: " << argv[0] << " <path to monika_implemented.txt> <path to output file>" << std::endl;
        return 1;
    }

    std::ifstream monika_implemented(argv[1]);
    std::ofstream output(argv[2]);

    output << "#include <cstdint>\n\n";
    output << "#include \"extended_commpage.h\"\n\n";
    output << "#include \"export.h\"\n\n";
    output << "#include \"haiku_errors.h\"\n";
    output << "extern \"C\"\n{\n\n";

    output << "void syscall_dispatcher(uint32_t callIndex, void* args, uint64_t* _returnValue);\n\n";

    std::unordered_set<std::string> implemented_syscalls;
    std::string line;
    while (std::getline(monika_implemented, line))
    {
        implemented_syscalls.insert(KENREL_SYSCALL_PREFIX + line.substr(STRINGSIZE(MONIKA_SYSCALL_PREFIX)));
    }
    
    for (int i = 0; i < kSyscallCount; i++)
    {
        bool implemented = implemented_syscalls.contains(kExtendedSyscallInfos[i].name);

        if (implemented)
        {
            output << "extern ";
        }

        output << GetParameterType(kExtendedSyscallInfos[i].return_type.size)
                  << " "
                  << MONIKA_SYSCALL_PREFIX + std::string(kExtendedSyscallInfos[i].name).substr(STRINGSIZE(KENREL_SYSCALL_PREFIX))
                  << "(";

        for (int j = 0; j < kExtendedSyscallInfos[i].parameter_count; ++j)
        {
            output << GetParameterType(kExtendedSyscallInfos[i].parameters[j].size);

            if (j < kExtendedSyscallInfos[i].parameter_count - 1)
            {
                output << ", ";
            }
        }

        output << ")";

        if (implemented)
        {
            output << ";\n";
        }
        else
        {
            output << "\n{\n";
            output << "    GET_HOSTCALLS()->printf(\"stub: " << kExtendedSyscallInfos[i].name << "\\n\");\n";
            output << "    GET_SERVERCALLS()->debug_output(\"stub: " << kExtendedSyscallInfos[i].name << "\", " << STRINGSIZE("stub: ") + strlen(kExtendedSyscallInfos[i].name) << ");\n";
            output << "    while (true) { GET_HOSTCALLS()->at_exit(1); }\n"; 
            output << "}\n";
        }
    }

    for (int i = 0; i < kSyscallCount; i++)
    {
        output << GetParameterType(kExtendedSyscallInfos[i].return_type.size)
                  << " MONIKA_EXPORT "
                  << kExtendedSyscallInfos[i].name
                  << "(";

        for (int j = 0; j < kExtendedSyscallInfos[i].parameter_count; ++j)
        {
            output << GetParameterType(kExtendedSyscallInfos[i].parameters[j].size);
            output << " ";
            output << "arg" << j;

            if (j < kExtendedSyscallInfos[i].parameter_count - 1)
            {
                output << ", ";
            }
        }

        output << ")\n";
        output << "{\n";
        output << "    uint8_t args[" << sizeof(debug_pre_syscall::args) << "];\n";
        for (int j = 0; j < kExtendedSyscallInfos[i].parameter_count; ++j)
        {
            output << "    *((" << GetParameterType(kExtendedSyscallInfos[i].parameters[j].used_size) << "*)(args + " << kExtendedSyscallInfos[i].parameters[j].offset << ")) = arg" << j << ";\n";
        }
        output << "    uint64_t returnValue;\n";
        output << "    syscall_dispatcher(" << i << ", args, &returnValue);\n";
        if (kExtendedSyscallInfos[i].return_type.size != 0)
        {
            output << "    return (" << GetParameterType(kExtendedSyscallInfos[i].return_type.size) << ")returnValue;\n";
        }
        else
        {
            output << "    return;\n";
        }
        output << "}\n";
    }

    output << "void syscall_dispatcher(uint32_t callIndex, void* args, uint64_t* _returnValue)\n";
    output << "{\n";
    output << "    GET_HOSTCALLS()->debugger_pre_syscall(callIndex, args);\n";
    output << "    switch (callIndex)\n";
    output << "    {\n";
    for (int i = 0; i < kSyscallCount; i++)
    {
        auto name = MONIKA_SYSCALL_PREFIX + std::string(kExtendedSyscallInfos[i].name).substr(STRINGSIZE(KENREL_SYSCALL_PREFIX));
        output << "        case " << i << ":\n";
        if (kExtendedSyscallInfos[i].return_type.size == 0)
        {
            output << "            " << name << "(";
        }
        else
        {
            output << "            *_returnValue = " << name << "(";
        }
        for (int j = 0; j < kExtendedSyscallInfos[i].parameter_count; ++j)
        {
            output << "(" << GetParameterType(kExtendedSyscallInfos[i].parameters[j].size) 
                   << ")*((" << GetParameterType(kExtendedSyscallInfos[i].parameters[j].used_size)
                   << "*)((uint8_t*)args + " << kExtendedSyscallInfos[i].parameters[j].offset << "))";
            if (j < kExtendedSyscallInfos[i].parameter_count - 1)
            {
                output << ", ";
            }
        }
        output << ");\n";
        output << "        break;\n";
    }
    output << "        default:\n";
    output << "            *_returnValue = B_BAD_VALUE;\n";
    output << "        break;\n";
    output << "    }\n";
    output << "    GET_HOSTCALLS()->debugger_post_syscall(callIndex, args, *_returnValue);\n";
    output << "}\n";

    output << "\n}\n";
}