//qr_code.cpp
#include "QRproject.h"
const int font_scale = 1;
const int font_thickness = 2;
// 사각형의 크기를 화면에 표시하는 함수
void putSizeText(Mat& img, double area) {
    String size_label;
    const int x = 50;//텍스트가 표시될 x좌표
    const int y = img.rows - 50;//텍스트가 표시될 y좌표
    const int area_cut_off = 25000;//면적의 최소치
    const int pixel_size_S = 80000; // S size 구분
    const int pixel_size_M = 110000; // M size 구분
    const int pixel_size_L = 150000; // L size 구분
    
    Scalar color(0, 255, 0); // 기본 텍스트 색상

    if (area < area_cut_off) {
        return; // 너무 작은 면적의 경우 텍스트를 출력하지 않음
    }
    else if (area < pixel_size_S) {
        size_label = "[ S ] size";
    }
    else if (area < pixel_size_M) {
        size_label = "[ M ] size";
    }
    else if (area < pixel_size_L) {
        size_label = "[ L ]  size";
    }
    else {
        size_label = "[ XL ] size";
    }

    putText(img, size_label, Point(x, y), FONT_HERSHEY_COMPLEX, font_scale, color, font_thickness);
}
//새 객체가 리스트에 있는지 확인하는 함수
bool notInList(Point new_object, const vector<Point>& detected_objects) {
    const double distance = 20.0; //기존 객체와의 거리
    for (const auto& obj : detected_objects) {
        float a = obj.x - new_object.x;
        float b = obj.y - new_object.y;
        if (sqrt(a * a + b * b) < distance)
            return false;
    }
    return true;
}

// 데이터베이스 초기화 함수
void initializeDatabase(sql::Connection*& con, sql::mysql::MySQL_Driver*& driver) {
    try {
        driver = sql::mysql::get_mysql_driver_instance(); // driver에 mysql 연결 드라이버 인스턴스 가져오기
        con = driver->connect("tcp://127.0.0.1:3306", "root", "0709"); //데이터베이스에 연결 설정
        con->setSchema("qrdata"); // qrdata라는 스키마 설정
    }
    catch (sql::SQLException& e) {
        cerr << "SQL 에러 : " << e.what() << endl;
    }
}

// 컨투어 감지 및 화면에 표시하는 함수
void detectAndDisplayContours(Mat& frame, Mat& gray, Mat& binary, vector<Point>& detected_objects) {
    const int total_pixels = binary.rows * binary.cols;//설정할 픽셀
    const double max_percent = 0.9;//화면 전체의 90퍼
    Mat img_contours = frame.clone();
    const int pixel_size_cut_off = 25000;
    vector<vector<Point>> contours;
    
    cvtColor(frame, gray, COLOR_BGR2GRAY);
    threshold(gray, binary, 100, 255, THRESH_BINARY);
    Mat kernel = getStructuringElement(MORPH_RECT, Size(7, 7));
    morphologyEx(binary, binary, MORPH_CLOSE, kernel);

   
    findContours(binary, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);


    for (const auto& contour : contours) {
        double area = contourArea(contour);
        if (area > pixel_size_cut_off && area < (total_pixels * max_percent)) {
            drawContours(img_contours, contours, static_cast<int>(&contour - &contours[0]), Scalar(0, 0, 255), 2);
            putSizeText(img_contours, area);
        }
    }

    imshow("Detected Contours", img_contours);
}

// QR 코드 감지 함수
void detectQRCode(Mat& frame, QRCodeDetector& detector, set<string>& detected_QRs, set<string>& printed_QRs, sql::Connection* con, sql::PreparedStatement*& pstmt) {
    const string print_table_1 = "sender_information";//출력할 mysql 테이블


    const string print_table_2 = "recipient_information";
    const string print_table_3 = "recipient_address";
    const double thickness = 2;
    vector<Point> points;
    String info = detector.detectAndDecode(frame, points);
    if (!info.empty() && detected_QRs.find(info) == detected_QRs.end()) {
        detected_QRs.insert(info);
        polylines(frame, points, true, Scalar(0, 0, 255), thickness);
        Beep(750, 1000);

        if (printed_QRs.find(info) == printed_QRs.end()) {
            printed_QRs.insert(info);
            pstmt = con->prepareStatement("SELECT * FROM qr WHERE qr_code_id = ?");
            pstmt->setString(1, info);
            sql::ResultSet* res = pstmt->executeQuery();
            if (res->next()) {
                string sender_info = res->getString(print_table_1);
                string recipient_info = res->getString(print_table_2);
                string recipient_add = res->getString(print_table_3);

                wstring_convert<codecvt_utf8_utf16<wchar_t>> converter;
                wstring wide_sender_info = converter.from_bytes(sender_info);
                wstring wide_reci_info = converter.from_bytes(recipient_info);
                wstring wide_reci_add = converter.from_bytes(recipient_add);
                wcout.imbue(locale(""));
                wcout << L"/////////////////////////////////////////////////////" << endl;
                wcout << L"보내는 사람 : " << wide_sender_info << endl;
                wcout << L"받는 사람 : " << wide_reci_info << endl;
                wcout << L"주소 : " << wide_reci_add << endl;
                wcout << endl;
            }
            delete res;
        }
    }
}

