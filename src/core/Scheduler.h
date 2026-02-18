#pragma once
#include <string>
#include <ostream>
#include "io/InputParser.h"
#include "processors/Processor.h"
#include "processors/FCFSProcessor.h"
#include "ds/Queue.h"
#include "ds/LinkedList.h"

enum class UIMode
{
    Interactive,
    Step,
    Silent
};
enum class TermReason
{
    NORMAL,
    SIGKILL,
    ORPHAN
};

class Scheduler
{
private:
    ParsedInput in;

    int totalProcs;
    Processor **processors;

    // BLK waiting queue + single IO device
    Queue<Process *> blkWait;
    Process *ioDev;
    int ioRemaining;

    // terminated list
    LinkedList<Process *> trm;
    int trmCount;

    // ===== Milestone D additions =====
    int nextPid;      // next PID for forked children
    int totalCreated; // total processes including forked children (stop condition)

    int migRTF;
    int migMaxW;
    int stealMoves;
    int forkedCreated;
    int killedCount;

    Node<KillEvent> *killCur; // pointer iterator over kill events list

    // ===== existing helpers =====
    void buildProcessors();
    int pickBestProcessorIndex() const;

    void waitMode(UIMode mode) const;
    void printSnapshot(int t) const;

    // ===== Phase2 core steps you already have =====
    void admitArrivals(int t);
    void dispatchIdleCPUs(int t); // UPDATED in Milestone D
    void executeOneTick();
    void postCpuTransitions(int t);
    void finishIOIfDone();
    void startIOIfPossible();

    // ===== Milestone D helpers (prototypes requested) =====
    void initNextPid(); // FIX: now Scheduler has it

    void terminateProcess(Process *p, int tt, TermReason why);

    bool killByPIDinFCFS(int pid, int tt, TermReason why);
    void applySigKill(int t);

    void attemptForking(int t);

    bool tryMigrateOnDispatch(Processor *from, Process *p, int t);

    void workStealIfNeeded(int t);

    // helper selection
    int pickShortestByType(ProcType tp) const;
    int pickShortestFCFS() const;
    int findLongestByEFT() const;
    int findShortestByEFT() const;

    // output
    void writeOutputFile(const std::string &path) const;

public:
    Scheduler();
    ~Scheduler();

    bool load(const std::string &inputPath, std::string &err);

    void printLoadedSummary() const;

    // FINAL function name (no phases)
    void simulate(UIMode mode);
};
