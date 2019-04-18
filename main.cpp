#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <cassert>
#include <netinet/in.h>
#include <stdint.h>
#include <cstring>
#include <array>
#include <deque>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <arpa/inet.h>

int main() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    assert(sockfd >= 0);
    sockaddr_in addr{};
    memset((char*)&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1339);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    assert(bind(sockfd, (sockaddr*)&addr, sizeof(addr)) >= 0);

    ip_mreq mreq{};
    mreq.imr_multiaddr.s_addr = inet_addr("239.1.3.38");
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    assert(setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) >= 0);

    sockaddr_in clientAddr{};
    socklen_t clientLen = 0;

    cv::namedWindow("imgOriginal", cv::WINDOW_NORMAL);
    cv::namedWindow("background", cv::WINDOW_NORMAL);
    cv::namedWindow("foreground", cv::WINDOW_NORMAL);
    cv::namedWindow("foregroundBin", cv::WINDOW_NORMAL);

    constexpr auto medianLength = 31;
    std::array<std::array<std::deque<uint8_t>, 8>, 8> temporalMedianFilterList;

    while (true) {
        char buf[64] = {0};
        ssize_t read = recvfrom(sockfd, buf, sizeof(buf), 0, (sockaddr*)&clientAddr, &clientLen);
        assert(read == 64);
        std::cout << "New frame" << std::endl;
        cv::Mat imgOriginal(8,8, CV_8UC1, buf);

        // Temporal median filtering
        cv::Mat background(imgOriginal.size(),CV_8UC1);
        for (auto x = 0; x < imgOriginal.cols; x++) {
            for (auto y = 0; y < imgOriginal.rows; y++) {
                temporalMedianFilterList[x][y].push_front(imgOriginal.at<uchar>(y,x));
                if(temporalMedianFilterList[x][y].size() > medianLength) {
                    temporalMedianFilterList[x][y].pop_back();
                }

                std::deque<uint8_t> listCopySorted(temporalMedianFilterList[x][y]);
                std::sort(listCopySorted.begin(), listCopySorted.end());
                background.at<uint8_t>(y,x) = listCopySorted[listCopySorted.size() / 2];
            }
        }
        cv::Mat foreground = imgOriginal - background; //@TODO we assume that the foreground is always warmer than the background

        cv::resize(imgOriginal, imgOriginal, cv::Size(128,128),0,0,CV_INTER_LANCZOS4);
        cv::resize(foreground, foreground, cv::Size(128,128),0,0,CV_INTER_LANCZOS4);
        cv::resize(background, background, cv::Size(128,128),0,0,CV_INTER_LANCZOS4);
        cv::Mat foregroundBin(foreground.size(), CV_8UC1);
        cv::threshold(foreground, foregroundBin, 10, 255, CV_THRESH_TOZERO);
        cv::equalizeHist(foregroundBin, foregroundBin);

        cv::imshow("imgOriginal", imgOriginal);
        cv::imshow("foreground", foreground);
        cv::imshow("background", background);
        cv::imshow("foregroundBin", foregroundBin);

        if(cv::waitKey(1) == 27) {
            break;
        }
    }

    return 0;
}