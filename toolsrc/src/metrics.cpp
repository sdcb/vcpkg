#include "metrics.h"
#include <utility>
#include <array>
#include <string>
#include <iostream>
#include <vector>
#include <sys/timeb.h>
#include <time.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winhttp.h>
#include <fstream>
#include "filesystem_fs.h"
#include "vcpkg_Strings.h"
#include "vcpkg_System.h"

namespace vcpkg
{
    static std::string GetCurrentDateTime()
    {
        struct tm newtime;
        time_t now;
        int milli;
        std::array<char, 80> date;
        date.fill(0);

        struct _timeb timebuffer;

        _ftime_s(&timebuffer);
        now = timebuffer.time;
        milli = timebuffer.millitm;

        errno_t err = gmtime_s(&newtime, &now);
        if (err)
        {
            return "";
        }

        strftime(&date[0], date.size(), "%Y-%m-%dT%H:%M:%S", &newtime);
        return std::string(&date[0]) + "." + std::to_string(milli) + "Z";
    }

    static std::string GenerateRandomUUID()
    {
        int partSizes[] = {8, 4, 4, 4, 12};
        char uuid[37];
        memset(uuid, 0, sizeof(uuid));
        int num;
        srand(static_cast<int>(time(nullptr)));
        int index = 0;
        for (int part = 0; part < 5; part++)
        {
            if (part > 0)
            {
                uuid[index] = '-';
                index++;
            }

            // Generating UUID format version 4
            // http://en.wikipedia.org/wiki/Universally_unique_identifier
            for (int i = 0; i < partSizes[part]; i++ , index++)
            {
                if (part == 2 && i == 0)
                {
                    num = 4;
                }
                else if (part == 4 && i == 0)
                {
                    num = (rand() % 4) + 8;
                }
                else
                {
                    num = rand() % 16;
                }

                if (num < 10)
                {
                    uuid[index] = static_cast<char>('0' + num);
                }
                else
                {
                    uuid[index] = static_cast<char>('a' + (num - 10));
                }
            }
        }

        return uuid;
    }

    static const std::string& get_session_id()
    {
        static const std::string id = GenerateRandomUUID();
        return id;
    }

    static std::string to_json_string(const std::string& str)
    {
        std::string encoded = "\"";
        for (auto&& ch : str)
        {
            if (ch == '\\')
            {
                encoded.append("\\\\");
            }
            else if (ch == '"')
            {
                encoded.append("\\\"");
            }
            else if (ch < 0x20 || ch >= 0x80)
            {
                // Note: this treats incoming Strings as Latin-1
                static constexpr const char hex[16] = {
                    '0', '1', '2', '3', '4', '5', '6', '7',
                    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
                encoded.append("\\u00");
                encoded.push_back(hex[ch / 16]);
                encoded.push_back(hex[ch % 16]);
            }
            else
            {
                encoded.push_back(ch);
            }
        }
        encoded.push_back('"');
        return encoded;
    }

    static std::string get_os_version_string()
    {
        std::wstring path;
        path.resize(MAX_PATH);
        auto n = GetSystemDirectoryW(&path[0], static_cast<UINT>(path.size()));
        path.resize(n);
        path += L"\\kernel32.dll";

        auto versz = GetFileVersionInfoSizeW(path.c_str(), nullptr);
        if (versz == 0)
            return "";

        std::vector<char> verbuf;
        verbuf.resize(versz);

        if (!GetFileVersionInfoW(path.c_str(), 0, static_cast<DWORD>(verbuf.size()), &verbuf[0]))
            return "";

        void* rootblock;
        UINT rootblocksize;
        if (!VerQueryValueW(&verbuf[0], L"\\", &rootblock, &rootblocksize))
            return "";

        auto rootblock_ffi = static_cast<VS_FIXEDFILEINFO *>(rootblock);

        return Strings::format("%d.%d.%d",
                               static_cast<int>(HIWORD(rootblock_ffi->dwProductVersionMS)),
                               static_cast<int>(LOWORD(rootblock_ffi->dwProductVersionMS)),
                               static_cast<int>(HIWORD(rootblock_ffi->dwProductVersionLS)));
    }

    struct MetricMessage
    {
        std::string user_id = GenerateRandomUUID();
        std::string user_timestamp;
        std::string timestamp = GetCurrentDateTime();
        std::string properties;
        std::string measurements;

        void TrackProperty(const std::string& name, const std::string& value)
        {
            if (properties.size() != 0)
                properties.push_back(',');
            properties.append(to_json_string(name));
            properties.push_back(':');
            properties.append(to_json_string(value));
        }

        void TrackMetric(const std::string& name, double value)
        {
            if (measurements.size() != 0)
                measurements.push_back(',');
            measurements.append(to_json_string(name));
            measurements.push_back(':');
            measurements.append(std::to_string(value));
        }

        std::string format_event_data_template() const
        {
            const std::string& session_id = get_session_id();
            return Strings::format(R"([{
    "ver": 1,
    "name": "Microsoft.ApplicationInsights.Event",
    "time": "%s",
    "sampleRate": 100.000000,
    "seq": "0:0",
    "iKey": "b4e88960-4393-4dd9-ab8e-97e8fe6d7603",
    "flags": 0.000000,
    "tags": {
        "ai.device.os": "Windows",
        "ai.device.osVersion": "%s",
        "ai.session.id": "%s",
        "ai.user.id": "%s",
        "ai.user.accountAcquisitionDate": "%s"
    },
    "data": {
        "baseType": "EventData",
        "baseData": {
            "ver": 2,
            "name": "commandline_test7",
            "properties": { %s },
            "measurements": { %s }
        }
    }
}])",
                                   timestamp,
                                   get_os_version_string(),
                                   session_id,
                                   user_id,
                                   user_timestamp,
                                   properties,
                                   measurements);
        }
    };

    static MetricMessage g_metricmessage;
    static bool g_should_send_metrics =
