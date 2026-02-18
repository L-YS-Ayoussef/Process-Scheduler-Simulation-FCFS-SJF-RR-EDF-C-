#pragma once
#include <cstddef>
#include <ostream>

class Process;

enum class ProcType
{
    FCFS,
    SJF,
    RR,
    EDF
};

class Processor
{
protected:
    int id;
    ProcType type;

    long long readyWork; // sum of remaining in RDY
    Process *running;

    long long busyTime;
    long long idleTime;

    // RR only (others keep 0)
    int timeSlice;
    int quantumCounter;

public:
    Processor(int ID, ProcType t)
        : id(ID), type(t),
          readyWork(0), running(nullptr),
          busyTime(0), idleTime(0),
          timeSlice(0), quantumCounter(0) {}

    virtual ~Processor() = default;

    Processor(const Processor &) = delete;
    Processor &operator=(const Processor &) = delete;

    int getID() const { return id; }
    ProcType getType() const { return type; }

    int getQuantumCounter() const { return quantumCounter; }

    bool isIdle() const { return running == nullptr; }
    Process *getRunning() const { return running; }

    void setRunning(Process *p) { running = p; }
    void clearRunning() { running = nullptr; }

    long long expectedFinishTime() const;

    // stats
    void addBusy() { ++busyTime; }
    void addIdle() { ++idleTime; }
    long long getBusy() const { return busyTime; }
    long long getIdle() const { return idleTime; }

    // RR controls
    void setTimeSlice(int ts) { timeSlice = ts; }
    int getTimeSlice() const { return timeSlice; }
    void resetQuantum() { quantumCounter = 0; }
    void incQuantum() { ++quantumCounter; }
    bool quantumExpired() const { return (timeSlice > 0 && quantumCounter >= timeSlice); }

    virtual void enqueue(Process *p) = 0;
    virtual Process *popReady() = 0;
    virtual std::size_t readyCount() const = 0;
    virtual void printReady(std::ostream &os) const = 0;
    virtual Process *peekReady() const = 0;
};
