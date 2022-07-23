#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>

#include <libhpkg/AttributeIterator.h>
#include <libhpkg/HpkgFileExtractor.h>
#include <libhpkg/Model/AttributeId.h>
#include <libhpkg/Model/PkgArchitecture.h>
#include <libhpkg/Model/PkgVersion.h>

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

    auto iterator = extractor.GetPackageAttributesIterator();
    auto context = extractor.GetPackageAttributeContext();

    while (iterator->HasNext())
    {
        auto attribute = iterator->Next();
        if (attribute->GetAttributeId() == LibHpkg::Model::AttributeId::PACKAGE_NAME)
        {
            std::cout << "Name: " << attribute->GetValue<std::string>(context) << std::endl;
        }
        else if (attribute->GetAttributeId() == LibHpkg::Model::AttributeId::PACKAGE_ARCHITECTURE)
        {
            int architecture = attribute->GetValue<mpz_class>(context).get_si();
            switch (architecture)
            {
                case (int)LibHpkg::Model::PkgArchitecture::X86_64:
                    std::cout << "Architecture: x86_64" << std::endl;
                    break;
                case (int)LibHpkg::Model::PkgArchitecture::X86:
                    std::cout << "Architecture: x86" << std::endl;
                    break;
                case (int)LibHpkg::Model::PkgArchitecture::X86_GCC2:
                    std::cout << "Architecture: x86_gcc2" << std::endl;
                    break;
                default:
                    std::cout << "Architecture: unknown" << std::endl;
                    break;
            }
        }
        else if (attribute->GetAttributeId() == LibHpkg::Model::AttributeId::PACKAGE_VERSION_MAJOR)
        {
            std::string major = attribute->GetValue<std::string>(context);
            std::string minor, micro, preRelease;
            std::optional<int> revision;

            auto childAttributes = attribute->GetChildAttributes();
            for (const auto & childAttribute: childAttributes)
            {
                if (childAttribute->GetAttributeId() == LibHpkg::Model::AttributeId::PACKAGE_VERSION_MINOR)
                {
                    minor = childAttribute->GetValue<std::string>(context);
                }
                else if (childAttribute->GetAttributeId() == LibHpkg::Model::AttributeId::PACKAGE_VERSION_MICRO)
                {
                    micro = childAttribute->GetValue<std::string>(context);
                }
                else if (childAttribute->GetAttributeId() == LibHpkg::Model::AttributeId::PACKAGE_VERSION_PRE_RELEASE)
                {
                    preRelease = childAttribute->GetValue<std::string>(context);
                }
                else if (childAttribute->GetAttributeId() == LibHpkg::Model::AttributeId::PACKAGE_VERSION_REVISION)
                {
                    revision = childAttribute->GetValue<mpz_class>(context).get_si();
                }
            }


            std::cout << "Version: " << LibHpkg::Model::PkgVersion(major, minor, micro, preRelease, revision).ToString() << std::endl;
        }
    }

    Print(extractor.GetToc());

    return 0;
}