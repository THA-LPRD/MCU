#ifndef RLEOBJECT_H_
#define RLEOBJECT_H_

#include <vector>
#include <cstdint>

class RLEObject {
public:
    RLEObject() = default;
    ~RLEObject() = default;
    void Compress(const std::vector<uint8_t>& data);
    std::vector<uint8_t> GetData(std::vector<uint8_t>& data);
private:
    std::vector<uint8_t> m_Data;
};

#endif /*RLEOBJECT_H_*/