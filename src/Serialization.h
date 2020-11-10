#pragma once

void WriteString(SKSE::SerializationInterface* intfc, std::string const& string)
{
    uint32_t length = static_cast<uint32_t>(string.length());
    intfc->WriteRecordData(&length, sizeof(length));
    intfc->WriteRecordData(string.c_str(), length);
}

template<typename Ty> void WriteContainerData(SKSE::SerializationInterface* intfc, Ty const& container)
{
    uint32_t size = static_cast<uint32_t>(container.size());
    intfc->WriteRecordData(&size, sizeof(size));
    for (auto const& kvp : container)
        intfc->WriteRecordData(&kvp, sizeof(kvp));
}

template<typename Ty> Ty ReadDataHelper(SKSE::SerializationInterface* intfc, uint32_t& length)
{
    uint32_t size = sizeof(Ty);
    if (size > length)
        throw std::length_error("savegame data ended unexpected");
    length -= size;
    Ty result;
    intfc->ReadRecordData(&result, size);
    return result;
}

std::string ReadString(SKSE::SerializationInterface* intfc, uint32_t& length)
{
    uint32_t strlength = ReadDataHelper<uint32_t>(intfc, length);
    if (strlength > length)
        throw std::length_error("savegame data ended unexpected");
    length -= strlength;
    char* buf = new char[strlength + 1];
    intfc->ReadRecordData(buf, strlength);
    buf[strlength] = '\0';
    std::string result(buf);
    delete[] buf;
    return result;
}

template <typename T>
bool WriteData(SKSE::SerializationInterface* intfc, const T* data)
{
    return intfc->WriteRecordData(data, sizeof(T));
}

template <typename T>
bool ReadData(SKSE::SerializationInterface* intfc, T* data)
{
    return intfc->ReadRecordData(data, sizeof(T)) > 0;
}