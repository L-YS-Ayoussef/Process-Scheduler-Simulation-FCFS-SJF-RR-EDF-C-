#include "io/InputParser.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <cctype>
#include <algorithm>

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

static std::string stripComment(const std::string &line)
{
    // remove everything after //
    std::size_t pos = line.find("//");
    return (pos == std::string::npos) ? line : line.substr(0, pos);
}

static bool isBlank(const std::string &s)
{
    for (char c : s)
        if (!std::isspace((unsigned char)c))
            return false;
    return true;
}

static bool parsePairToken(const std::string &tok, int &r, int &d)
{
    // tok = "(3,2)" possibly with spaces removed already
    if (tok.size() < 5)
        return false;
    if (tok.front() != '(' || tok.back() != ')')
        return false;
    std::string mid = tok.substr(1, tok.size() - 2);
    std::size_t comma = mid.find(',');
    if (comma == std::string::npos)
        return false;
    r = std::stoi(mid.substr(0, comma));
    d = std::stoi(mid.substr(comma + 1));
    return true;
}

static bool isIntToken(const std::string &tok)
{
    if (tok.empty())
        return false;
    std::size_t i = 0;
    if (tok[0] == '-')
    {
        if (tok.size() == 1)
            return false;
        i = 1;
    }
    for (; i < tok.size(); ++i)
        if (!std::isdigit((unsigned char)tok[i]))
            return false;
    return true;
}

bool InputParser::parseFile(const std::string &path, ParsedInput &out, std::string &err)
{
    err.clear();

    std::ifstream fin(path);
    if (!fin)
    {
        err = "Cannot open input file: " + path;
        return false;
    }

    out.NF = out.NS = out.NR = 0;
    out.timeSlice = 0;
    out.RTF = out.MaxW = out.STL = out.forkProb = 0;
    out.M = 0;

    while (out.newList.getHead())
    {
        Process *p = nullptr;
        out.newList.popFront(p);
    }
    while (out.killEvents.getHead())
    {
        KillEvent k;
        out.killEvents.popFront(k);
    }
    while (out.allProcesses.getHead())
    {
        Process *p = nullptr;
        out.allProcesses.popFront(p);
    }

    auto readNextDataLine = [&](std::string &lineOut) -> bool
    {
        std::string line;
        while (std::getline(fin, line))
        {
            line = stripComment(line);
            if (isBlank(line))
                continue;
            lineOut = line;
            return true;
        }
        return false;
    };

    std::string line;

    // ---- NF NS NR ----
    if (!readNextDataLine(line))
    {
        err = "Missing NF NS NR line";
        return false;
    }
    {
        std::stringstream ss(line);
        if (!(ss >> out.NF >> out.NS >> out.NR >> out.NE))
        {
            err = "Bad NF NS NR NE line: " + line;
            return false;
        }
    }

    // ---- RR time slice ----
    if (!readNextDataLine(line))
    {
        err = "Missing RR time slice line";
        return false;
    }
    {
        std::stringstream ss(line);
        if (!(ss >> out.timeSlice))
        {
            err = "Bad RR time slice line: " + line;
            return false;
        }
    }

    // ---- RTF MaxW STL ForkProb ----
    if (!readNextDataLine(line))
    {
        err = "Missing RTF MaxW STL ForkProb line";
        return false;
    }
    {
        std::stringstream ss(line);
        if (!(ss >> out.RTF >> out.MaxW >> out.STL >> out.forkProb))
        {
            err = "Bad RTF/MaxW/STL/ForkProb line: " + line;
            return false;
        }
    }

    // ---- M ----
    if (!readNextDataLine(line))
    {
        err = "Missing M line";
        return false;
    }
    {
        std::stringstream ss(line);
        if (!(ss >> out.M))
        {
            err = "Bad M line: " + line;
            return false;
        }
        if (out.M < 0)
        {
            err = "Bad M value (negative)";
            return false;
        }
    }

    // ---- Processes ----
    std::vector<Process *> procVec;
    procVec.reserve((size_t)out.M);

    int readCount = 0;
    while (readCount < out.M)
    {
        if (!readNextDataLine(line))
        {
            err = "Unexpected EOF while reading processes";
            return false;
        }

        std::stringstream ss(line);

        int AT = 0, PID = 0, CT = 0;
        if (!(ss >> AT >> PID >> CT))
        {
            err = "Bad process line (AT PID CT missing): " + line;
            return false;
        }

        std::vector<int> ints;
        std::vector<std::pair<int, int>> pairs;

        std::string tok;
        while (ss >> tok)
        {
            if (!tok.empty() && tok.front() == '(')
            {
                int r = 0, d = 0;
                if (!parsePairToken(tok, r, d))
                {
                    err = "Bad IO pair token: " + tok + " in line: " + line;
                    return false;
                }
                pairs.push_back({r, d});
            }
            else if (isIntToken(tok))
            {
                ints.push_back(std::stoi(tok));
            }
            else
            {
                err = "Unknown token: " + tok + " in line: " + line;
                return false;
            }
        }

        int DL = -1;
        int ioCount = 0;

        // Backward compatible:
        // old:  AT PID CT IOcount (...)
        // new:  AT PID CT DL IOcount (...)
        if (ints.size() == 1)
        {
            ioCount = ints[0];
        }
        else if (ints.size() == 2)
        {
            DL = ints[0];
            ioCount = ints[1];
        }
        else
        {
            err = "Bad process line: expected IOcount or DL IOcount: " + line;
            return false;
        }

        if ((int)pairs.size() != ioCount)
        {
            err = "IOcount mismatch in line: " + line;
            return false;
        }

        // ---- Build IO array (adjust struct name/fields if needed) ----
        IORequest *ioArr = nullptr;
        if (ioCount > 0)
        {
            ioArr = new IORequest[ioCount];
            for (int k = 0; k < ioCount; ++k)
            {
                ioArr[k].io_r = pairs[k].first;
                ioArr[k].io_d = pairs[k].second;
            }
        }

        Process *p = new Process(PID, AT, CT, ioCount, ioArr);

        // ✅ EDF: store absolute deadline if present
        if (DL >= 0)
            p->setDeadline(DL);

        out.allProcesses.pushBack(p);
        procVec.push_back(p);

        ++readCount;
    }

    // Ensure NEW list is sorted by AT (and PID tie-break)
    std::sort(procVec.begin(), procVec.end(),
              [](Process *a, Process *b)
              {
                  if (a->getAT() != b->getAT())
                      return a->getAT() < b->getAT();
                  return a->getPID() < b->getPID();
              });

    for (Process *p : procVec)
        out.newList.pushBack(p);

    // ---- Kill events until EOF ----
    std::vector<KillEvent> kills;
    while (readNextDataLine(line))
    {
        std::stringstream ss(line);
        KillEvent k;
        if (!(ss >> k.time >> k.pid))
        {
            err = "Bad kill event line: " + line;
            return false;
        }
        kills.push_back(k);
    }

    std::sort(kills.begin(), kills.end(),
              [](const KillEvent &a, const KillEvent &b)
              {
                  if (a.time != b.time)
                      return a.time < b.time;
                  return a.pid < b.pid;
              });

    for (auto &k : kills)
        out.killEvents.pushBack(k);

    return true;
}
