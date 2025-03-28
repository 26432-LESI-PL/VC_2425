#include <opencv2/opencv.hpp> // Main C++ header - includes most things
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string>

// --- CORRECTION ---
// Include C headers needed by vc.h *before* the extern "C" block
#include <opencv2/core/core_c.h> // Needed for IplImage definition used in vc.h

// extern "C" block now only includes the header for your C library
extern "C" {
#include "lib/vc.h" // This header correctly uses extern "C" internally
                    // for its function declarations.
}

// Helper function for Mat -> IplImage conversion (use cautiously)
// Note: OpenCV 4 deprecated direct cvIplImage. We can make a wrapper or use the internal struct.
// This creates an IplImage header pointing to the Mat's data *without* copying.
// The IplImage's lifetime is tied to the Mat's data.
static IplImage cvMatToIplImage(cv::Mat& mat) {
    IplImage img = cvIplImage(mat);
    return img;
}


int main(void)
{
    std::string videofile; // Use std::string for filenames
    int escolha = 0;
    while (escolha == 0) {
        printf("Video: (1-4)\n");
        // Add error checking for scanf
        if (scanf("%d", &escolha) != 1) {
            printf("Invalid input. Please enter a number.\n");
            // Clear input buffer
            while (getchar() != '\n');
            escolha = 0; // Reset choice to loop again
            continue;
        }

        if (escolha == 1) { videofile = "video1.mp4"; }
        else if (escolha == 2) { videofile = "video2.mp4"; }

        else {
            printf("Invalid choice.\n");
            escolha = 0;
        }
    }

#ifdef _WIN32
    system("cls");
#else
    system("clear"); // For Linux/macOS
#endif

    // Video using C++ API
    cv::VideoCapture capture(videofile); // Open video file in constructor

    // Check if video opened successfully
    if (!capture.isOpened()) {
        fprintf(stderr, "Erro ao abrir o ficheiro de video: %s\n", videofile.c_str());
        return 1;
    }

    // Video properties
    int width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
    int height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);
    int ntotalframes = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
    int fps = (int)capture.get(cv::CAP_PROP_FPS);
    int nframe = 0; // Will be updated in the loop

    // Image Mats
    cv::Mat frame, frame2;

    // Text parameters
    int fontFace = cv::FONT_HERSHEY_SIMPLEX;
    double fontScale = 0.6; // Adjusted scale for potentially better look
    int thickness = 1;
    int thickness_bkg = 2; // Slightly thicker for background
    cv::Scalar colorWhite = cv::Scalar(255, 255, 255);
    cv::Scalar colorBlack = cv::Scalar(0, 0, 0);
    char str[500] = { 0 };

    // Outros
    int key = 0;

    // Create window
    cv::namedWindow("VC - TP2", cv::WINDOW_AUTOSIZE);

    // Allocate memory for external C function parameters
    int *excluir = (int *)calloc(10, sizeof(int));
    int *total = (int *)calloc(9, sizeof(int));
    if (!excluir || !total) {
        fprintf(stderr, "Erro ao alocar memoria!\n");
        return 1; // Exit if allocation fails
    }

    // Structuring element for morphology
    cv::Mat morphKernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));

    while (key != 'q' && key != 27 /* ESC key */) {
        // Read frames using C++ API
        if (!capture.read(frame)) break; // Read frame1
        if (!capture.read(frame2)) {     // Read frame2 (consecutive)
            frame.release(); // Release frame1 if frame2 couldn't be read
            break;
        }

        // Check if frames are empty (redundant if break above is used, but good practice)
        if (frame.empty() || frame2.empty()) break;

        // Apply morphology using C++ API
        // Using temporary Mats for in-place operations is safer if needed,
        // but morphologyEx often handles it. Let's use separate outputs for clarity.
        cv::Mat frame_opened, frame2_opened;
        cv::morphologyEx(frame, frame_opened, cv::MORPH_OPEN, morphKernel, cv::Point(-1,-1), 5);
        cv::morphologyEx(frame2, frame2_opened, cv::MORPH_OPEN, morphKernel, cv::Point(-1,-1), 5);

        // Update frame number
        // Note: CAP_PROP_POS_FRAMES might be approximate. Incrementing manually can be more reliable.
        // nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);
        // Let's manually increment after reading pairs. Start nframe at 0.
        nframe += 2; // Since we read two frames

        // Frame processing logic
        bool shouldProcess = false;
        if (escolha == 4) {
            if (nframe > 19 && nframe < 215) shouldProcess = true;
            else if (nframe >= 215) break; // Exit loop condition
        } else if (escolha == 3) {
            if (nframe > 19 && nframe < 417) shouldProcess = true;
            else if (nframe >= 417) break; // Exit loop condition
        } else { // For choices 1 and 2
             if (nframe > 19) shouldProcess = true;
        }

        if (shouldProcess) {
            // --- Compatibility Layer for ProcessaFrame ---
            // Create IplImage headers pointing to the Mat data
            IplImage ipl_frame_opened = cvMatToIplImage(frame_opened);
            IplImage ipl_frame2_opened = cvMatToIplImage(frame2_opened);

            // Call the external C function
            ProcessaFrame(&ipl_frame_opened, &ipl_frame2_opened, excluir, total);
            // --- End Compatibility Layer ---
            // Note: Changes made by ProcessaFrame to the IplImage data *will* affect
            // frame_opened and frame2_opened Mats because they share data.
        }


        // Draw text using C++ API
        // Display info on the *original* frame before morphology for clarity, or use frame_opened
        cv::Mat displayFrame = frame; // Choose which frame to display info on

        sprintf(str, "RESOLUCAO: %dx%d", width, height);
        cv::putText(displayFrame, str, cv::Point(20, 25), fontFace, fontScale, colorBlack, thickness_bkg);
        cv::putText(displayFrame, str, cv::Point(20, 25), fontFace, fontScale, colorWhite, thickness);

        sprintf(str, "TOTAL DE FRAMES: %d", ntotalframes);
        cv::putText(displayFrame, str, cv::Point(20, 50), fontFace, fontScale, colorBlack, thickness_bkg);
        cv::putText(displayFrame, str, cv::Point(20, 50), fontFace, fontScale, colorWhite, thickness);

        sprintf(str, "FRAME RATE: %d", fps);
        cv::putText(displayFrame, str, cv::Point(20, 75), fontFace, fontScale, colorBlack, thickness_bkg);
        cv::putText(displayFrame, str, cv::Point(20, 75), fontFace, fontScale, colorWhite, thickness);

        // Display the frame number *after* processing
        sprintf(str, "N. FRAME: %d", nframe);
        cv::putText(displayFrame, str, cv::Point(20, 100), fontFace, fontScale, colorBlack, thickness_bkg);
        cv::putText(displayFrame, str, cv::Point(20, 100), fontFace, fontScale, colorWhite, thickness);

        // Display the frame
        cv::imshow("VC - TP2", displayFrame); // Or show frame_opened to see morphology result

        // Wait for key press
        key = cv::waitKey(1); // Use a small delay (e.g., 1ms) for video playback
                              // If you want to sync to FPS, calculate delay: 1000 / fps
    }

    // Calculate and print totals
    total[8] = total[0] + total[1] + total[2] + total[3] + total[4] + total[5] + total[6] + total[7];
    float tot = total[0] * 0.01f + total[1] * 0.02f + total[2] * 0.05f + total[3] * 0.10f + total[4] * 0.20f + total[5] * 0.50f + total[6] * 1.00f + total[7] * 2.00f;
    printf("\n--- Resultados ---\n");
    printf("Total de moedas:%d\n", total[8]);
    printf("Total de cada moeda:\n");
    printf("  1 centimo: %d\n", total[0]);
    printf("  2 centimos: %d\n", total[1]);
    printf("  5 centimos: %d\n", total[2]);
    printf(" 10 centimos: %d\n", total[3]);
    printf(" 20 centimos: %d\n", total[4]);
    printf(" 50 centimos: %d\n", total[5]);
    printf("  1 euro: %d\n", total[6]);
    printf("  2 euros: %d\n", total[7]);
    printf("Valor total: %.2f euros\n", tot); // Format to 2 decimal places

#ifdef _WIN32
    system("pause");
#else
    printf("Pressione Enter para continuar...");
    getchar(); // Consume potential leftover newline
    getchar(); // Wait for Enter
#endif

    // Clean up
    cv::destroyWindow("VC - TP2");
    capture.release(); // Release video capture object (good practice, though often automatic)
    free(excluir);     // Free allocated memory
    free(total);       // Free allocated memory

    return 0;
}
