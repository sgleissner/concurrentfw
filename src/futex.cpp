/*
 * futex.cpp
 *
 * (C) 2017-2022 by Simon Gleissner <simon@gleissner.de>, http://concurrentfw.de
 *
 * This file is distributed under the ISC license, see file LICENSE.
 */


/**
 * Fast Userspace Mutex
 *
 * futex algorithm based on work of Ulrich Drepper
 * see: http://www.akkadia.org/drepper/futex.pdf
 *
 * As the atomic futex storage 'value' is the only object inside the class,
 * a memory order 'relaxed' would be sufficient for the class objects itself.
 * therefore release-aquire ordering must be used.
 * see: http://en.cppreference.com/w/cpp/atomic/memory_order
 */

#include <system_error>

#include <errno.h>

#include <concurrentfw/futex.hpp>


namespace ConcurrentFW
{

void Futex::wait(int cached_state)
{
    // we don't have aquired the futex, so we need to inform the others that we are waiting
    // value (cached_state) is now 1 (LOCKED_NOWAITERS) or 2 (LOCKED_WAITERS)
    if (cached_state != State::LOCKED_WAITERS)  // locked, no other waiting
    {
        // memory order: might be the last atomic operation before critical section,
        // therefore use atomic-aquire here
        cached_state = value.exchange<AtomicMemoryOrder::ACQUIRE>(State::LOCKED_WAITERS);
    }

    while (cached_state != State::UNLOCKED)
    {
        if ((futex_wait(State::LOCKED_WAITERS) != 0)                // wait if the value is still '2'
            && (errno != EAGAIN) && (errno != EINTR)) [[unlikely]]  // and check for errors
        {
            throw std::system_error(errno, std::system_category(), "FutexBase::futex_wait()");
        }
        // memory order: might be the last atomic operation before critical section,
        // therefore use atomic-aquire here
        cached_state = value.exchange<AtomicMemoryOrder::ACQUIRE>(State::LOCKED_WAITERS);
    }
}

bool Futex::wait_timeout(int cached_state, const struct timespec* timeout_relative)
{
    // we don't have aquired the futex, so we need to inform the others that we are waiting
    // value (cached_state) is now 1 (LOCKED_NOWAITERS) or 2 (LOCKED_WAITERS)
    if (cached_state != State::LOCKED_WAITERS)  // locked, no other waiting
    {
        // memory order: might be the last atomic operation before critical section,
        // therefore use atomic-aquire here
        cached_state = value.exchange<AtomicMemoryOrder::ACQUIRE>(State::LOCKED_WAITERS);
    }

    while (cached_state != State::UNLOCKED)
    {
        if ((futex_wait(State::LOCKED_WAITERS, timeout_relative) != 0)  // wait if the value is still '2'
            && (errno != EAGAIN) && (errno != EINTR))                   // and check for errors
        {
            if (errno == ETIMEDOUT) [[likely]]
                return false;  // timeout, futex not owned
            throw std::system_error(errno, std::system_category(), "FutexBase::futex_wait()");
        }
        // memory order: might be the last atomic operation before critical section,
        // therefore use atomic-aquire here
        cached_state = value.exchange<AtomicMemoryOrder::ACQUIRE>(State::LOCKED_WAITERS);
    }

    return true;  // futex owned
}

void Futex::wake(void)
{  // we are per definition the only running code inside the lock
    // memory order: atomic-release already done in unlock(), so we are relaxed
    value.store<AtomicMemoryOrder::RELAXED>(State::UNLOCKED);  // unlock futex
    if (futex_wake() < 0) [[unlikely]]                         // wake one thread
    {
        throw std::system_error(errno, std::system_category(), "FutexBase::futex_wake()");
    }
}

}  // namespace ConcurrentFW
