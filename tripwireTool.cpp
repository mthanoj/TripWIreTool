#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace fs = std::filesystem;

std::mutex logMutex;
std::mutex coutMutex;

const std::wstring LOG_FILE = L"tripwire_log.txt";
const std::wstring HTML_LOG_FILE = L"tripwire_log.html";

struct WatchTarget {
    std::wstring directory;
};

std::wstring GetTimestamp()
{
    auto now = std::chrono::system_clock::now();
    std::time_t timeNow = std::chrono::system_clock::to_time_t(now);

    std::tm localTime{};
    localtime_s(&localTime, &timeNow);

    std::wstringstream wss;
    wss << std::put_time(&localTime, L"%Y-%m-%d %H:%M:%S");
    return wss.str();
}

std::wstring ToLower(const std::wstring& input)
{
    std::wstring result = input;
    std::transform(result.begin(), result.end(), result.begin(), towlower);
    return result;
}

bool DirectoryExists(const std::wstring& path)
{
    std::error_code ec;
    return fs::exists(path, ec) && fs::is_directory(path, ec);
}

bool FileExists(const std::wstring& path)
{
    std::error_code ec;
    return fs::exists(path, ec) && fs::is_regular_file(path, ec);
}

void EnsureDirectoryExists(const std::wstring& path)
{
    std::error_code ec;
    if (!fs::exists(path, ec)) {
        fs::create_directories(path, ec);
    }
}

std::wstring EscapeHtml(const std::wstring& input)
{
    std::wstring output;
    for (wchar_t ch : input) {
        switch (ch) {
        case L'&': output += L"&amp;"; break;
        case L'<': output += L"&lt;"; break;
        case L'>': output += L"&gt;"; break;
        case L'"': output += L"&quot;"; break;
        case L'\'': output += L"&#39;"; break;
        default: output += ch; break;
        }
    }
    return output;
}

void InitializeHtmlLog()
{
    std::wofstream htmlFile(HTML_LOG_FILE, std::ios::trunc);
    if (!htmlFile.is_open()) {
        return;
    }

    htmlFile << L"<!DOCTYPE html>\n";
    htmlFile << L"<html lang=\"en\">\n";
    htmlFile << L"<head>\n";
    htmlFile << L"  <meta charset=\"UTF-8\">\n";
    htmlFile << L"  <meta http-equiv=\"refresh\" content=\"3\">\n";
    htmlFile << L"  <title>Tripwire Log</title>\n";
    htmlFile << L"  <style>\n";
    htmlFile << L"    body { font-family: Arial, sans-serif; margin: 20px; background-color: #f4f6f8; }\n";
    htmlFile << L"    h1 { color: #1f3b57; }\n";
    htmlFile << L"    table { border-collapse: collapse; width: 100%; background: white; }\n";
    htmlFile << L"    th, td { border: 1px solid #ccc; padding: 10px; text-align: left; }\n";
    htmlFile << L"    th { background-color: #1f3b57; color: white; }\n";
    htmlFile << L"    tr:nth-child(even) { background-color: #f9f9f9; }\n";
    htmlFile << L"  </style>\n";
    htmlFile << L"</head>\n";
    htmlFile << L"<body>\n";
    htmlFile << L"  <h1>Tripwire Monitoring Log</h1>\n";
    htmlFile << L"  <p>This page refreshes every 3 seconds.</p>\n";
    htmlFile << L"  <table>\n";
    htmlFile << L"    <tr><th>Timestamp</th><th>Event</th></tr>\n";
    htmlFile << L"  </table>\n";
    htmlFile << L"</body>\n";
    htmlFile << L"</html>\n";

    htmlFile.close();
}

void AppendHtmlLog(const std::wstring& timestamp, const std::wstring& message)
{
    std::wifstream inFile(HTML_LOG_FILE);
    if (!inFile.is_open()) {
        return;
    }

    std::wstringstream buffer;
    buffer << inFile.rdbuf();
    std::wstring content = buffer.str();
    inFile.close();

    std::wstring row = L"    <tr><td>" + EscapeHtml(timestamp) + L"</td><td>" + EscapeHtml(message) + L"</td></tr>\n";

    size_t tableEnd = content.rfind(L"</table>");
    if (tableEnd == std::wstring::npos) {
        return;
    }

    content.insert(tableEnd, row);

    std::wofstream outFile(HTML_LOG_FILE, std::ios::trunc);
    if (!outFile.is_open()) {
        return;
    }

    outFile << content;
    outFile.close();
}

void LogEvent(const std::wstring& message)
{
    std::lock_guard<std::mutex> lock(logMutex);

    std::wstring timestamp = GetTimestamp();

    std::wofstream logFile(LOG_FILE, std::ios::app);
    if (logFile.is_open()) {
        logFile << L"[" << timestamp << L"] " << message << std::endl;
        logFile.close();
    }

    AppendHtmlLog(timestamp, message);

    std::lock_guard<std::mutex> coutLock(coutMutex);
    std::wcout << L"[" << timestamp << L"] " << message << std::endl;
}

void CreateFileIfMissing(const std::wstring& filePath, const std::wstring& content)
{
    if (!FileExists(filePath)) {
        std::wofstream out(filePath);
        if (out.is_open()) {
            out << content << std::endl;
            out.close();
            LogEvent(L"Created decoy tripwire file: " + filePath);
        }
        else {
            LogEvent(L"Failed to create decoy file: " + filePath);
        }
    }
    else {
        LogEvent(L"Decoy file already exists: " + filePath);
    }
}

