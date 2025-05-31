#pragma once

#include "config.h"

#include <QObject>
#include <QSharedMemory>
#include <QString>
#include <QSystemSemaphore>

namespace passcave {
	QString const RunGuardHash = "c30dd25fe847673aeca9a1d965d2f44e1c6511b9414f21077-"
			+ QString::number(passcave_VERSION_MAJOR) + "." + QString::number(passcave_VERSION_MINOR);
}

class RunGuard {
public:
    RunGuard(const QString& key);
    ~RunGuard();

    bool isAlreadyRunning();
    bool tryToRun();
    void release();

private:
    const QString key;
    const QString memLockKey;
    const QString sharedmemKey;

    QSharedMemory sharedMem;
    QSystemSemaphore memLock;

    Q_DISABLE_COPY(RunGuard)
};
