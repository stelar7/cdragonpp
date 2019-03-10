#include "WebDownloader.hpp"
#include "../../libs/curl/include/curl.h"
#include <filesystem>
#include <iostream>
#include <map>

using namespace cdragon::web;


std::size_t writeString(void *ptr, const std::size_t size, const std::size_t nmemb, std::string* data) {
    data->append(reinterpret_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

std::size_t writeData(void *ptr, const std::size_t size, const std::size_t nmemb, FILE *stream) {
    const auto written = fwrite(ptr, size, nmemb, stream);
    return written;
}


std::string Downloader::downloadString(std::string& url) const {
    std::string responseString;

    if (handles[0]) {
        curl_easy_setopt(handles[0], CURLOPT_URL, url.c_str());
        curl_easy_setopt(handles[0], CURLOPT_WRITEDATA, &responseString);
        curl_easy_setopt(handles[0], CURLOPT_WRITEFUNCTION, writeString);

        std::cout << "DOWNLOADING: " << url << std::endl;

        const auto res = curl_easy_perform(handles[0]);
        if (res != CURLE_OK) {
            std::cout << "CURL ERROR: " << url << std::endl;
            std::cout << "CURL ERROR: " << curl_easy_strerror(res) << std::endl;
        }
        else {
            std::cout << "DOWNLOADED FILE TO MEMORY: " << url << std::endl;
        }

        return responseString;
    }

    throw std::exception("Failed to read from url!");
}


bool Downloader::downloadFile(std::string& url, std::filesystem::path& output) const {
    FILE* fp;
    auto status = true;

    create_directories(output.parent_path());
    const auto err = fopen_s(&fp, output.string().c_str(), "wb");
    if (err != 0) {
        char errmsg[256];
        strerror_s(errmsg, 256, err);
        std::cout << "FILE ERROR: Path: " << output.string() << std::endl;
        std::cout << "FILE ERROR: Message: " << errmsg << std::endl;
        return false;
    }

    if (handles[0]) {
        curl_easy_setopt(handles[0], CURLOPT_URL, url.c_str());
        curl_easy_setopt(handles[0], CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(handles[0], CURLOPT_WRITEFUNCTION, writeData);

        std::cout << "DOWNLOADING FILE: " << url << std::endl;

        const auto res = curl_easy_perform(handles[0]);
        if (res != CURLE_OK) {
            std::cout << "CURL ERROR: " << url << std::endl;
            std::cout << "CURL ERROR: " << curl_easy_strerror(res) << std::endl;
            status = false;
        }
        else {
            std::cout << "FILE SAVED TO LOCAL DRIVE: " << absolute(output).string() << std::endl;
        }

        if (fp != nullptr) {
            fclose(fp);
        }

        return status;
    }

    throw std::exception("Failed to read from url!");
}

bool Downloader::downloadFiles(std::vector<std::pair<std::string, std::filesystem::path>>& urls) const {

    if(urls.empty())
    {
        return true;
    }

    FILE* fp[HANDLE_COUNT];
    auto running_handles = 0;

    auto it = urls.begin();
    for (auto i = 0; i < HANDLE_COUNT; i++)
    {
        std::cout << "Starting download of " << it->first << std::endl;

        curl_easy_setopt(handles[i], CURLOPT_URL, it->first.c_str());

        std::filesystem::create_directories(it->second.parent_path());
        const auto err = fopen_s(&fp[i], it->second.string().c_str(), "wb");
        if (err != 0) {
            char errmsg[256];
            strerror_s(errmsg, 256, err);
            std::cout << "FILE ERROR: Path: " << it->second.string() << std::endl;
            std::cout << "FILE ERROR: Message: " << errmsg << std::endl;
            return false;
        }

        curl_easy_setopt(handles[i], CURLOPT_WRITEDATA, fp[i]);
        curl_easy_setopt(handles[i], CURLOPT_WRITEFUNCTION, writeData);
        curl_multi_add_handle(multi_handle, handles[i]);

        std::advance(it, 1);
        if (it == urls.end())
        {
            break;
        }
    }

    curl_multi_perform(multi_handle, &running_handles);

    while (running_handles)
    {
        struct timeval timeout {};

        fd_set fdread;
        fd_set fdwrite;
        fd_set fdexcep;
        auto maxfd = -1;

        auto curl_timeout = -1l;

        FD_ZERO(&fdread);
        FD_ZERO(&fdwrite);
        FD_ZERO(&fdexcep);

        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        curl_multi_timeout(multi_handle, &curl_timeout);
        if (curl_timeout >= 0) {
            timeout.tv_sec = curl_timeout / 1000;
            timeout.tv_sec = (timeout.tv_sec > 1 ? 1 : ((curl_timeout % 1000) * 1000));
        }

        const auto multi_code = curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);
        if (multi_code != CURLM_OK)
        {
            fprintf(stderr, "curl_multi_fdset() failed, code %d.\n", multi_code);
            break;
        }

        // ReSharper disable once CppInitializedValueIsAlwaysRewritten
        auto return_code = 0;

        if (maxfd == -1)
        {

#ifdef _WIN32
#include "windows.h"

            Sleep(100);
            return_code = 0;
#else
            struct timeval wait = { 0, 100 * 1000 };
            rc = select(0, NULL, NULL, NULL, &wait);
#endif
        }
        else
        {
            return_code = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
        }

        switch (return_code)
        {
            case -1: break;
            case 0:
            default: curl_multi_perform(multi_handle, &running_handles);
        }

        CURLMsg *msg;
        auto msg_count = 0;

        while ((msg = curl_multi_info_read(multi_handle, &msg_count)))
        {
            if (msg->msg == CURLMSG_DONE)
            {
                auto index = 0;
                for (; index < HANDLE_COUNT; index++) {
                    if (msg->easy_handle == handles[index])
                    {
                        break;
                    }
                }

                std::cout << "Download finished (" << running_handles << " active transfers remaining)" << std::endl;
                if (it != urls.end())
                {
                    std::advance(it, 1);
                }

                if (it == urls.end())
                {
                    break;
                }

                fclose(fp[index]);

                std::cout << "Starting download of " << it->first << std::endl;
                std::filesystem::create_directories(it->second.parent_path());
                const auto err = fopen_s(&fp[index], it->second.string().c_str(), "wb");
                if (err != 0) {
                    char errmsg[256];
                    strerror_s(errmsg, 256, err);
                    std::cout << "FILE ERROR: Path: " << it->second.string() << std::endl;
                    std::cout << "FILE ERROR: Message: " << errmsg << std::endl;
                    return false;
                }

                curl_easy_setopt(handles[index], CURLOPT_URL, it->first.c_str());
                curl_easy_setopt(handles[index], CURLOPT_WRITEDATA, fp[index]);

                // re-adding the handle is needed for it to pick-up the changed state
                curl_multi_remove_handle(multi_handle, handles[index]);
                curl_multi_add_handle(multi_handle, handles[index]);

                // run atleast once so we update the handle count
                curl_multi_perform(multi_handle, &running_handles);
            }
        }
    }

    return true;
}

