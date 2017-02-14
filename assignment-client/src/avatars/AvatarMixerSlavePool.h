//
//  AvatarMixerSlavePool.h
//  assignment-client/src/avatar
//
//  Created by Brad Hefta-Gaub on 2/14/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AvatarMixerSlavePool_h
#define hifi_AvatarMixerSlavePool_h

#include <condition_variable>
#include <mutex>
#include <vector>

#include <tbb/concurrent_queue.h>

#include <QThread>

#include <NodeList.h>

#include "AvatarMixerSlave.h"

class AvatarMixerSlavePool;

class AvatarMixerSlaveThread : public QThread, public AvatarMixerSlave {
    Q_OBJECT
    using ConstIter = NodeList::const_iterator;
    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;

public:
    AvatarMixerSlaveThread(AvatarMixerSlavePool& pool) : _pool(pool) {}

    void run() override final;

private:
    friend class AvatarMixerSlavePool;

    void wait();
    void notify(bool stopping);
    bool try_pop(SharedNodePointer& node);

    AvatarMixerSlavePool& _pool;
    bool _stop { false };
};

// Slave pool for audio mixers
//   AvatarMixerSlavePool is not thread-safe! It should be instantiated and used from a single thread.
class AvatarMixerSlavePool {
    using Queue = tbb::concurrent_queue<SharedNodePointer>;
    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;
    using ConditionVariable = std::condition_variable;

public:
    using ConstIter = NodeList::const_iterator;

    AvatarMixerSlavePool(int numThreads = QThread::idealThreadCount()) { setNumThreads(numThreads); }
    ~AvatarMixerSlavePool() { resize(0); }

    // iterate over all slaves
    void each(std::function<void(AvatarMixerSlave& slave)> functor);

    void setNumThreads(int numThreads);
    int numThreads() { return _numThreads; }

    // Jobs the slave pool can do...
    void processIncomingPackets(ConstIter begin, ConstIter end);


private:
    void resize(int numThreads);

    std::vector<std::unique_ptr<AvatarMixerSlaveThread>> _slaves;

    friend void AvatarMixerSlaveThread::wait();
    friend void AvatarMixerSlaveThread::notify(bool stopping);
    friend bool AvatarMixerSlaveThread::try_pop(SharedNodePointer& node);

    // synchronization state
    Mutex _mutex;
    ConditionVariable _slaveCondition;
    ConditionVariable _poolCondition;
    int _numThreads { 0 };
    int _numStarted { 0 }; // guarded by _mutex
    int _numFinished { 0 }; // guarded by _mutex
    int _numStopped { 0 }; // guarded by _mutex

    // frame state
    Queue _queue;
    unsigned int _frame { 0 };
    float _throttlingRatio { 0.0f };
    ConstIter _begin;
    ConstIter _end;
};

#endif // hifi_AvatarMixerSlavePool_h
