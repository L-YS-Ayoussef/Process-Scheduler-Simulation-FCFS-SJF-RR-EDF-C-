#include "processors/Processor.h"
#include "model/Process.h"

long long Processor::expectedFinishTime() const
{
    long long runRem = 0;
    if (running)
        runRem = running->getRemaining();
    return readyWork + runRem;
}
