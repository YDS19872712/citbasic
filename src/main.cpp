#include <algorithm>
#include <string>
#include <fstream>
#include <iostream>
#include <windows.h>
#include "Interpreter.hpp"
#include "resource.h"

int main(int argc, char* argv[])
{
    HWND hwnd = ::GetConsoleWindow();

    HINSTANCE hinst = reinterpret_cast<HINSTANCE>(
        ::GetWindowLongPtr(hwnd, GWLP_HINSTANCE));

    HICON hicon = ::LoadIcon(hinst, MAKEINTRESOURCE(IDI_MAIN));

    ::SetClassLongPtr(hwnd, GCLP_HICON, reinterpret_cast<LONG_PTR>(hicon));

    std::string title("CIT BASIC 1.0");
    ::SetConsoleTitleA(title.c_str());

    UINT codePageId = 1251;

    if (argc > 2)
    {
        std::string codePage(argv[2]);

        std::transform(
            codePage.begin(),
            codePage.end(),
            codePage.begin(),
            std::toupper);

        if (("DOS" == codePage) || ("OEM" == codePage))
            codePageId = CP_OEMCP;
        else
            codePageId = static_cast<UINT>(std::stoul(codePage));
    }

    ::SetConsoleCP(codePageId);
    ::SetConsoleOutputCP(codePageId);

    bool fileNamePassedAsParameter = false;

    std::string fileName;

    if (argc <= 1)
    {
        std::cout << "Input source file name: ";
        std::getline(std::cin, fileName);
    }
    else
    {
        fileName = argv[1];
        fileNamePassedAsParameter = true;
    }

    std::ifstream file(fileName.c_str(), std::ios::in);

    if (file.is_open())
    {
        title.append(" \"").append(fileName).append("\"");
        ::SetConsoleTitleA(title.c_str());

        Interpreter interpreter;
        bool isLoaded = interpreter.load(file);
        
        file.close();

        if (isLoaded)
            interpreter.run();
    }
    else
    {
        std::cerr << "Cannot open \"" << fileName << "\"!\n";
    }

    if (!fileNamePassedAsParameter)
    {
        std::cout << "Press space bar to exit...\n";

        while (!((::GetForegroundWindow() == hwnd) &&
                ((::GetKeyState(VK_SPACE) & 0x8000) != 0)))
            ::Sleep(0);
    }

    return 0;
}
