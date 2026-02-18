#include "model/Process.h"

Process::Process(int PID, int AT, int CT, int ioCnt, IORequest *ioArr)
    : pid(PID), at(AT), ct(CT),
      remaining(CT), executed(0),
      ioCount(ioCnt), io(ioArr),
      nextIOIdx(0), pendingIODur(0), totalIODur(0),
      state(ProcState::NEW),
      firstRunSet(false), firstRunTime(-1),
      tt(-1)
{
    for (int i = 0; i < ioCount; ++i)
        totalIODur += io[i].io_d;
}

Process::~Process()
{
    delete[] io;
}

void Process::markFirstRunIfNeeded(int t)
{
    if (!firstRunSet)
    {
        firstRunSet = true;
        firstRunTime = t;
    }
}

void Process::cpuTick()
{
    if (remaining > 0)
    {
        --remaining;
        ++executed;
    }
}

bool Process::ioDueNow() const
{
    if (nextIOIdx >= ioCount)
        return false;
    return executed == io[nextIOIdx].io_r;
}

void Process::moveDueIOToPending()
{
    if (!ioDueNow())
        return;
    pendingIODur = io[nextIOIdx].io_d;
    ++nextIOIdx;
}

int Process::takePendingIO()
{
    int d = pendingIODur;
    pendingIODur = 0;
    return d;
}
