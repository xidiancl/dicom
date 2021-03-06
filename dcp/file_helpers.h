#pragma once
//
// DicomFile.h
// Declaration of the App class.
//

#pragma once

namespace DCM
{

enum FileType { Directory, File };

inline HRESULT GetChildren(
    const std::wstring& input,
    std::vector<std::wstring>* children,
    FileType type = FileType::File)
{
    RETURN_HR_IF_NULL(E_POINTER, children);
    children->clear();

    namespace fs = std::experimental::filesystem;

    for (auto& file : fs::directory_iterator(input))
    {
        std::wstring path = file.path();
        DWORD dwAttrib = GetFileAttributes(path.c_str());

        if (type == FileType::Directory &&
            (dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
        {
            children->push_back(std::move(path));
        }
        else if (type == FileType::File &&
                (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
            children->push_back(std::move(path));
        }
    }

    return S_OK;
}

inline HRESULT GetMetadataFiles(std::wstring inputFolder, std::vector<std::shared_ptr<DicomFile>>* outFiles)
{
    RETURN_HR_IF_NULL(E_POINTER, outFiles);
    outFiles->clear();

    std::vector<std::wstring> children;
    RETURN_IF_FAILED(GetChildren(inputFolder, &children));

    std::vector<std::shared_ptr<DicomFile>> metadataFiles(children.size());
    std::transform(std::begin(children), std::end(children), std::begin(metadataFiles),
        [](const std::wstring& fileName)
        {
            std::shared_ptr<DicomFile> dicomFile;
            FAIL_FAST_IF_FAILED(MakeDicomMetadataFile(fileName, &dicomFile));
            return dicomFile;
        });

    *outFiles = metadataFiles;

    return S_OK;
}

inline float ParseFloat(const std::wstring& str)
{
    wchar_t* stopString;
    return static_cast<float>(wcstod(str.c_str(), &stopString));
}

inline std::vector<float> GetAsFloatVector(const std::wstring& str)
{
    std::wistringstream wisstream(str);
    std::vector<float> values;
    std::wstring token;
    while (std::getline(wisstream, token, L'\\'))
    {
        wchar_t* stopString;
        values.push_back(static_cast<float>(wcstod(token.c_str(), &stopString)));
    }
    return values;
}



// Sort the files by their order in the scene
inline HRESULT SortFilesInScene(std::vector<std::shared_ptr<DicomFile>>* outFiles)
{
    // Get orientation
    auto firstFile = outFiles->at(0);

    std::wstring firstImageOrientation;
    firstFile->GetAttribute(Tags::ImageOrientationPatient, &firstImageOrientation);

    // Ensure all images are oriented in same direction
    auto foundNotMatchingIt =
        std::find_if(
            std::begin(*outFiles), std::end(*outFiles),
            [&firstImageOrientation](auto dicomFile) {
                std::wstring imageOrientation;
                dicomFile->GetAttribute(Tags::ImageOrientationPatient, &imageOrientation);
                return _wcsicmp(imageOrientation.c_str(), firstImageOrientation.c_str()) != 0;
            });
    
    FAIL_FAST_IF_FALSE(foundNotMatchingIt == std::end(*outFiles));

    // Get first images position
    std::wstring firstImagePosition;
    firstFile->GetAttribute(Tags::ImagePositionPatient, &firstImagePosition);
    auto posValues = GetAsFloatVector(firstImagePosition);
    FAIL_FAST_IF_TRUE(posValues.size() != 3);

    // Construct image frame of reference to zorder files
    auto orientationValues = GetAsFloatVector(firstImageOrientation);
    FAIL_FAST_IF_TRUE(orientationValues.size() != 6);
    
    auto xaxis = DirectX::XMVectorSet(
        orientationValues[0],
        orientationValues[1],
        orientationValues[2],
        0.f);
    auto yaxis = DirectX::XMVectorSet(
        orientationValues[3],
        orientationValues[4],
        orientationValues[5],
        0.f);
    auto zaxis = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(xaxis, yaxis));
    
    const DirectX::XMFLOAT4X4 refFrameToWorld4x4(
        orientationValues[0], orientationValues[1], orientationValues[2], 0,
        orientationValues[3], orientationValues[4], orientationValues[5], 0,
        DirectX::XMVectorGetX(zaxis), DirectX::XMVectorGetY(zaxis), DirectX::XMVectorGetZ(zaxis), 0,
        posValues[0], posValues[1], posValues[2], 1
    );

    auto refFrameToWorld = DirectX::XMLoadFloat4x4(&refFrameToWorld4x4);
    DirectX::XMVECTOR determinant;
    auto worldToRefFrame = DirectX::XMMatrixInverse(&determinant, refFrameToWorld);
    
    // Sort the files by their order in the scene
    std::sort(std::begin(*outFiles), std::end(*outFiles),
        [&worldToRefFrame](auto left, auto right)
        {
            std::wstring leftImagePosition, rightImagePosition;
            left->GetAttribute(Tags::ImagePositionPatient, &leftImagePosition);
            auto leftPosValues = GetAsFloatVector(leftImagePosition);
            FAIL_FAST_IF_TRUE(leftPosValues.size() != 3);
            auto leftVec = DirectX::XMVectorSet(leftPosValues[0], leftPosValues[1], leftPosValues[2], 1.f);
            leftVec = DirectX::XMVector4Transform(leftVec, worldToRefFrame);

            right->GetAttribute(Tags::ImagePositionPatient, &rightImagePosition);
            auto rightPosValues = GetAsFloatVector(rightImagePosition);
            FAIL_FAST_IF_TRUE(rightPosValues.size() != 3);
            auto rightVec = DirectX::XMVectorSet(rightPosValues[0], rightPosValues[1], rightPosValues[2], 1.f);
            rightVec = DirectX::XMVector4Transform(rightVec, worldToRefFrame);

            return DirectX::XMVectorGetZ(leftVec) > DirectX::XMVectorGetZ(rightVec);
        });

    return S_OK;
}

}