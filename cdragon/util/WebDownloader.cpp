#include "WebDownloader.hpp"
#include "../../libs/curl/include/curl.h"
#include <filesystem>
#include <iostream>
#include <thread>

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

    if (handle) {
        curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, &responseString);
        curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeString);

        std::cout << "DOWNLOADING: " << url << std::endl;

        const auto res = curl_easy_perform(handle);
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

    if (handle) {
        curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeData);

        std::cout << "DOWNLOADING FILE: " << url << std::endl;

        const auto res = curl_easy_perform(handle);
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

void push_handle(CURL* handle, FILE* fp, std::string& url, std::filesystem::path& file)
{
    std::cout << "Starting download of " << url << std::endl;

    curl_easy_setopt(handle, CURLOPT_URL, url.c_str());

    std::filesystem::create_directories(file.parent_path());
    const auto err = fopen_s(&fp, file.string().c_str(), "wb");
    if (err != 0) {
        char errmsg[256];
        strerror_s(errmsg, 256, err);
        std::cout << "FILE ERROR: Path: " << file.string() << std::endl;
        std::cout << "FILE ERROR: Message: " << errmsg << std::endl;
    }

    curl_easy_setopt(handle, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeData);
    curl_easy_setopt(handle, CURLOPT_VERBOSE, 1L);
}

bool Downloader::downloadFiles(std::vector<std::pair<std::string, std::filesystem::path>>& urls) const {

    if (urls.empty())
    {
        return true;
    }

    auto running_handles = 0;
    for (auto& url_part : urls)
    {
        auto& url = url_part.first;
        auto& loc = url_part.second;

        auto handle = curl_easy_init();
        curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0L);

        push_handle(handle, nullptr, url, loc);
        curl_multi_add_handle(multi_handle, handle);
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

        auto return_code = 0;
        if (maxfd == -1)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        else
        {
            return_code = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
        }

        if (return_code != -1)
        {
            curl_multi_perform(multi_handle, &running_handles);
        }

        CURLMsg *msg;
        auto msg_count = 0;

        while ((msg = curl_multi_info_read(multi_handle, &msg_count)))
        {

            if (msg->msg == CURLMSG_DONE)
            {
                std::cout << curl_easy_strerror(msg->data.result) << "(" << msg->data.result << ") - ";
                std::cout << "Download finished (" << running_handles << " active transfers remaining)" << std::endl;
                break;
            }
        }
    }

    return true;
}

