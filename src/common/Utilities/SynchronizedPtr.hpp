#pragma once

#include <type_traits>
#include "ThreadsafetyDebugger.h"

namespace util
{
    template<class T, class Lock>
    class synchronized_ptr
    {
        template<class U, class V>
        friend class synchronized_ptr;

        T* _ptr;
        Lock _lock;

    public:
        synchronized_ptr() : _ptr(nullptr), _lock() { InitLockFlag(); }
        synchronized_ptr(T* ptr, Lock&& lock) : _ptr(ptr), _lock(std::move(lock)) { InitLockFlag(); }
        synchronized_ptr(std::nullptr_t) : synchronized_ptr() {}
        synchronized_ptr(const synchronized_ptr& other) = delete;

        synchronized_ptr(synchronized_ptr&& other) : _ptr(other._ptr), _lock(std::move(other._lock)) 
        { 
            other._ptr = nullptr;
#ifdef THREADSAFETY_DEBUG_ENABLE
            _lockIndex = other._lockIndex;
            other.InitLockFlag();
#endif
        }
        
        template<class OtherT, typename = std::enable_if_t<std::is_convertible<OtherT*, T*>::value>>
        synchronized_ptr(synchronized_ptr<OtherT, Lock>&& other) : _ptr(other._ptr), _lock(std::move(other._lock)) 
        { 
            other._ptr = nullptr;
#ifdef THREADSAFETY_DEBUG_ENABLE
            _lockIndex = other._lockIndex;
            other.InitLockFlag();
#endif
        }

        void clear()
        {
            _ptr = nullptr;
            _lock = Lock();
        }

        explicit operator bool() const { return (_ptr != nullptr); }

        T& operator*() { return (*_ptr); }
        const T& operator*() const { return (*_ptr); }

        T* operator->() { return _ptr; }
        const T* operator->() const { return _ptr; }

        synchronized_ptr& operator=(synchronized_ptr&& other)
        {
            _ptr = other._ptr;
            _lock = std::move(other._lock);
            other._ptr = nullptr;
#ifdef THREADSAFETY_DEBUG_ENABLE
            _lockIndex = other._lockIndex;
            other.InitLockFlag();
#endif
            return *this;
        };

#ifdef THREADSAFETY_DEBUG_ENABLE
    private:
        void InitLockFlag() { _lockIndex = DeadlockDetection::Index::NONE; }
        DeadlockDetection::Index _lockIndex;
        void LockFlag(DeadlockDetection::Index idx)
        {
            DeadlockDetection::Increase(idx);
            _lockIndex = idx;
        }
    public:
        synchronized_ptr(T* ptr, Lock&& lock, DeadlockDetection::Index idx) : synchronized_ptr(ptr, std::move(lock)) 
        { 
            LockFlag(idx);
        }
        ~synchronized_ptr() 
        {
            if (_lockIndex != DeadlockDetection::Index::NONE)
                DeadlockDetection::Decrease(_lockIndex);
        }
#else
        void InitLockFlag() { }
#endif
    };

    template<class T, class Lock>
    class synchronized_ptr<std::shared_ptr<T>, Lock>
    {
        template<class U, class V>
        friend class synchronized_ptr;

        std::shared_ptr<T> _ptr;
        Lock _lock;

    public:
        synchronized_ptr() : _ptr(nullptr), _lock() { InitLockFlag(); }
        synchronized_ptr(std::shared_ptr<T> ptr, Lock&& lock) : _ptr(std::move(ptr)), _lock(std::move(lock)) { InitLockFlag(); }
        synchronized_ptr(std::nullptr_t) : synchronized_ptr() {}
        synchronized_ptr(const synchronized_ptr& other) = delete;

        synchronized_ptr(synchronized_ptr&& other) : _ptr(std::move(other._ptr)), _lock(std::move(other._lock))
        {
#ifdef THREADSAFETY_DEBUG_ENABLE
            _lockIndex = other._lockIndex;
            other.InitLockFlag();
#endif
        }

        template<class OtherT, typename = std::enable_if_t<std::is_convertible<OtherT*, T*>::value>>
        synchronized_ptr(synchronized_ptr<OtherT, Lock>&& other) : _ptr(std::move(other._ptr)), _lock(std::move(other._lock))
        {
            other._ptr = nullptr;
#ifdef THREADSAFETY_DEBUG_ENABLE
            _lockIndex = other._lockIndex;
            other.InitLockFlag();
#endif
        }

        void clear()
        {
            _lock = Lock();
            _ptr = nullptr;
        }

        explicit operator bool() const { return (_ptr.get() != nullptr); }

        T& operator*() { return (*_ptr.get()); }
        const T& operator*() const { return (*_ptr.get()); }

        T* operator->() { return _ptr.get(); }
        const T* operator->() const { return _ptr.get(); }

        synchronized_ptr& operator=(synchronized_ptr&& other)
        {
            _ptr = std::move(other._ptr);
            _lock = std::move(other._lock);
#ifdef THREADSAFETY_DEBUG_ENABLE
            _lockIndex = other._lockIndex;
            other.InitLockFlag();
#endif
            return *this;
        };

#ifdef THREADSAFETY_DEBUG_ENABLE
    private:
        void InitLockFlag() { _lockIndex = DeadlockDetection::Index::NONE; }
        DeadlockDetection::Index _lockIndex;
        void LockFlag(DeadlockDetection::Index idx)
        {
            DeadlockDetection::Increase(idx);
            _lockIndex = idx;
        }
    public:
        synchronized_ptr(std::shared_ptr<T> ptr, Lock&& lock, DeadlockDetection::Index idx) : synchronized_ptr(std::move(ptr), std::move(lock))
        {
            LockFlag(idx);
        }
        ~synchronized_ptr()
        {
            if (_lockIndex != DeadlockDetection::Index::NONE)
                DeadlockDetection::Decrease(_lockIndex);
        }
#else
        void InitLockFlag() { }
#endif
    };

    template<class T, class Lock>
    class synchronized_reference
    {
        T& _ptr;
        Lock _lock;

    public:
        synchronized_reference(T& ptr, Lock&& lock) : _ptr(ptr), _lock(std::move(lock)) {}

        void clear()
        {
            _lock = Lock();
        }

        T& operator*() { return _ptr; }
        T* operator->() { return &_ptr; }
    };
}
