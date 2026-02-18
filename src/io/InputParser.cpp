#include "io/InputParser.h"
#include <fstream>
#include <sstream>
#include <cctype>

static std::string stripComments(const std::string &line)
{
    std::size_t pos = line.find("//");
    if (pos == std::string::npos)
        return line;
    return line.substr(0, pos);
}

static bool isWhitespaceLine(const std::string &s)
{
    for (char c : s)
        if (!std::isspace(static_cast<unsigned char>(c)))
            return false;
    return true;
}

// Parses IO pairs like: (1,20),(84,17)
static IORequest *parseIOPairs(const std::string &rest, int ioCnt, bool &ok)
{
    ok = true;
    if (ioCnt <= 0)
        return nullptr;

    IORequest *arr = new IORequest[ioCnt];
    int filled = 0;

    int i = 0;
    while (rest[i] && filled < ioCnt)
    {
        while (rest[i] && rest[i] != '(')
            ++i;
        if (!rest[i])
            break;
        ++i; // skip '('

        // parse number
        int a = 0, b = 0;
        bool neg = false;

        while (rest[i] && std::isspace((unsigned char)rest[i]))
            ++i;
        if (rest[i] == '-')
        {
            neg = true;
            ++i;
        }
        while (rest[i] && std::isdigit((unsigned char)rest[i]))
        {
            a = a * 10 + (rest[i] - '0');
            ++i;
        }
        if (neg)
            a = -a;

        while (rest[i] && (rest[i] == ',' || std::isspace((unsigned char)rest[i])))
            ++i;

        neg = false;
        if (rest[i] == '-')
        {
            neg = true;
            ++i;
        }
        while (rest[i] && std::isdigit((unsigned char)rest[i]))
        {
            b = b * 10 + (rest[i] - '0');
            ++i;
        }
        if (neg)
            b = -b;

        while (rest[i] && rest[i] != ')')
            ++i;
        if (rest[i] == ')')
            ++i;

        arr[filled++] = IORequest{a, b};
    }

    if (filled != ioCnt)
    {
        ok = false;
        delete[] arr;
        return nullptr;
    }
    return arr;
}

bool InputParser::parseFile(const std::string &path, ParsedInput &out, std::string &err)
{
    std::ifstream fin(path);
    if (!fin)
    {
        err = "Cannot open input file: " + path;
        return false;
    }

    std::string line;

    // read first non-empty line
    auto readLine = [&](std::string &dst) -> bool
    {
        while (std::getline(fin, dst))
        {
            dst = stripComments(dst);
            if (!isWhitespaceLine(dst))
                return true;
        }
        return false;
    };

    if (!readLine(line))
    {
        err = "Empty file";
        return false;
    }
    {
        std::stringstream ss(line);
        ss >> out.NF >> out.NS >> out.NR;
    }

    if (!readLine(line))
    {
        err = "Missing RR time slice line";
        return false;
    }
    {
        std::stringstream ss(line);
        ss >> out.timeSlice;
    }

    if (!readLine(line))
    {
        err = "Missing RTF/MaxW/STL/forkProb line";
        return false;
    }
    {
        std::stringstream ss(line);
        ss >> out.RTF >> out.MaxW >> out.STL >> out.forkProb;
    }

    if (!readLine(line))
    {
        err = "Missing M line";
        return false;
    }
    {
        std::stringstream ss(line);
        ss >> out.M;
    }

    // Read M process lines
    for (int k = 0; k < out.M; ++k)
    {
        if (!readLine(line))
        {
            err = "Missing process line #" + std::to_string(k + 1);
            return false;
        }

        // We need to parse first 4 fields then the rest as IO pairs
        std::stringstream ss(line);

        double atD = 0;
        int pid = 0, ct = 0, ioCnt = 0;
        ss >> atD >> pid >> ct >> ioCnt;
        int at = static_cast<int>(atD); // simulation is timestep-based

        std::string rest;
        std::getline(ss, rest); // remaining text for IO pairs

        bool ok = true;
        IORequest *ioArr = parseIOPairs(rest, ioCnt, ok);
        if (!ok)
        {
            err = "Failed to parse IO pairs for PID=" + std::to_string(pid);
            return false;
        }

        Process *p = new Process(pid, at, ct, ioCnt, ioArr);
        out.newList.pushBack(p);
        out.allProcesses.pushBack(p);
    }

    // Remaining lines: kill events (time pid)
    while (readLine(line))
    {
        std::stringstream ss(line);
        KillEvent ke{};
        if (!(ss >> ke.time >> ke.pid))
            continue;
        out.killEvents.pushBack(ke);
    }

    return true;
}
