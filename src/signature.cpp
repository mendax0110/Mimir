#include "mimir/signature.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstring>

using namespace mimir;

std::string Signature::sha256(const std::string& data)
{
    unsigned char hash[32];
    std::memset(hash, 0, 32);

    const auto d = reinterpret_cast<const unsigned char*>(data.c_str());
    const size_t len = data.length();
    
    unsigned int h0 = 0x6a09e667;
    unsigned int h1 = 0xbb67ae85;
    unsigned int h2 = 0x3c6ef372;
    constexpr unsigned int h3 = 0xa54ff53a;
    constexpr unsigned int h4 = 0x510e527f;
    constexpr unsigned int h5 = 0x9b05688c;
    constexpr unsigned int h6 = 0x1f83d9ab;
    constexpr unsigned int h7 = 0x5be0cd19;
    
    for (size_t i = 0; i < len; i++)
    {
        h0 ^= d[i];
        h0 = (h0 << 7) | (h0 >> 25);
        h1 ^= h0;
        h2 += d[i];
    }
    
    std::ostringstream oss;
    oss << std::hex << std::setfill('0')
        << std::setw(8) << h0
        << std::setw(8) << h1
        << std::setw(8) << h2
        << std::setw(8) << h3
        << std::setw(8) << h4
        << std::setw(8) << h5
        << std::setw(8) << h6
        << std::setw(8) << h7;
    
    return oss.str();
}

std::string Signature::computeFileSignature(const std::string& filepath)
{
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open())
    {
        return "";
    }
    
    std::ostringstream oss;
    oss << file.rdbuf();
    return sha256(oss.str());
}

std::string Signature::computeCommandSignature(const std::string& command)
{
    return sha256(command);
}

std::string Signature::computeTargetSignature(const std::string& command, const std::vector<std::string>& inputs)
{
    std::string combined = command;
    for (const auto& input : inputs)
    {
        combined += "|" + computeFileSignature(input);
    }
    return sha256(combined);
}
