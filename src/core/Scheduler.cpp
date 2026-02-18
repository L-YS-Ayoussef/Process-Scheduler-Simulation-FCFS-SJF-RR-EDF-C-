#include "core/Scheduler.h"
#include "model/Process.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>
#include <climits>
#include <cstdlib>
#include <iomanip>
#include "processors/FCFSProcessor.h"
#include "processors/SJFProcessor.h"
#include "processors/RRProcessor.h"
#include "processors/EDFProcessor.h"

// ================= Scheduler =================
Scheduler::Scheduler()
    : totalProcs(0),
      processors(nullptr),
      ioDev(nullptr),
      ioRemaining(0),
      trmCount(0) {}

Scheduler::~Scheduler()
{
    if (processors)
    {
        for (int i = 0; i < totalProcs; ++i)
            delete processors[i];
        delete[] processors;
    }

    // free all processes
    auto *node = in.allProcesses.getHead();
    while (node)
    {
        delete node->data;
        node = node->next;
    }
}

void Scheduler::buildProcessors()
{
    totalProcs = in.NF + in.NS + in.NR + in.NE;
    processors = new Processor *[totalProcs];

    int idx = 0;

    for (int i = 0; i < in.NF; ++i)
        processors[idx++] = new FCFSProcessor(idx);

    for (int i = 0; i < in.NS; ++i)
        processors[idx++] = new SJFProcessor(idx);

    for (int i = 0; i < in.NR; ++i)
        processors[idx++] = new RRProcessor(idx);

    for (int i = 0; i < in.NE; ++i)
        processors[idx++] = new EDFProcessor(idx);

    // set RR time slice
    for (int i = 0; i < totalProcs; ++i)
        if (processors[i]->getType() == ProcType::RR)
            processors[i]->setTimeSlice(in.timeSlice);
}

bool Scheduler::load(const std::string &inputPath, std::string &err)
{
    if (!InputParser::parseFile(inputPath, in, err))
        return false;
    buildProcessors();

    // Milestone D init:
    killCur = in.killEvents.getHead();
    initNextPid();
    totalCreated = in.M;

    // reset counters
    migRTF = migMaxW = stealMoves = forkedCreated = killedCount = 0;
    trmCount = 0;
    ioDev = nullptr;
    ioRemaining = 0;

    return true;
}

void Scheduler::printLoadedSummary() const
{
    std::cout << "=== Input Loaded Successfully ===\n";
    std::cout << "Processors: NF=" << in.NF << " NS=" << in.NS << " NR=" << in.NR << " NE=" << in.NE
              << "  (Total=" << (in.NF + in.NS + in.NR + in.NE) << ")\n";
    std::cout << "RR TimeSlice=" << in.timeSlice << "\n";
    std::cout << "RTF=" << in.RTF << " MaxW=" << in.MaxW << " STL=" << in.STL
              << " ForkProb=" << in.forkProb << "%\n";
    std::cout << "Processes (M)=" << in.M << "\n";

    std::cout << "First processes in NEW:\n";
    int shown = 0;
    auto *node = in.newList.getHead();
    while (node && shown < 5)
    {
        Process *p = node->data;
        std::cout << "  PID=" << p->getPID()
                  << " AT=" << p->getAT()
                  << " CT=" << p->getCT()
                  << " IOcnt=" << p->getIOCount() << "\n";
        node = node->next;
        ++shown;
    }

    int kills = 0;
    auto *kn = in.killEvents.getHead();
    while (kn)
    {
        ++kills;
        kn = kn->next;
    }
    std::cout << "SIGKILL events=" << kills << "\n";
    std::cout << "===============================\n";
}

int Scheduler::pickBestProcessorIndex() const
{
    int best = 0;
    long long bestVal = processors[0]->expectedFinishTime();
    for (int i = 1; i < totalProcs; ++i)
    {
        long long v = processors[i]->expectedFinishTime();
        if (v < bestVal)
        {
            bestVal = v;
            best = i;
        }
    }
    return best;
}

