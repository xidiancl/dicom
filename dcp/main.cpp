// ConsoleApplication1.cpp : Defines the entry point for the console application.
//

#include "precomp.h"

using namespace Application::Infrastructure;
using namespace DCM::Operations;

static const struct
{
    const wchar_t* Name;
    unsigned NArgs;
    bool IsRequired;
} DCPArguments[] =
{

{ L"--input-folder",           1, false },
{ L"--output-folder",          1, false },
{ L"--voxelize-mean",          3, false },
{ L"--voxelize-stddev",        3, false },
{ L"--image-average",          0, false },
{ L"--image-concat",           0, false },
{ L"--image-convert-to-csv",   0, false },
{ L"--image-snr",              2, false },
{ L"--image-gfactor",          3, false },
{ L"--image-ssim",             0, false }

};

bool DirectoryExists(const std::wstring& path)
{
    DWORD dwAttrib = GetFileAttributes(path.c_str());

    return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
        (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool FileExists(const std::wstring& path)
{
    DWORD dwAttrib = GetFileAttributes(path.c_str());

    return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
        (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == 0);
}

class DCPApplication : public ApplicationBase<DCPApplication>
{
    HRESULT EnsureOption(const wchar_t* pOptionName)
    {
        bool isSet;
        RETURN_IF_FAILED(IsOptionSet(pOptionName, &isSet));
        if (!isSet)
        {
            RETURN_IF_FAILED(PrintInvalidArgument(true, pOptionName, L"Option not set."));
            return E_FAIL;
        }

        return S_OK;
    }
public:

    template <OperationType TType, typename... TArgs>
    HRESULT RunOperation(TArgs&&... args)
    {
        Application::Infrastructure::DeviceResources resources;
        auto spOperation = MakeOperation<TType>(std::forward<TArgs>(args)...);
        spOperation->Run(resources);
        return S_OK;
    }

    HRESULT Run()
    {
        bool isSet;
        unsigned x, y, z;
        if (SUCCEEDED(IsOptionSet(L"--voxelize-mean", &isSet)) && isSet &&
            SUCCEEDED(GetOptionParameterAt<0>(L"--voxelize-mean", &x)) &&
            SUCCEEDED(GetOptionParameterAt<1>(L"--voxelize-mean", &y)) &&
            SUCCEEDED(GetOptionParameterAt<2>(L"--voxelize-mean", &z)))
        {
            RETURN_IF_FAILED(EnsureOption(L"--input-folder"));
            std::wstring inputFolder;
            RETURN_IF_FAILED(GetOptionParameterAt<0>(L"--input-folder", &inputFolder));
            RETURN_IF_FAILED(RunOperation<OperationType::VoxelizeMeans>(inputFolder, x, y, z));
            return S_OK;
        }

        if (SUCCEEDED(IsOptionSet(L"--voxelize-stddev", &isSet)) && isSet &&
            SUCCEEDED(GetOptionParameterAt<0>(L"--voxelize-stddev", &x)) &&
            SUCCEEDED(GetOptionParameterAt<1>(L"--voxelize-stddev", &y)) &&
            SUCCEEDED(GetOptionParameterAt<2>(L"--voxelize-stddev", &z)))
        {
            RETURN_IF_FAILED(EnsureOption(L"--input-folder"));
            std::wstring inputFolder;
            RETURN_IF_FAILED(GetOptionParameterAt<0>(L"--input-folder", &inputFolder));
            RETURN_IF_FAILED(RunOperation<OperationType::VoxelizeStdDev>(inputFolder, x, y, z));
            return S_OK;
        }

        if (SUCCEEDED(IsOptionSet(L"--image-average", &isSet)) && isSet)
        {
            RETURN_IF_FAILED(EnsureOption(L"--input-folder"));
            std::wstring inputFolder;
            RETURN_IF_FAILED(GetOptionParameterAt<0>(L"--input-folder", &inputFolder));
            RETURN_IF_FAILED(RunOperation<OperationType::AverageImages>(inputFolder));
            return S_OK;
        }

        if (SUCCEEDED(IsOptionSet(L"--image-concat", &isSet)) && isSet)
        {
            RETURN_IF_FAILED(EnsureOption(L"--input-folder"));
            std::wstring inputFolder;
            RETURN_IF_FAILED(GetOptionParameterAt<0>(L"--input-folder", &inputFolder));
            RETURN_IF_FAILED(RunOperation<OperationType::ConcatenateImages>(inputFolder));
            return S_OK;
        }

        if (SUCCEEDED(IsOptionSet(L"--image-convert-to-csv", &isSet)) && isSet)
        {
            RETURN_IF_FAILED(EnsureOption(L"--input-file"));
            std::wstring inputFile;
            RETURN_IF_FAILED(GetOptionParameterAt<0>(L"--input-file", &inputFile));
            RETURN_IF_FAILED(RunOperation<OperationType::ImageToCsv>(inputFile));
            return S_OK;
        }

        if (SUCCEEDED(IsOptionSet(L"--image-snr", &isSet)) && isSet)
        {
            return S_OK;
        }

        if (SUCCEEDED(IsOptionSet(L"--image-gfactor", &isSet)) && isSet)
        {
            return S_OK;
        }

        if (SUCCEEDED(IsOptionSet(L"--image-ssim", &isSet)) && isSet)
        {
        }

        return E_FAIL;
    }

    HRESULT GetLengthOfArgumentsToFollow(wchar_t* pwzArgName, unsigned* nArgsToFollow)
    {
        RETURN_HR_IF_NULL(E_POINTER, nArgsToFollow);
        RETURN_HR_IF_NULL(E_POINTER, pwzArgName);

        *nArgsToFollow = 0;

        auto foundIt = 
            std::find_if(
                std::begin(DCPArguments),
                std::end(DCPArguments),
                [pwzArgName](const auto& argument)
                {
                    return _wcsicmp(argument.Name, pwzArgName) == 0;
                }
            );

        RETURN_HR_IF(E_INVALIDARG, foundIt == std::end(DCPArguments));

        // Set the number of return args to follow
        *nArgsToFollow = foundIt->NArgs;

        return S_OK;
    };

    HRESULT ValidateArgument(const wchar_t* pArgumentName, bool* isValid)
    {
        RETURN_HR_IF_NULL(E_INVALIDARG, pArgumentName);
        RETURN_HR_IF_NULL(E_POINTER, isValid);
        *isValid = true;

        if (_wcsicmp(pArgumentName, L"--input-folder") == 0)
        {
            std::wstring inputFolder;
            *isValid = SUCCEEDED(GetOptionParameterAt<0>(L"--input-folder", &inputFolder));
            RETURN_HR_IF_FALSE(S_OK, *isValid);
            *isValid = DirectoryExists(inputFolder);
            RETURN_HR_IF_FALSE(S_OK, *isValid);
        }

        if (_wcsicmp(pArgumentName, L"--input-file") == 0)
        {
            std::wstring inputFile;
            *isValid = SUCCEEDED(GetOptionParameterAt<0>(L"--input-file", &inputFile));
            RETURN_HR_IF_FALSE(S_OK, *isValid);
            *isValid = FileExists(inputFile);
            RETURN_HR_IF_FALSE(S_OK, *isValid);
        }

        if (_wcsicmp(pArgumentName, L"--voxelize-mean") == 0)
        {


            unsigned tmp;
            *isValid = SUCCEEDED(GetOptionParameterAt<0>(L"--voxelize-mean", &tmp));
            RETURN_HR_IF_FALSE(S_OK, *isValid);
            *isValid = SUCCEEDED(GetOptionParameterAt<1>(L"--voxelize-mean", &tmp));
            RETURN_HR_IF_FALSE(S_OK, *isValid);
            *isValid = SUCCEEDED(GetOptionParameterAt<2>(L"--voxelize-mean", &tmp));
            RETURN_HR_IF_FALSE(S_OK, *isValid);
        }

        if (_wcsicmp(pArgumentName, L"--voxelize-stddev") == 0)
        {
            unsigned tmp;
            *isValid = SUCCEEDED(GetOptionParameterAt<0>(L"--voxelize-stddev", &tmp));
            RETURN_HR_IF_FALSE(S_OK, *isValid);
            *isValid = SUCCEEDED(GetOptionParameterAt<1>(L"--voxelize-stddev", &tmp));
            RETURN_HR_IF_FALSE(S_OK, *isValid);
            *isValid = SUCCEEDED(GetOptionParameterAt<2>(L"--voxelize-stddev", &tmp));
            RETURN_HR_IF_FALSE(S_OK, *isValid);
        }
        return S_OK;
    }

    HRESULT PrintHelp()
    {
        static const auto HELP = LR"(
DCPApplication Reference (dcaapplication.exe)

Parameter               Usage
--------------------------------------------------------------------------------------------------------
--dataPath              The path (absolute: c:\..., or relative) to a folder containing dicoms images.

                        Examples:
                        dcaapplication.exe --dataPath "C:\foldername" [[OTHER PARAMETERS]]
                        dcaapplication.exe --dataPath "C:\foldername\subfolder\" [[OTHER PARAMETERS]]
                        dcaapplication.exe --dataPath "folder" [[OTHER PARAMETERS]]
                        dcaapplication.exe --dataPath "folder\subdolder\" [[OTHER PARAMETERS]]

--mean                  Compute the voxel means

                        Examples:
                        dcaapplication.exe --mean [[OTHER PARAMETERS]]

--stddev                Compute the voxel standard deviations

                        Examples:
                        dcaapplication.exe --stddev [[OTHER PARAMETERS]]

--voxelize-mean         The dimensions of each voxel in the form: "dim_one_size dim_two_size dim_three_size"
                        in millimeters.

                        Example: Setting 2cm cubed voxel size
                        dcaapplication.exe --voxel-shape 20 20 20 [[OTHER PARAMETERS]]
)";
        wprintf(HELP);
        return S_OK;
    }
};


int main()
{
    DCPApplication::Execute();
    return 0;
}

