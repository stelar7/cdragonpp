#pragma once

#include "WebDownloader.hpp"
#include "../../libs/curl/include/curl.h"
#include <filesystem>
#include <iostream>
#include <string.h>
#include <errno.h>

using namespace cdragon::web;


std::size_t writeString(void *ptr, std::size_t size, std::size_t nmemb, std::string* data) {
    data->append((char*)ptr, size * nmemb);
    return size * nmemb;
}

std::size_t writeData(void *ptr, std::size_t size, std::size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}


std::string cdragon::web::Downloader::downloadString(std::string url) {
    CURLcode res;
    std::string responseString;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeString);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cout << "CURL ERROR: " << url << std::endl;
            std::cout << "CURL ERROR: " << curl_easy_strerror(res) << std::endl;
        }

        return responseString;
    }

    throw std::exception("Failed to read from url!");
}


bool cdragon::web::Downloader::downloadFile(std::string url, std::filesystem::path output) {
    CURLcode res;
    FILE* fp;
    bool status = true;

    std::filesystem::create_directories(output.parent_path());
    errno_t err = fopen_s(&fp, output.string().c_str(), "wb");
    if (err != 0) {
        char errmsg[256];
        strerror_s(errmsg, 256, err);
        std::cout << "FILE ERROR: Path: " << output.string() << std::endl;
        std::cout << "FILE ERROR: Message: " << errmsg << std::endl;
        return false;
    }

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeData);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cout << "CURL ERROR: " << url << std::endl;
            std::cout << "CURL ERROR: " << curl_easy_strerror(res) << std::endl;
            status = false;
        }

        fclose(fp);
        return status;
    }

    throw std::exception("Failed to read from url!");
}
