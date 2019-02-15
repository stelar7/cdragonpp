#include "../../libs/curl/include/curl.h"
#include <iostream>

namespace cdragon {
    namespace web {

        size_t writeString(void *ptr, size_t size, size_t nmemb, std::string* data) {
            data->append((char*)ptr, size * nmemb);
            return size * nmemb;
        }

        size_t writeData(void *ptr, size_t size, size_t nmemb, FILE *stream) {
            size_t written = fwrite(ptr, size, nmemb, stream);
            return written;
        }

        std::string downloadString(std::string url) {
            CURL* curl;
            CURLcode res;
            std::string responseString;

            curl = curl_easy_init();
            if (curl) {
                curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeString);

                res = curl_easy_perform(curl);
                if (res != CURLE_OK) {
                    std::cout << curl_easy_strerror(res) << std::endl;
                }

                curl_easy_cleanup(curl);
                return responseString;
            }

            throw std::exception("Failed to read from url!");
        }

        bool downloadFile(std::string url, std::string output) {
            CURL* curl;
            CURLcode res;
            FILE* fp;
            bool status = true;

            curl = curl_easy_init();
            if (curl) {
                fopen_s(&fp, output.c_str(), "wb");

                curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeData);

                res = curl_easy_perform(curl);
                if (res != CURLE_OK) {
                    std::cout << curl_easy_strerror(res) << std::endl;
                    status = false;
                }

                curl_easy_cleanup(curl);
                fclose(fp);

                return status;
            }

            throw std::exception("Failed to read from url!");
        }
    }
}