#ifndef SINGLETON_H_
#define SINGLETON_H_

#include <memory>
#include <mutex>

#define SINGLETON_DECLARE(T)         \
    friend class std::shared_ptr<T>; \
    friend class Singleton<T>;

template <typename T>
class Singleton
{
public:
    // 获取全局单例对象
    template <typename... Args>
    static std::shared_ptr<T> Instance(Args&&... args)
    {
        if (!m_singleton) {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_singleton) {
                // m_singleton = std::make_shared<T>(std::forward<Args>(args)...);
                m_singleton.reset(new T(std::forward<Args>(args)...));
            }
        }

        return m_singleton;
    }

    // 主动析构单例对象（一般不需要主动析构，除非特殊需求）
    static void ReleaseInstance()
    {
        if (m_singleton) {
            m_singleton.reset();
            m_singleton = nullptr;
        }
    }

private:
    Singleton() = default;
    virtual ~Singleton() = default;

    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

private:
    static std::shared_ptr<T> m_singleton;
    static std::mutex m_mutex;
};

template <typename T>
std::shared_ptr<T> Singleton<T>::m_singleton = nullptr;

template <typename T>
std::mutex Singleton<T>::m_mutex;

#endif
