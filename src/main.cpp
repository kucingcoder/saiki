#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>
#include <fstream>
#include <algorithm>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
using namespace std;

bool full = 0;

// exit if CTRL + C with success status
void handleSIGINT(int signal)
{
    exit(0);
}

// convert bytes to readable size
string size_computer(long size_bytes)
{
    const long TB = 1024UL * 1024 * 1024 * 1024;
    const long GB = 1024UL * 1024 * 1024;
    const long MB = 1024UL * 1024;
    const long KB = 1024UL;

    double numbers;
    string types, easy_number;

    if (size_bytes >= TB)
    {
        numbers = (double)size_bytes / TB;
        types = "TB";
    }
    else if (size_bytes >= GB)
    {
        numbers = (double)size_bytes / GB;
        types = "GB";
    }
    else if (size_bytes >= MB)
    {
        numbers = (double)size_bytes / MB;
        types = "MB";
    }
    else if (size_bytes >= KB)
    {
        numbers = (double)size_bytes / KB;
        types = "KB";
    }
    else
    {
        numbers = (double)size_bytes;
        types = "Bytes";
    }

    if ((int)numbers == numbers)
    {
        easy_number = to_string((int)numbers);
    }
    else
    {
        easy_number = to_string(numbers);
    }

    return easy_number + " " + types;
}

void stats()
{
    // retrieve username
    const char *username = getenv("USER");

    // retrieving OS information
    ifstream file_os("/etc/os-release");
    string line_os, os;

    while (getline(file_os, line_os))
    {
        if (line_os.starts_with("PRETTY_NAME="))
        {
            os = line_os.substr(12);
            if (!os.empty() && os.front() == '"' && os.back() == '"')
            {
                os = os.substr(1, os.size() - 2);
            }
            break;
        }
    }
    file_os.close();

    // retrieves kernel information
    struct utsname kernel;
    uname(&kernel);

    // retrieves uptime information
    struct sysinfo uptime_info;
    sysinfo(&uptime_info);

    long uptime = uptime_info.uptime;
    long days = uptime / (24 * 3600);
    long hours = (uptime % (24 * 3600)) / 3600;
    long minutes = (uptime % 3600) / 60;
    long seconds = uptime % 60;

    // retrieves information on the number of packages installed
    ifstream file_package("/var/lib/dpkg/status");
    int package = 0;
    string line_package;
    bool inPackageBlock = false;

    while (getline(file_package, line_package))
    {
        if (line_package.starts_with("Package:"))
        {
            inPackageBlock = true;
        }
        else if (line_package.empty())
        {
            if (inPackageBlock)
            {
                ++package;
                inPackageBlock = false;
            }
        }
    }

    if (inPackageBlock)
    {
        ++package;
    }
    file_package.close();

    // retrieves CPU information
    ifstream file_cpu("/proc/cpuinfo");
    string cpu, line_cpu;
    unsigned int cpu_threads = 0, cpu_speed = 0;

    while (getline(file_cpu, line_cpu))
    {
        if (line_cpu.find("model name") != string::npos)
        {
            cpu = line_cpu.substr(line_cpu.find(":") + 2);
        }
        else if (line_cpu.find("processor") != string::npos)
        {
            cpu_threads++;
        }
        else if (line_cpu.find("cpu MHz") != string::npos)
        {
            cpu_speed = stoi(line_cpu.substr(line_cpu.find(":") + 2));
        }
    }
    file_cpu.close();

    // retrieves RAM information
    ifstream ram_file("/proc/meminfo");
    string line_ram;
    long ram, used_ram, percetage_ram;

    while (getline(ram_file, line_ram))
    {
        if (line_ram.starts_with("MemTotal:"))
        {
            line_ram.erase(remove_if(line_ram.begin(), line_ram.end(), [](char ch)
                                     { return ch == ' ' || ch == '\t'; }),
                           line_ram.end());

            line_ram.erase(line_ram.length() - 2);

            ram = stol(line_ram.substr(line_ram.find(":") + 1)) * 1024;
        }
        else if (line_ram.starts_with("MemAvailable:"))
        {
            line_ram.erase(remove_if(line_ram.begin(), line_ram.end(), [](char ch)
                                     { return ch == ' ' || ch == '\t'; }),
                           line_ram.end());

            line_ram.erase(line_ram.length() - 2);

            used_ram = ram - stol(line_ram.substr(line_ram.find(":") + 1)) * 1024;

            percetage_ram = used_ram * 100 / ram;
        }
    }

    // display information simply
    cout << "User\t : " << username << "@" << kernel.nodename << endl;
    cout << "OS\t : " << os << " " << kernel.machine << endl;
    cout << "Kernel\t : " << kernel.release << endl;
    cout << "Uptime\t : " << days << " Hari " << hours << " Jam " << minutes << " Menit " << seconds << " Detik" << endl;
    cout << "Packages : " << package << " Installed" << endl;
    cout << "CPU\t : " << cpu << " (" << cpu_threads << " vCPU) " << (double)cpu_speed / 1024 << " GHz" << endl;
    cout << "RAM\t : " << size_computer(used_ram) << " / " << size_computer(ram) << " (" << percetage_ram << "%)" << endl;

    if (full)
    {
    }
    else
    {
    }
}

int main(int argc, char const *argv[])
{
    if (argc > 1)
    {
        if (strcmp(argv[1], "--version") == 0)
        {
            // print version
            cout << "Saiki v1.0.0" << endl;
        }
        else if (strcmp(argv[1], "--help") == 0)
        {
            // just info
            cout << "Saiki version 1.0.0\n";
            cout << "Usage : saiki [parameter] [parameter]\n";
            cout << "Parameters are only used if you need them\n";
            cout << "Parameters:\n";
            cout << "  --version\tdisplay the application version\n";
            cout << "  --help\tdisplay how to use and parameter list\n";
            cout << "  --real-time\truns endlessly until until killed by CTRL + C\n";
            cout << "  --full\tDisplay all hardware & software information (second parameter)" << endl;
        }
        else if (strcmp(argv[1], "--real-time") == 0)
        {
            // need full info
            if (argc > 2 && strcmp(argv[2], "--full") == 0)
                full = 1;

            while (true)
            {
                // clear console
                system("clear");

                // print stats
                stats();

                // sleep 1 sec
                this_thread::sleep_for(chrono::seconds(1));
            }
        }
        else if (strcmp(argv[1], "--full") == 0)
        {
            // need full info
            full = 1;

            // print stats
            stats();
        }
        else
        {
            // said that there is no such paramater
            cout << "The parameter you entered was not found\n";
            cout << "Try using the --help parameter to see a list of available parameters" << endl;
        }
    }
    else
    {
        // print stats
        stats();
    }

    return 0;
}
