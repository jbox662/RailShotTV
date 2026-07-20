#pragma once

namespace railshot {

class ComInitializer {
public:
    ComInitializer();
    ~ComInitializer();
    bool ok() const { return m_ok; }
    ComInitializer(const ComInitializer&) = delete;
    ComInitializer& operator=(const ComInitializer&) = delete;
private:
    bool m_ok = false;
    bool m_initializedHere = false;
};

} // namespace railshot
