#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>
#include <fstream>
#include <algorithm>
#include <string.h>
#include <vector>
#include <sys/sysinfo.h>
#include <sys/utsname.h>

extern "C"
{
#include <pci/pci.h>
}

using namespace std;

bool full = 0;

string
    username,
    hostname,
    os_name, os_arch, kernel_linux,
    uptime_system, installed_packages,
    cpu_status, ram_status, swap_status;

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

    if (days > 0)
    {
        uptime_system += days + " Days ";
    }

    if (hours > 0)
    {
        uptime_system += hours + " Hours ";
    }

    if (minutes > 0)
    {
        uptime_system += minutes + " Min ";
    }

    if (seconds > 0)
    {
        uptime_system += seconds + " Sec";
    }

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

    installed_packages = package + " Installed";

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

    cpu_status = cpu + " (" + to_string(cpu_threads) + " vCPU) " + to_string((double)cpu_speed / 1024) + " GHz";

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

        for (dev = pacc->devices; dev; dev = dev->next)
        {
            pci_fill_info(dev, PCI_FILL_IDENT | PCI_FILL_BASES | PCI_FILL_CLASS);
            name = pci_lookup_name(pacc, namebuf, sizeof(namebuf), PCI_LOOKUP_DEVICE, dev->vendor_id, dev->device_id);
            if (name && *name)
            {
                device_now = name;

                for (const auto &keyword : {"GPU", "Render", "3D", "AI", "Accelerator"})
                {
                    if (device_now.find(keyword) != std::string::npos)
                    {
                        pcie_devices_for_graphic_and_ai.push_back(name);
                    }
                }
            }
        }
        pci_cleanup(pacc);
    }
}

void print()
{
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
                // get info
                stats();

                // clear console
                system("clear");

                // print info
                print();

                // sleep 1 sec
                this_thread::sleep_for(chrono::seconds(1));
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
