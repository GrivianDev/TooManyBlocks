#ifndef TOOMANYBLOCKS_RENDERAPIOBJECT_H
#define TOOMANYBLOCKS_RENDERAPIOBJECT_H

// Base class wich prevents copying, but allows moving
class RenderApiObject {
protected:
    unsigned int m_rendererId;

    RenderApiObject(unsigned int id = 0) noexcept : m_rendererId(id) {};
    RenderApiObject(RenderApiObject&& other) noexcept : m_rendererId(other.m_rendererId) { other.m_rendererId = 0; }
    RenderApiObject(const RenderApiObject&) = delete;
    virtual ~RenderApiObject() = default;

public:
    inline unsigned int rendererId() const { return m_rendererId; }

    inline bool isValid() const { return m_rendererId != 0; }

    RenderApiObject& operator=(RenderApiObject&& other) noexcept {
        if (this != &other) {
            m_rendererId = other.m_rendererId;
            other.m_rendererId = 0;
        }
        return *this;
    }
    RenderApiObject& operator=(const RenderApiObject&) = delete;
};

#endif