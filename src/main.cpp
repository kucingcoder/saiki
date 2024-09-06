#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>
#include <fstream>
#include <algorithm>
#include <vector>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <dirent.h>
#include "fort.hpp"

extern "C"
{
#include <pci/pci.h>
}

using namespace std;
using namespace fort;

bool full = 0;

enum ProcessState
{
    RUNNING,
    SLEEPING,
    STOPPED,
    ZOMBIE,
    UNKNOWN
};

string
    username,
    hostname,
    os_name, os_arch, kernel_linux,
    uptime_system, installed_packages,
    cpu_status, ram_status, swap_status,
    input_data, output_data;

int runningCount,
    sleepingCount,
    stoppedCount,
    zombieCount;

vector<string> pcie_devices_for_graphic_and_ai;

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
    string types;
    ostringstream oss;

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

    oss << fixed << setprecision(3) << numbers;
    string easy_number = oss.str();

    return easy_number + " " + types;
}

ProcessState getProcessState(const string &statusFilePath)
{
    ifstream statusFile(statusFilePath);
    string line;
    while (getline(statusFile, line))
    {
        if (line.find("State:") == 0)
        {
            char state;
            sscanf(line.c_str(), "State: %c", &state);
            switch (state)
            {
            case 'R':
                statusFile.close();
                return RUNNING;
            case 'S':
                statusFile.close();
                return SLEEPING;
            case 'T':
                statusFile.close();
                return STOPPED;
            case 'Z':
                statusFile.close();
                return ZOMBIE;
            default:
                statusFile.close();
                return UNKNOWN;
            }
        }
    }

    statusFile.close();
    return UNKNOWN;
}

void stats()
{
    // retrieve username
    username = getenv("USER");

    // retrieving OS information
    ifstream file_os("/etc/os-release");
    string line_os;

    while (getline(file_os, line_os))
    {
        if (line_os.starts_with("PRETTY_NAME="))
        {
            os_name = line_os.substr(12);
            if (!os_name.empty() && os_name.front() == '"' && os_name.back() == '"')
            {
                os_name = os_name.substr(1, os_name.size() - 2);
            }
            break;
        }
    }
    file_os.close();

    // retrieves kernel information
    struct utsname kernel;
    uname(&kernel);

    hostname = kernel.nodename;
    os_arch = kernel.machine;
    kernel_linux = kernel.release;

    // retrieves uptime information
    struct sysinfo uptime_swap_info;
    sysinfo(&uptime_swap_info);

    long uptime = uptime_swap_info.uptime;
    long days = uptime / (24 * 3600);
    long hours = (uptime % (24 * 3600)) / 3600;
    long minutes = (uptime % 3600) / 60;
    long seconds = uptime % 60;

    string time = "";

    if (days > 0)
    {
        time += to_string(days) + " Days ";
    }

    if (hours > 0)
    {
        time += to_string(hours) + " Hours ";
    }

    if (minutes > 0)
    {
        time += to_string(minutes) + " Mins ";
    }

    if (seconds > 0)
    {
        time += to_string(seconds) + " Secs";
    }

    uptime_system = time;

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

    installed_packages = to_string(package) + " Installed";

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

    ostringstream ostring_cpu_speed;
    double cpu_speed_ghz = cpu_speed / 1024.0;
    ostring_cpu_speed << fixed << setprecision(2) << cpu_speed_ghz;
    string simple_cpu_speed = ostring_cpu_speed.str();

    cpu_status = cpu + " (" + to_string(cpu_threads) + " vCPU) " + simple_cpu_speed + " GHz";

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
            break;
        }
    }
    ram_file.close();

    ram_status = size_computer(used_ram) + " / " + size_computer(ram) + " (" + to_string(percetage_ram) + "%)";

    // retrieves SWAP information
    long swap = uptime_swap_info.totalswap;
    long used_swap = uptime_swap_info.totalswap - uptime_swap_info.freeswap;
    long percetage_swap = used_swap * 100 / swap;
    swap_status = size_computer(used_swap) + " / " + size_computer(swap) + " (" + to_string(percetage_swap) + "%)";

    if (full)
    {
        // retrieves PCIe devices for graphics or ai
        struct pci_access *pacc;
        struct pci_dev *dev;
        char namebuf[1024], *name;

        pacc = pci_alloc();
        pci_init(pacc);
        pci_scan_bus(pacc);

        string device_now;

        pcie_devices_for_graphic_and_ai.clear();

        for (dev = pacc->devices; dev; dev = dev->next)
        {
            pci_fill_info(dev, PCI_FILL_IDENT | PCI_FILL_BASES | PCI_FILL_CLASS);
            name = pci_lookup_name(pacc, namebuf, sizeof(namebuf), PCI_LOOKUP_DEVICE, dev->vendor_id, dev->device_id);
            if (name && *name)
            {
                device_now = name;

                for (const auto &keyword : {"GPU", "Render", "3D", "AI", "Accelerator"})
                {
                    if (device_now.find(keyword) != string::npos)
                    {
                        pcie_devices_for_graphic_and_ai.push_back(name);
                    }
                }
            }
        }
        pci_cleanup(pacc);

        // retrieves task info
        const string procDir = "/proc";
        DIR *dir = opendir(procDir.c_str());

        struct dirent *entry;

        runningCount = 0;
        sleepingCount = 0;
        stoppedCount = 0;
        zombieCount = 0;

        while ((entry = readdir(dir)) != nullptr)
        {
            if (entry->d_type == DT_DIR && isdigit(entry->d_name[0]))
            {
                string pidDir = procDir + "/" + entry->d_name;
                string statusFilePath = pidDir + "/status";
                ProcessState state = getProcessState(statusFilePath);
                switch (state)
                {
                case RUNNING:
                    runningCount++;
                    break;
                case SLEEPING:
                    sleepingCount++;
                    break;
                case STOPPED:
                    stoppedCount++;
                    break;
                case ZOMBIE:
                    zombieCount++;
                    break;
                case UNKNOWN:
                    break;
                }
            }
        }
        closedir(dir);

        // retrieves RX & TX info
        ifstream file_rxtx("/proc/net/dev");
        string line;

        getline(file_rxtx, line);
        getline(file_rxtx, line);

        uint64_t totalRxBytes = 0;
        uint64_t totalTxBytes = 0;

        while (getline(file_rxtx, line))
        {
            istringstream iss(line);
            string interface;
            uint64_t rxBytes, txBytes;
            if (iss >> interface >> rxBytes)
            {
                for (int i = 0; i < 7; ++i)
                {
                    iss >> ws;
                    if (iss.peek() == ' ')
                        iss.ignore();
                }
                iss >> txBytes;

                interface = interface.substr(0, interface.size() - 1);

                totalRxBytes += rxBytes;
                totalTxBytes += txBytes;
            }
        }
        file_rxtx.close();

        input_data = size_computer(totalRxBytes);
        output_data = size_computer(totalTxBytes);
    }
}

