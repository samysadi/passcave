#include "runguard.h"

RunGuard::RunGuard(const QString& key) :
    key(key),
	memLockKey(passcave::RunGuardHash + key + "_m"),
	sharedmemKey(passcave::RunGuardHash + key + "_s"),
    sharedMem(sharedmemKey),
    memLock(memLockKey, 1) {
    memLock.acquire();
    {
        QSharedMemory fix(sharedmemKey);
        fix.attach();
    }
    memLock.release();
}

RunGuard::~RunGuard() {
    release();
}

bool RunGuard::isAlreadyRunning() {
    if (sharedMem.isAttached())
        return false;

    memLock.acquire();
    const bool isRunning = sharedMem.attach();
    if (isRunning)
        sharedMem.detach();
    memLock.release();

    return isRunning;
}

bool RunGuard::tryToRun() {
    if (isAlreadyRunning())
        return false;

    memLock.acquire();
    const bool result = sharedMem.create(sizeof(quint64));
    memLock.release();
    if (!result) {
        release();
        return false;
    }

    return true;
}

void RunGuard::release() {
    memLock.acquire();
    if (sharedMem.isAttached())
        sharedMem.detach();
    memLock.release();
}