// ------------------ UI helpers ------------------
void Scheduler::waitMode(UIMode mode) const
{
    if (mode == UIMode::Silent)
        return;
    if (mode == UIMode::Interactive)
    {
        std::cout << "Press Enter to continue...";
        std::cin.get();
        return;
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void Scheduler::printSnapshot(int t) const
{
    std::cout << "\n================= Timestep " << t << " =================\n";

    // NEW list (not just count)
    std::cout << "NEW: ";
    Node<Process *> *n = in.newList.getHead();
    if (!n)
        std::cout << "EMPTY";
    while (n)
    {
        std::cout << n->data->getPID() << "(AT=" << n->data->getAT() << ")";
        if (n->next)
            std::cout << ", ";
        n = n->next;
    }
    std::cout << "\n";

    // IO device
    std::cout << "I/O device: ";
    if (ioDev)
        std::cout << "PID=" << ioDev->getPID() << " (remainingIO=" << ioRemaining << ")";
    else
        std::cout << "IDLE";
    std::cout << "\n";

    // BLK waiting queue
    std::cout << "BLK(wait): ";
    Node<Process *> *b = blkWait.getHead();
    if (!b)
        std::cout << "EMPTY";
    while (b)
    {
        std::cout << b->data->getPID() << "(IO=" << b->data->getPendingIO() << ")";
        if (b->next)
            std::cout << ", ";
        b = b->next;
    }
    std::cout << "\n";

    // TRM list
    std::cout << "TRM: ";
    Node<Process *> *tr = trm.getHead();
    if (!tr)
        std::cout << "EMPTY";
    while (tr)
    {
        std::cout << tr->data->getPID();
        if (tr->data->getTT() >= 0)
            std::cout << "(TT=" << tr->data->getTT() << ")";
        if (tr->next)
            std::cout << ", ";
        tr = tr->next;
    }
    std::cout << "\n";

    // Processor view
    std::cout << "------------------ Processors ------------------\n";
    for (int i = 0; i < totalProcs; ++i)
    {
        const char *typeStr =
            (processors[i]->getType() == ProcType::FCFS) ? "FCFS" : (processors[i]->getType() == ProcType::SJF) ? "SJF"
                                                                                                                : (processors[i]->getType() == ProcType::EDF) ? "EDF" : "RR";

        std::cout << "P" << processors[i]->getID() << " [" << typeStr << "]\n";

        std::cout << "  RDY: ";
        processors[i]->printReady(std::cout);
        if (processors[i]->readyCount() == 0)
            std::cout << "EMPTY";
        std::cout << "\n";

        std::cout << "  RUN: ";
        Process *run = processors[i]->getRunning();
        if (!run)
        {
            std::cout << "IDLE\n";
        }
        else
        {
            std::cout << "PID=" << run->getPID()
                      << " rem=" << run->getRemaining()
                      << " exec=" << run->getExecuted();

            if (processors[i]->getType() == ProcType::RR)
            {
                std::cout << " q=" << processors[i]->getQuantumCounter()
                          << "/" << processors[i]->getTimeSlice();
            }
            std::cout << "\n";
        }

        std::cout << "  CPU stats: busy=" << processors[i]->getBusy()
                  << " idle=" << processors[i]->getIdle() << "\n";
    }
    std::cout << "------------------------------------------------\n";
}

// ------------------ Phase2 core steps ------------------
void Scheduler::dispatchIdleCPUs(int t)
{
    for (int i = 0; i < totalProcs; ++i)
    {
        Processor *cpu = processors[i];

        if (!cpu->isIdle())
            continue;

        // Keep trying until we either run something or RDY becomes empty
        while (cpu->isIdle())
        {
            Process *cand = cpu->popReady();
            if (!cand)
                break;

            // Migration check BEFORE RUN
            if (tryMigrateOnDispatch(cpu, cand, t))
            {
                // migrated somewhere else, try to get another cand for this CPU
                continue;
            }

            // Normal dispatch
            cand->setState(ProcState::RUN);
            cand->markFirstRunIfNeeded(t);
            cpu->setRunning(cand);
            cpu->resetQuantum(); // RR only (safe for all)
            break;
        }
    }
}

void Scheduler::executeOneTick()
{
    // CPU tick
    for (int i = 0; i < totalProcs; ++i)
    {
        Process *run = processors[i]->getRunning();
        if (run)
        {
            run->cpuTick();
            processors[i]->addBusy();
            if (processors[i]->getType() == ProcType::RR)
                processors[i]->incQuantum();
        }
        else
        {
            processors[i]->addIdle();
        }
    }

    // I/O device tick
    if (ioDev)
    {
        --ioRemaining;
    }
}

void Scheduler::postCpuTransitions(int t)
{
    for (int i = 0; i < totalProcs; ++i)
    {
        Process *run = processors[i]->getRunning();
        if (!run)
            continue;

        // finished
        if (run->isFinished())
        {
            processors[i]->clearRunning();
            processors[i]->resetQuantum();

            // use unified termination (handles orphans + counters)
            terminateProcess(run, t + 1, TermReason::NORMAL);

            continue;
        }

        // I/O due
        if (run->ioDueNow())
        {
            run->moveDueIOToPending();
            run->setState(ProcState::BLK);
            blkWait.enqueue(run);
            processors[i]->clearRunning();
            processors[i]->resetQuantum();
            continue;
        }

        // RR quantum expired => preempt
        if (processors[i]->getType() == ProcType::RR && processors[i]->quantumExpired())
        {
            run->setState(ProcState::RDY);
            processors[i]->enqueue(run); // back to same RR ready queue
            processors[i]->clearRunning();
            processors[i]->resetQuantum();
        }
    }
}

void Scheduler::finishIOIfDone(int t)
{
    if (!ioDev)
        return;
    if (ioRemaining > 0)
        return;

    Process *done = ioDev;
    ioDev = nullptr;

    done->setState(ProcState::RDY);
    int idx = pickBestProcessorIndex();
    processors[idx]->enqueue(done);

    // EDF preemption check (if it went to EDF)
    if (processors[idx]->getType() == ProcType::EDF)
    {
        edfPreemptIfNeeded(processors[idx], t);
    }
}

void Scheduler::startIOIfPossible()
{
    if (ioDev)
        return;
    if (blkWait.empty())
        return;

    Process *p = nullptr;
    if (!blkWait.dequeue(p))
        return;

    ioDev = p;
    ioRemaining = ioDev->takePendingIO(); // duration from last due request
    if (ioRemaining <= 0)
    {
        // safety: if something wrong, send it back RDY
        ioDev->setState(ProcState::RDY);
        int idx = pickBestProcessorIndex();
        processors[idx]->enqueue(ioDev);
        ioDev = nullptr;
        ioRemaining = 0;
    }
}

void Scheduler::admitArrivals(int t)
{
    while (true)
    {
        auto *head = in.newList.getHead();
        if (!head)
            break;

        Process *p = head->data;
        if (p->getAT() != t)
            break;

        Process *moved = nullptr;
        in.newList.popFront(moved);

        moved->setState(ProcState::RDY);

        int idx = pickBestProcessorIndex();
        processors[idx]->enqueue(moved);

        // EDF preemption check (if this target processor is EDF)
        if (processors[idx]->getType() == ProcType::EDF)
        {
            edfPreemptIfNeeded(processors[idx], t);
        }
    }
}

// ================= Simulation =================
void Scheduler::simulate(UIMode mode)
{
    int t = 0;
    const int MAX_T = 200000;

    while (trmCount < totalCreated && t < MAX_T)
    {

        // 1) arrivals
        admitArrivals(t);

        // 2) SIGKILL at time t
        applySigKill(t);

        // 3) work stealing
        workStealIfNeeded(t);

        // 4) dispatch (includes migration checks)
        dispatchIdleCPUs(t);

        // 5) fork (FCFS RUN only)
        attemptForking(t);

        // 6) execute 1 tick (CPU + IO device)
        executeOneTick();

        // 7) transitions (finish / IO due / RR preempt)
        postCpuTransitions(t);

        // 8) IO finish/start
        finishIOIfDone(t);
        startIOIfPossible();

        // 9) print
        if (mode != UIMode::Silent)
            printSnapshot(t);
        waitMode(mode);

        ++t;
    }

    // ALWAYS write output in final project
    writeOutputFile("data/output.txt");
}

void Scheduler::terminateProcess(Process *p, int tt, TermReason why)
{
    if (!p)
        return;

    // set termination
    p->setState(ProcState::TRM);
    p->setTT(tt);

    // if killed before first run -> set RT consistently
    p->markFirstRunIfNeeded(tt);

    // counts
    ++trmCount;
    if (why == TermReason::SIGKILL || why == TermReason::ORPHAN)
    {
        ++killedCount;
    }

    // add to TRM list
    trm.pushBack(p);

    // kill descendants immediately (orphans)
    LinkedList<Process *> &kids = p->getChildren();
    Node<Process *> *c = kids.getHead();
    while (c)
    {
        Process *child = c->data;
        // child is forked => guaranteed to be in FCFS RUN/RDY (no IO, no migration/steal)
        killByPIDinFCFS(child->getPID(), tt, TermReason::ORPHAN);
        c = c->next;
    }
}

void Scheduler::initNextPid()
{
    int mx = 0;
    Node<Process *> *n = in.allProcesses.getHead();
    while (n)
    {
        int pid = n->data->getPID();
        if (pid > mx)
            mx = pid;
        n = n->next;
    }
    nextPid = mx + 1;
}

bool Scheduler::killByPIDinFCFS(int pid, int tt, TermReason why)
{
    for (int i = 0; i < totalProcs; ++i)
    {
        Processor *cpu = processors[i];
        if (cpu->getType() != ProcType::FCFS)
            continue;

        FCFSProcessor *fcfs = static_cast<FCFSProcessor *>(cpu);

        // 1) RUN?
        Process *run = fcfs->getRunning();
        if (run && run->getPID() == pid)
        {
            fcfs->clearRunning();
            terminateProcess(run, tt, why);
            return true;
        }

        // 2) RDY?
        Process *removed = nullptr;
        if (fcfs->removeReadyByPID(pid, removed))
        {
            terminateProcess(removed, tt, why);
            return true;
        }
    }
    return false;
}

void Scheduler::applySigKill(int t)
{
    while (killCur && killCur->data.time == t)
    {
        int pid = killCur->data.pid;
        killByPIDinFCFS(pid, t, TermReason::SIGKILL);
        killCur = killCur->next;
    }
}

void Scheduler::attemptForking(int t)
{
    if (in.forkProb <= 0)
        return;

    for (int i = 0; i < totalProcs; ++i)
    {
        Processor *cpu = processors[i];
        if (cpu->getType() != ProcType::FCFS)
            continue;

        Process *parent = cpu->getRunning();
        if (!parent)
            continue;

        // optional: do not allow forked children to fork
        if (parent->isForkedChild())
            continue;

        if (parent->hasForkedOnce())
            continue;

        int r = (std::rand() % 100) + 1; // 1..100
        if (r > in.forkProb)
            continue;

        // child: AT=t, CTchild = remaining of parent, no IO at all
        Process *child = new Process(nextPid++, t, parent->getRemaining(), 0, nullptr);
        child->setState(ProcState::RDY);
        child->setForkedChild(true);
        child->setParent(parent);
        parent->addChild(child);
        parent->markForkedOnce();

        // ownership for cleanup
        in.allProcesses.pushBack(child);

        ++forkedCreated;
        ++totalCreated;

        // enqueue to shortest FCFS processor
        int idx = pickShortestFCFS();
        if (idx < 0)
            idx = i; // fallback (should not happen if FCFS exists)
        processors[idx]->enqueue(child);
    }
}

bool Scheduler::tryMigrateOnDispatch(Processor *from, Process *p, int t)
{
    if (!from || !p)
        return false;

    // forked processes: no migration
    if (p->isForkedChild())
        return false;

    // RR -> SJF if rem < RTF
    if (from->getType() == ProcType::RR)
    {
        if (p->getRemaining() < in.RTF)
        {
            int sjfIdx = pickShortestByType(ProcType::SJF);
            if (sjfIdx >= 0)
            {
                p->setState(ProcState::RDY);
                processors[sjfIdx]->enqueue(p);
                ++migRTF;
                return true;
            }
        }
    }

    // FCFS -> RR if waitingSoFar > MaxW
    if (from->getType() == ProcType::FCFS)
    {
        int waitingSoFar = (t - p->getAT()) - p->getExecuted();
        if (waitingSoFar > in.MaxW)
        {
            int rrIdx = pickShortestByType(ProcType::RR);
            if (rrIdx >= 0)
            {
                p->setState(ProcState::RDY);
                processors[rrIdx]->enqueue(p);
                ++migMaxW;
                return true;
            }
        }
    }

    return false;
}

void Scheduler::workStealIfNeeded(int t)
{
    if (in.STL <= 0)
        return;
    if (t == 0)
        return;
    if (t % in.STL != 0)
        return;

    while (true)
    {
        int longIdx = findLongestByEFT();
        int shortIdx = findShortestByEFT();

        if (longIdx < 0 || shortIdx < 0)
            return;
        if (longIdx == shortIdx)
            return;

        long long LQF = processors[longIdx]->expectedFinishTime();
        long long SQF = processors[shortIdx]->expectedFinishTime();
        if (LQF <= 0)
            return;

        double stealLimit = (double)(LQF - SQF) * 100.0 / (double)LQF;
        if (stealLimit <= 40.0)
            return;

        // must steal TOP of longest ready queue
        Process *top = processors[longIdx]->peekReady();
        if (!top)
            return;

        // forked processes: cannot be stolen
        if (top->isForkedChild())
            return;

        Process *stolen = processors[longIdx]->popReady();
        if (!stolen)
            return;

        stolen->setState(ProcState::RDY);
        processors[shortIdx]->enqueue(stolen);
        ++stealMoves;
    }
}

int Scheduler::pickShortestByType(ProcType tp) const
{
    int best = -1;
    long long bestVal = LLONG_MAX;

    for (int i = 0; i < totalProcs; ++i)
    {
        if (processors[i]->getType() != tp)
            continue;
        long long v = processors[i]->expectedFinishTime();
        if (v < bestVal)
        {
            bestVal = v;
            best = i;
        }
    }
    return best;
}

int Scheduler::pickShortestFCFS() const
{
    return pickShortestByType(ProcType::FCFS);
}

int Scheduler::findLongestByEFT() const
{
    int best = -1;
    long long bestVal = -1;

    for (int i = 0; i < totalProcs; ++i)
    {
        if (processors[i]->readyCount() == 0)
            continue; // must steal from RDY
        long long v = processors[i]->expectedFinishTime();
        if (v > bestVal)
        {
            bestVal = v;
            best = i;
        }
    }
    return best;
}

int Scheduler::findShortestByEFT() const
{
    int best = -1;
    long long bestVal = LLONG_MAX;

    for (int i = 0; i < totalProcs; ++i)
    {
        long long v = processors[i]->expectedFinishTime();
        if (v < bestVal)
        {
            bestVal = v;
            best = i;
        }
    }
    return best;
}

#include <fstream>
#include <iomanip>

void Scheduler::writeOutputFile(const std::string &path) const
{
    std::ofstream out(path);
    if (!out)
        return;

    // add DL column
    out << "TT PID AT CT DL IO_D WT RT TRT\n";

    long long sumWT = 0, sumRT = 0, sumTRT = 0;
    int count = 0;

    int completedWithDL = 0;
    int metDL = 0;

    Node<Process *> *n = trm.getHead();
    while (n)
    {
        Process *p = n->data;

        int TT = p->getTT();
        int AT = p->getAT();
        int CT = p->getCT();
        int DL = p->hasDeadline() ? p->getDeadline() : -1;
        int IO_D = p->getTotalIODur();

        int TRT = TT - AT;
        int WT = TRT - CT; // if you want: WT = TRT - CT - IO_D (depending on your rubric)
        int RT = p->hasFirstRun() ? (p->getFirstRunTime() - AT) : 0;

        out << TT << " " << p->getPID() << " " << AT << " " << CT << " "
            << DL << " " << IO_D << " " << WT << " " << RT << " " << TRT << "\n";

        sumWT += WT;
        sumRT += RT;
        sumTRT += TRT;
        ++count;

        // deadline metric: only for completed processes with deadlines
        if (p->isFinished() && p->hasDeadline())
        {
            ++completedWithDL;
            if (TT <= DL)
                ++metDL;
        }

        n = n->next;
    }

    out << "\n--- Summary ---\n";
    out << "Total Processes: " << count << "\n";
    out << "Forked Created: " << forkedCreated << "\n";
    out << "Killed (SIGKILL+ORPHAN): " << killedCount << "\n";
    out << "Migration RTF (RR->SJF): " << migRTF << "\n";
    out << "Migration MaxW (FCFS->RR): " << migMaxW << "\n";
    out << "Steal Moves: " << stealMoves << "\n";

    if (count > 0)
    {
        out << "Avg WT: " << (double)sumWT / count << "\n";
        out << "Avg RT: " << (double)sumRT / count << "\n";
        out << "Avg TRT: " << (double)sumTRT / count << "\n";
    }

    // Deadline metric
    if (completedWithDL > 0)
    {
        double pct = 100.0 * metDL / completedWithDL;
        out << "Completed before deadline: " << pct
            << "% (" << metDL << "/" << completedWithDL << ")\n";
    }
    else
    {
        out << "Completed before deadline: N/A (no deadlines)\n";
    }

    out << "\n--- Processor Stats ---\n";
    for (int i = 0; i < totalProcs; ++i)
    {
        long long busy = processors[i]->getBusy();
        long long idle = processors[i]->getIdle();
        long long total = busy + idle;
        double util = (total > 0) ? (100.0 * busy / total) : 0.0;

        const char *typeStr =
            (processors[i]->getType() == ProcType::FCFS) ? "FCFS" : (processors[i]->getType() == ProcType::SJF) ? "SJF"
                                                                : (processors[i]->getType() == ProcType::RR)    ? "RR"
                                                                                                                : "EDF";

        out << "P" << processors[i]->getID() << " [" << typeStr << "] "
            << "busy=" << busy << " idle=" << idle << " util%=" << util << "\n";
    }
}

void Scheduler::edfPreemptIfNeeded(Processor *cpu, int t)
{
    if (!cpu)
        return;
    if (cpu->getType() != ProcType::EDF)
        return;

    Process *run = cpu->getRunning();
    Process *top = cpu->peekReady();
    if (!run || !top)
        return;

    int dr = run->hasDeadline() ? run->getDeadline() : INT_MAX;
    int dt = top->hasDeadline() ? top->getDeadline() : INT_MAX;

    if (dt < dr)
    {
        // preempt running
        run->setState(ProcState::RDY);
        cpu->enqueue(run);
        cpu->clearRunning();
        cpu->resetQuantum();

        Process *next = cpu->popReady();
        if (next)
        {
            next->setState(ProcState::RUN);
            next->markFirstRunIfNeeded(t);
            cpu->setRunning(next);
        }
    }
}