// 템플릿 매칭 함수
void performTemplateMatching(Mat& gray, Mat& img_contours, vector<Point>& detectedObjects, bool& display_text, time_point<steady_clock>& start_time) {
    vector<Mat> images;
    const int img_size = images.size();
    const double threshold_area = 0.8;
    const double thickness_rectangle = 0.5;
    vector<string> image_paths = {
        "as1.png", "as1_1.png", "as2.png", "as2_1.png", "as3.png", "as3_1.png"
    };

   
    for (const auto& path : image_paths) {
        Mat frame_temp = imread(path, IMREAD_GRAYSCALE);
        if (!frame_temp.empty()) images.push_back(frame_temp);
    }

    vector<Mat> result(img_size);
    for (size_t i = 0; i < img_size; ++i) {
        matchTemplate(gray, images[i], result[i], TM_CCOEFF_NORMED);
        int temp_w = images[i].cols;
        int temp_h = images[i].rows;

        for (int x = 0; x < result[i].cols; ++x) {
            for (int y = 0; y < result[i].rows; ++y) {
                if (result[i].at<float>(y, x) >= threshold_area && notInList(Point(x, y), detectedObjects)) {
                    detectedObjects.push_back(Point(x, y));
                    rectangle(img_contours, Point(x, y), Point(x + temp_w, y + temp_h), Scalar(255, 0, 255), thickness_rectangle);
                    display_text = true;
                    start_time = steady_clock::now();
                }
            }
        }
    }
}

// 경고 텍스트 표시 함수
void showWarning(Mat& img_contours, bool& display_text, time_point<steady_clock>& start_time) {
    string text = "danger!";
    const int x = img_contours.cols - 200;
    const int y = 30;
    if (display_text) {
        auto current_time = steady_clock::now();
        auto elapsed_seconds = duration_cast<seconds>(current_time - start_time).count();

        if (elapsed_seconds < 1) {
            
            putText(img_contours, text, Point(x, y), FONT_HERSHEY_COMPLEX, font_scale, Scalar(0, 165, 255), font_thickness, LINE_AA);
        }
        else {
            display_text = false;
        }
    }
}

// 영상 처리 메인 함수
void processVideo() {
    utils::logging::setLogLevel(utils::logging::LOG_LEVEL_ERROR);
    const int ESC = 27;
    const int set_width_value = 640;
    const int set_height_value = 480;
    QRCodeDetector detector;
    set<string> detected_QRs;
    set<string> printed_QRs;
    sql::mysql::MySQL_Driver* driver;
    sql::Connection* con;
    sql::PreparedStatement* pstmt;
    vector<Point> detected_objects;


    VideoCapture cap(0);
    if (!cap.isOpened()) {
        cerr << "Camera open failed!" << endl;
        return;
    }

    cap.set(CAP_PROP_FRAME_WIDTH, set_width_value);
    cap.set(CAP_PROP_FRAME_HEIGHT, set_height_value);

   

    initializeDatabase(con, driver);

   
    auto start_time = steady_clock::now();
    bool display_text = false;

    Mat frame, gray, binary;
    while (true) {
        cap >> frame;
        if (frame.empty()) {
            cerr << "Frame load failed!" << endl;
            break;
        }

        detectAndDisplayContours(frame, gray, binary, detected_objects);
        detectQRCode(frame, detector, detected_QRs, printed_QRs, con, pstmt);
        performTemplateMatching(gray, frame, detected_objects, display_text, start_time);
        showWarning(frame, display_text, start_time);

        imshow("Binary Image", binary);
        if (waitKey(1) == ESC) {
            break;
        }
    }

    cap.release();
    destroyAllWindows();
    delete pstmt;
    delete con;
    
}
