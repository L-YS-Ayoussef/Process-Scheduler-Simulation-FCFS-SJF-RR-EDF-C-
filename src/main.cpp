#include <iostream>
#include <string>
#include "core/Scheduler.h"

static UIMode parseMode(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i)
    {
        std::string a = argv[i];
        if (a == "--mode=interactive")
            return UIMode::Interactive;
        if (a == "--mode=step")
            return UIMode::Step;
        if (a == "--mode=silent")
            return UIMode::Silent;
    }
    return UIMode::Interactive;
}

static bool hasFlag(int argc, char **argv, const char *flag)
{
    for (int i = 1; i < argc; ++i)
        if (std::string(argv[i]) == flag)
            return true;
    return false;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cout << "Usage: ProcessScheduler <input_file> --mode=interactive|step|silent\n";
        return 1;
    }

    Scheduler s;
    std::string err;
    if (!s.load(argv[1], err))
    {
        std::cout << "Load failed: " << err << "\n";
        return 1;
    }

    s.printLoadedSummary();

    UIMode mode = UIMode::Interactive;
    for (int i = 1; i < argc; ++i)
    {
        std::string a = argv[i];
        if (a == "--mode=step")
            mode = UIMode::Step;
        else if (a == "--mode=silent")
            mode = UIMode::Silent;
        else if (a == "--mode=interactive")
            mode = UIMode::Interactive;
    }

    s.simulate(mode);
    return 0;
}