void print()
{
    // a table structure for hardware info
    table specs;
    specs.set_border_style(FT_NICE_STYLE);
    specs << header << "HARDWARE" << endr
          << "CPU" << cpu_status << endr
          << "RAM" << ram_status << endr
          << "SWAP" << swap_status << endr
          << "" << "" << endr
          << "" << "" << endr;

    // a table structure for os info
    table os;
    os.set_border_style(FT_NICE_STYLE);
    os << header << "OS" << endr
       << "Distro" << os_name + " " + os_arch << endr
       << "Kernel" << kernel_linux << endr
       << "Uptime" << uptime_system << endr
       << "Packages" << installed_packages << endr
       << "User" << username + "@" + hostname << endr;

    string table_specs = specs.to_string();
    string table_os = os.to_string();

    // format so that they can be printed side by side
    istringstream table_specs_stream(table_specs);
    istringstream table_os_stream(table_os);
    string table_specs_line, table_os_line;

    while (getline(table_specs_stream, table_specs_line) && getline(table_os_stream, table_os_line))
    {
        cout << table_specs_line << " " << table_os_line << endl;
    }

    // print full info
    if (full)
    {
        // a table structure for Task info
        table task_count;
        task_count.set_border_style(FT_NICE_STYLE);
        task_count << header << "" << "Running" << "Sleeping" << "Stopped" << "Zombie" << endr
                   << "Task" << runningCount << sleepingCount << stoppedCount << zombieCount << endr;

        // a table structure for RXTX info
        table rxtx_count;
        rxtx_count.set_border_style(FT_NICE_STYLE);
        rxtx_count << header << "" << "RX (Incoming)" << "TX (Outgoing)" << endr
                   << "Size" << input_data << output_data << endr;

        string table_task = task_count.to_string();
        string table_data_flow = rxtx_count.to_string();

        // format so that they can be printed side by side
        istringstream table_task_stream(table_task);
        istringstream table_data_flow_stream(table_data_flow);
        string table_task_line, table_data_flow_line;

        while (getline(table_task_stream, table_task_line) && getline(table_data_flow_stream, table_data_flow_line))
        {
            cout << table_task_line << " " << table_data_flow_line << endl;
        }

        // a table structure for PCIe devices info
        table pcie;
        pcie.set_border_style(FT_NICE_STYLE);
        pcie << header << "Graphics or AI Accelerator (PCIe)" << endr;

        for (const string &item : pcie_devices_for_graphic_and_ai)
        {
            pcie << item << endr;
        }

        cout << pcie.to_string();
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
                // get info
                stats();

                // clear console
                system("clear");

                // print info
                print();

                // sleep 5 sec
                this_thread::sleep_for(chrono::seconds(5));
            }
        }
        else if (strcmp(argv[1], "--full") == 0)
        {
            // need full info
            full = 1;

            // get info
            stats();

            // print info
            print();
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
        // get info
        stats();

        // print info
        print();
    }

    return 0;
}
