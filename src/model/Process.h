#pragma once
#include "IORequest.h"
#include "ds/LinkedList.h"

enum class ProcState
{
    NEW,
    RDY,
    RUN,
    BLK,
    TRM
};

class Process
{
private:
    int pid;
    int at;
    int ct;

    int remaining;
    int executed; // CPU time executed so far

    int ioCount;
    IORequest *io; // dynamic array (no STL)

    int nextIOIdx;    // next IO request index
    int pendingIODur; // IO duration waiting to be served by I/O device
    int totalIODur;   // sum of all IO durations (for later output)

    ProcState state;

    bool firstRunSet;
    int firstRunTime;
    int tt; // termination time

    Process *parent = nullptr;
    LinkedList<Process *> children; // store pointers only (do NOT delete children here)

    bool forkedChild = false; // true if created by fork
    bool forkedOnce = false;  // each process can fork at most once

public:
    Process(int PID, int AT, int CT, int ioCnt, IORequest *ioArr);
    ~Process();

    Process(const Process &) = delete;
    Process &operator=(const Process &) = delete;

    // getters
    int getPID() const { return pid; }
    int getAT() const { return at; }
    int getCT() const { return ct; }
    int getRemaining() const { return remaining; }
    int getExecuted() const { return executed; }

    int getIOCount() const { return ioCount; }
    const IORequest *getIOArr() const { return io; }
    int getTotalIODur() const { return totalIODur; }

    ProcState getState() const { return state; }
    void setState(ProcState s) { state = s; }

    int getPendingIO() const { return pendingIODur; }
    int getNextIOIndex() const { return nextIOIdx; }

    // timing
    void markFirstRunIfNeeded(int t);
    bool hasFirstRun() const { return firstRunSet; }
    int getFirstRunTime() const { return firstRunTime; }

    void setTT(int t) { tt = t; }
    int getTT() const { return tt; }

    // CPU execution: one tick
    void cpuTick();

    // IO logic: after cpuTick, check if IO is due now
    bool ioDueNow() const;
    void moveDueIOToPending(); // sets pendingIODur + advances nextIOIdx
    int takePendingIO();       // returns pending dur and clears it

    bool isFinished() const { return remaining <= 0; }

    static const char *stateName(ProcState s)
    {
        switch (s)
        {
        case ProcState::NEW:
            return "NEW";
        case ProcState::RDY:
            return "RDY";
        case ProcState::RUN:
            return "RUN";
        case ProcState::BLK:
            return "BLK";
        case ProcState::TRM:
            return "TRM";
        }
        return "?";
    }

    bool isForkedChild() const { return forkedChild; }
    void setForkedChild(bool v) { forkedChild = v; }

    bool hasForkedOnce() const { return forkedOnce; }
    void markForkedOnce() { forkedOnce = true; }

    Process *getParent() const { return parent; }
    void setParent(Process *p) { parent = p; }

    void addChild(Process *c) { children.pushBack(c); }
    LinkedList<Process *> &getChildren() { return children; }
};