void CreateDefaultDecoyFiles(const std::wstring& decoyFolder)
{
    EnsureDirectoryExists(decoyFolder);

    CreateFileIfMissing(decoyFolder + L"\\payroll_backup.txt",
        L"Tripwire decoy file - unauthorized access may indicate suspicious activity.");

    CreateFileIfMissing(decoyFolder + L"\\admin_passwords.txt",
        L"Tripwire decoy file - this file is monitored.");

    CreateFileIfMissing(decoyFolder + L"\\confidential_notes.txt",
        L"Tripwire decoy file - access to this file should be investigated.");

    CreateFileIfMissing(decoyFolder + L"\\finance_records.txt",
        L"Tripwire decoy file - monitored by defensive tool.");
}

std::wstring ActionToString(DWORD action)
{
    switch (action) {
    case FILE_ACTION_ADDED:
        return L"CREATED";
    case FILE_ACTION_REMOVED:
        return L"DELETED";
    case FILE_ACTION_MODIFIED:
        return L"MODIFIED";
    case FILE_ACTION_RENAMED_OLD_NAME:
        return L"RENAMED_OLD_NAME";
    case FILE_ACTION_RENAMED_NEW_NAME:
        return L"RENAMED_NEW_NAME";
    default:
        return L"UNKNOWN";
    }
}

void MonitorDirectory(const std::wstring& directoryPath)
{
    HANDLE dirHandle = CreateFileW(
        directoryPath.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr
    );

    if (dirHandle == INVALID_HANDLE_VALUE) {
        LogEvent(L"Failed to monitor directory: " + directoryPath);
        return;
    }

    LogEvent(L"Now monitoring directory: " + directoryPath);

    const DWORD bufferSize = 8192;
    BYTE buffer[bufferSize];
    DWORD bytesReturned = 0;

    while (true) {
        BOOL success = ReadDirectoryChangesW(
            dirHandle,
            &buffer,
            bufferSize,
            TRUE,
            FILE_NOTIFY_CHANGE_FILE_NAME |
            FILE_NOTIFY_CHANGE_DIR_NAME |
            FILE_NOTIFY_CHANGE_LAST_WRITE |
            FILE_NOTIFY_CHANGE_CREATION |
            FILE_NOTIFY_CHANGE_SIZE,
            &bytesReturned,
            nullptr,
            nullptr
        );

        if (!success) {
            LogEvent(L"ReadDirectoryChangesW failed for: " + directoryPath);
            break;
        }

        FILE_NOTIFY_INFORMATION* info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer);

        while (true) {
            std::wstring fileName(info->FileName, info->FileNameLength / sizeof(WCHAR));
            std::wstring fullPath = directoryPath + L"\\" + fileName;
            std::wstring action = ActionToString(info->Action);

            LogEvent(L"[" + directoryPath + L"] " + action + L": " + fullPath);

            if (info->NextEntryOffset == 0) {
                break;
            }

            info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                reinterpret_cast<BYTE*>(info) + info->NextEntryOffset
                );
        }
    }

    CloseHandle(dirHandle);
}

void AddUserDirectories(std::vector<WatchTarget>& targets)
{
    while (true) {
        std::wcout << L"\nEnter a folder path to monitor (or type DONE to finish): ";
        std::wstring input;
        std::getline(std::wcin, input);

        if (ToLower(input) == L"done") {
            break;
        }

        if (input.empty()) {
            continue;
        }

        if (DirectoryExists(input)) {
            bool alreadyExists = false;
            for (const auto& t : targets) {
                if (ToLower(t.directory) == ToLower(input)) {
                    alreadyExists = true;
                    break;
                }
            }

            if (!alreadyExists) {
                targets.push_back({ input });
                LogEvent(L"Added user directory to watch list: " + input);
            }
            else {
                LogEvent(L"Directory already in watch list: " + input);
            }
        }
        else {
            LogEvent(L"Invalid directory path, not added: " + input);
        }
    }
}

int wmain()
{
    std::wcout << L"=== Simple Tripwire / Defensive Monitoring Tool ===\n";
    std::wcout << L"This tool creates decoy files and monitors folders for changes.\n\n";

    InitializeHtmlLog();

    std::vector<WatchTarget> watchTargets;

    std::wstring decoyFolder = L".\\DecoyFiles";
    CreateDefaultDecoyFiles(decoyFolder);

    watchTargets.push_back({ decoyFolder });

    AddUserDirectories(watchTargets);

    LogEvent(L"Monitoring session started.");
    LogEvent(L"Total watched directories: " + std::to_wstring(watchTargets.size()));
    LogEvent(L"HTML log available at: " + HTML_LOG_FILE);

    std::vector<std::thread> monitorThreads;
    for (const auto& target : watchTargets) {
        monitorThreads.emplace_back(MonitorDirectory, target.directory);
    }

    std::wcout << L"\nMonitoring is active. Press Ctrl+C to stop.\n";
    std::wcout << L"Open tripwire_log.html in your browser to view the HTML log.\n";

    for (auto& thread : monitorThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    return 0;
}