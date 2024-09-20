#pragma once
//QRproject.h
#ifndef HEADER_H
#define HEADER_H

#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <locale>
#include <codecvt>
#include <set>
#include <chrono>
#include <windows.h>
#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/exception.h>

using namespace cv;
using namespace std;
using namespace chrono;

void putSizeText(Mat& img, double area);
void initializeDatabase(sql::Connection*& con, sql::mysql::MySQL_Driver*& driver);
void detectAndDisplayContours(Mat& frame, Mat& gray, Mat& binary, vector<Point>& detectedObjects);
void detectQRCode(Mat& frame, QRCodeDetector& detector, set<string>& detectedQRs, set<string>& printedQRs, sql::Connection* con, sql::PreparedStatement*& pstmt);
void performTemplateMatching(Mat& gray, Mat& img_contours, vector<Point>& detectedObjects, bool& displayText, time_point<steady_clock>& start_time);
void showWarning(Mat& img_contours, bool& displayText, time_point<steady_clock>& start_time);
bool notInList(Point newObject, const vector<Point>& detectedObjects);
void processVideo();

#endif
