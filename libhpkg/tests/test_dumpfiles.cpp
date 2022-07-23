#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>

#include <libhpkg/HpkgFileExtractor.h>

void Print(const std::vector<std::shared_ptr<LibHpkg::Model::Attribute>>& attributes, int generation = 0)
{
    std::string indent(generation * 2, ' ');
    for (const auto& a : attributes)
    {
        std::cout << indent << a->ToString() << std::endl;
        Print(a->GetChildAttributes(), generation + 1);
    }
}

int main()
{
    std::filesystem::path filePath = std::filesystem::current_path() / "tipster-1.1.1-1-x86_64.hpkg";

    LibHpkg::HpkgFileExtractor extractor(filePath);

    Print(extractor.GetToc());

    return 0;
}