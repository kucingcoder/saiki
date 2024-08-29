#include <iostream>
#include <string.h>
#include <csignal>
#include <thread>
#include <chrono>
using namespace std;

bool full = 0;

void handleSIGINT(int signal)
{
    exit(0);
}

void stats()
{
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
                // print stats
                stats();

                // ensures that the output buffer is printed directly to the screen
                cout.flush();

                // sleep 1 sec
                this_thread::sleep_for(chrono::seconds(3));

                // moves the cursor back to the beginning of the line without creating a new line.
                cout << "\r";
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