#if defined(NDEBUG) && (DISABLE_METRICS == 0)
true
#else
    false
#endif
    ;
    static bool g_should_print_metrics = false;

    bool GetCompiledMetricsEnabled()
    {
        return DISABLE_METRICS == 0;
    }

    std::wstring GetSQMUser()
    {
        LONG err = NULL;

        struct RAII_HKEY {
            HKEY hkey = NULL;
            ~RAII_HKEY()
            {
                if (hkey != NULL)
                    RegCloseKey(hkey);
            }
        } HKCU_SQMClient;

        err = RegOpenKeyExW(HKEY_CURRENT_USER, LR"(Software\Microsoft\SQMClient)", NULL, KEY_READ, &HKCU_SQMClient.hkey);
        if (err != ERROR_SUCCESS)
        {
            return L"{}";
        }

        std::array<wchar_t,128> buffer;
        DWORD lType = 0;
        DWORD dwBufferSize = static_cast<DWORD>(buffer.size() * sizeof(wchar_t));
        err = RegQueryValueExW(HKCU_SQMClient.hkey, L"UserId", NULL, &lType, reinterpret_cast<LPBYTE>(buffer.data()), &dwBufferSize);
        if (err == ERROR_SUCCESS && lType == REG_SZ && dwBufferSize >= sizeof(wchar_t))
        {
            size_t sz = dwBufferSize / sizeof(wchar_t);
            if (buffer[sz - 1] == '\0')
                --sz;
            return std::wstring(buffer.begin(), buffer.begin() + sz);
        }

        return L"{}";
    }

    void SetUserInformation(const std::string& user_id, const std::string& first_use_time)
    {
        g_metricmessage.user_id = user_id;
        g_metricmessage.user_timestamp = first_use_time;
    }

    void InitUserInformation(std::string& user_id, std::string& first_use_time)
    {
        user_id = GenerateRandomUUID();
        first_use_time = GetCurrentDateTime();
    }

    void SetSendMetrics(bool should_send_metrics)
    {
        g_should_send_metrics = should_send_metrics;
    }

    void SetPrintMetrics(bool should_print_metrics)
    {
        g_should_print_metrics = should_print_metrics;
    }

    void TrackMetric(const std::string& name, double value)
    {
        g_metricmessage.TrackMetric(name, value);
    }

    void TrackProperty(const std::string& name, const std::wstring& value)
    {
        // Note: this is not valid UTF-16 -> UTF-8, it just yields a close enough approximation for our purposes.
        std::string converted_value;
        converted_value.resize(value.size());
        std::transform(
            value.begin(), value.end(),
            converted_value.begin(),
            [](wchar_t ch)
            {
                return static_cast<char>(ch);
            });

        g_metricmessage.TrackProperty(name, converted_value);
    }

    void TrackProperty(const std::string& name, const std::string& value)
    {
        g_metricmessage.TrackProperty(name, value);
    }

    void Upload(const std::string& payload)
    {
        HINTERNET hSession = nullptr, hConnect = nullptr, hRequest = nullptr;
        BOOL bResults = FALSE;

        hSession = WinHttpOpen(L"vcpkg/1.0",
                               WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                               WINHTTP_NO_PROXY_NAME,
                               WINHTTP_NO_PROXY_BYPASS,
                               0);
        if (hSession)
            hConnect = WinHttpConnect(hSession, L"dc.services.visualstudio.com", INTERNET_DEFAULT_HTTPS_PORT, 0);

        if (hConnect)
            hRequest = WinHttpOpenRequest(hConnect,
                                          L"POST",
                                          L"/v2/track",
                                          nullptr,
                                          WINHTTP_NO_REFERER,
                                          WINHTTP_DEFAULT_ACCEPT_TYPES,
                                          WINHTTP_FLAG_SECURE);

        if (hRequest)
        {
            if (MAXDWORD <= payload.size())
                abort();
            std::wstring hdrs = L"Content-Type: application/json\r\n";
            bResults = WinHttpSendRequest(hRequest,
                                          hdrs.c_str(), static_cast<DWORD>(hdrs.size()),
                                          (void*)&payload[0], static_cast<DWORD>(payload.size()), static_cast<DWORD>(payload.size()),
                                          0);
        }

        if (bResults)
        {
            bResults = WinHttpReceiveResponse(hRequest, nullptr);
        }

        DWORD http_code = 0, junk = sizeof(DWORD);

        if (bResults)
        {
            bResults = WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, nullptr, &http_code, &junk, WINHTTP_NO_HEADER_INDEX);
        }

        std::vector<char> responseBuffer;
        if (bResults)
        {
            DWORD availableData = 0, readData = 0, totalData = 0;

            while ((bResults = WinHttpQueryDataAvailable(hRequest, &availableData)) && availableData > 0)
            {
                responseBuffer.resize(responseBuffer.size() + availableData);

                bResults = WinHttpReadData(hRequest, &responseBuffer.data()[totalData], availableData, &readData);

                if (!bResults)
                {
                    break;
                }

                totalData += readData;

                responseBuffer.resize(totalData);
            }
        }

        if (!bResults)
        {
#ifndef NDEBUG
            __debugbreak();
            auto err = GetLastError();
            std::cerr << "[DEBUG] failed to connect to server: " << err << "\n";
#endif
        }

        if (hRequest)
            WinHttpCloseHandle(hRequest);
        if (hConnect)
            WinHttpCloseHandle(hConnect);
        if (hSession)
            WinHttpCloseHandle(hSession);
    }

    static fs::path get_bindir()
    {
        wchar_t buf[_MAX_PATH ];
        int bytes = GetModuleFileNameW(nullptr, buf, _MAX_PATH);
        if (bytes == 0)
            std::abort();
        return fs::path(buf, buf + bytes);
    }

    void Flush()
    {
        std::string payload = g_metricmessage.format_event_data_template();
        if (g_should_print_metrics)
            std::cerr << payload << "\n";
        if (!g_should_send_metrics)
            return;

        // Upload(payload);

        wchar_t temp_folder[MAX_PATH];
        GetTempPathW(MAX_PATH, temp_folder);

        const fs::path temp_folder_path = temp_folder;
        const fs::path temp_folder_path_exe = temp_folder_path / "vcpkgmetricsuploader.exe";

        if (true)
        {
            const fs::path exe_path = []() -> fs::path
                {
                    auto vcpkgdir = get_bindir().parent_path();
                    auto path = vcpkgdir / "vcpkgmetricsuploader.exe";
                    if (fs::exists(path))
                        return path;

                    path = vcpkgdir / "scripts" / "vcpkgmetricsuploader.exe";
                    if (fs::exists(path))
                        return path;

                    return L"";
                }();

            std::error_code ec;
            fs::copy_file(exe_path, temp_folder_path_exe, fs::copy_options::skip_existing, ec);
            if (ec)
                return;
        }

        const fs::path vcpkg_metrics_txt_path = temp_folder_path / ("vcpkg" + GenerateRandomUUID() + ".txt");
        std::ofstream(vcpkg_metrics_txt_path) << payload;

        const std::wstring cmdLine = Strings::wformat(L"start %s %s", temp_folder_path_exe.native(), vcpkg_metrics_txt_path.native());
        System::cmd_execute(cmdLine);
    }
}
