#include <opencv2/opencv.hpp> // Main C++ header - includes most things
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <thread>             // For multithreading
#include <mutex>              // For safe data access
#include <queue>              // For frame queue
#include <condition_variable> // For thread synchronization
#include <atomic>             // For atomic flags

// --- CORRECTION ---
// Include C headers needed by vc.h *before* the extern "C" block
#include <opencv2/core/core_c.h> // Needed for IplImage definition used in vc.h

// extern "C" block now only includes the header for your C library
extern "C"
{
#include "lib/vc.h" // This header correctly uses extern "C" internally
                    // for its function declarations.
}

// Helper function for Mat -> IplImage conversion (use cautiously)
static IplImage cvMatToIplImage(cv::Mat &mat)
{
    IplImage img = cvIplImage(mat);
    return img;
}

// Thread-safe frame queue structure
struct FrameQueue
{
    std::queue<std::pair<cv::Mat, cv::Mat>> frames; // Pairs of consecutive frames
    std::mutex mutex;
    std::condition_variable cond_not_empty;
    std::condition_variable cond_not_full;
    size_t max_size = 30;            // Maximum queue size
    std::atomic<bool> finish{false}; // Signal to end processing
};

// Thread-safe results structure
struct Results
{
    int total[9] = {0}; // Store coin counts
    std::mutex mutex;   // Protect access to results
};

// Worker thread function to process frames
void processFramesWorker(FrameQueue *queue, Results *results, int *excluir)
{
    cv::Mat morphKernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));

    while (true)
    {
        // Get a frame pair from the queue
        std::pair<cv::Mat, cv::Mat> framePair;
        {
            std::unique_lock<std::mutex> lock(queue->mutex);
            queue->cond_not_empty.wait(lock, [queue]
                                       { return !queue->frames.empty() || queue->finish; });

            // Check if we should exit
            if (queue->frames.empty() && queue->finish)
            {
                break;
            }

            framePair = queue->frames.front();
            queue->frames.pop();
            lock.unlock();
            queue->cond_not_full.notify_one();
        }

        // Process the frames
        cv::Mat frame_opened, frame2_opened;
        cv::morphologyEx(framePair.first, frame_opened, cv::MORPH_OPEN, morphKernel, cv::Point(-1, -1), 5);
        cv::morphologyEx(framePair.second, frame2_opened, cv::MORPH_OPEN, morphKernel, cv::Point(-1, -1), 5);

        // Convert to IplImage for ProcessaFrame
        IplImage ipl_frame_opened = cvMatToIplImage(frame_opened);
        IplImage ipl_frame2_opened = cvMatToIplImage(frame2_opened);

        // Temporary array for thread-local results
        int thread_total[9] = {0};

        // Call the external C function
        ProcessaFrame(&ipl_frame_opened, &ipl_frame2_opened, excluir, thread_total);

        // Update global results
        {
            std::lock_guard<std::mutex> lock(results->mutex);
            for (int i = 0; i < 9; i++)
            {
                results->total[i] += thread_total[i];
            }
        }
    }
}

int main(void)
{
    std::string videofile; // Use std::string for filenames
    int escolha = 0;
    while (escolha == 0)
    {
        printf("Video: (1-2)\n");
        // Add error checking for scanf
        if (scanf("%d", &escolha) != 1)
        {
            printf("Invalid input. Please enter a number.\n");
            // Clear input buffer
            while (getchar() != '\n')
                ;
            escolha = 0; // Reset choice to loop again
            continue;
        }

        if (escolha == 1)
        {
            videofile = "video1.mp4";
        }
        else if (escolha == 2)
        {
            videofile = "video2.mp4";
        }
        else
        {
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
    if (!capture.isOpened())
    {
        fprintf(stderr, "Erro ao abrir o ficheiro de video: %s\n", videofile.c_str());
        return 1;
    }

    // Video properties
    int width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
    int height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);
    int ntotalframes = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
    int fps = (int)capture.get(cv::CAP_PROP_FPS);
    int nframe = 0; // Will be updated in the loop

    // Text parameters
    int fontFace = cv::FONT_HERSHEY_SIMPLEX;
    double fontScale = 0.6;
    int thickness = 1;
    int thickness_bkg = 2;
    cv::Scalar colorWhite = cv::Scalar(255, 255, 255);
    cv::Scalar colorBlack = cv::Scalar(0, 0, 0);
    char str[500] = {0};

    // Outros
    int key = 0;

    // Create window
    cv::namedWindow("VC - TP2", cv::WINDOW_AUTOSIZE);

    // Allocate memory for external C function parameters
    int *excluir = (int *)calloc(10, sizeof(int));
    if (!excluir)
    {
        fprintf(stderr, "Erro ao alocar memoria!\n");
        return 1; // Exit if allocation fails
    }

    // Set up multithreading
    unsigned int num_threads = std::thread::hardware_concurrency();
    // Use at least 2 threads but no more than 8
    num_threads = std::max(2u, std::min(8u, num_threads));
    printf("Using %u worker threads\n", num_threads);

    // Create thread-safe queue and results
    FrameQueue frameQueue;
    Results results;

    // Create worker threads
    std::vector<std::thread> workers;
    for (unsigned int i = 0; i < num_threads; i++)
    {
        workers.emplace_back(processFramesWorker, &frameQueue, &results, excluir);
    }

    while (key != 'q' && key != 27 /* ESC key */)
    {
        // Read frames using C++ API
        cv::Mat frame, frame2;
        if (!capture.read(frame))
            break;
        if (!capture.read(frame2))
        {
            frame.release();
            break;
        }

        // Check if frames are empty
        if (frame.empty() || frame2.empty())
            break;

        // Update frame number
        nframe += 2;

        // Frame processing logic
        bool shouldProcess = false;
        if (escolha == 4)
        {
            if (nframe > 19 && nframe < 215)
                shouldProcess = true;
            else if (nframe >= 215)
                break;
        }
        else if (escolha == 3)
        {
            if (nframe > 19 && nframe < 417)
                shouldProcess = true;
            else if (nframe >= 417)
                break;
        }
        else
        { // For choices 1 and 2
            if (nframe > 19)
                shouldProcess = true;
        }

        if (shouldProcess)
        {
            // Add frames to the processing queue
            std::unique_lock<std::mutex> lock(frameQueue.mutex);
            frameQueue.cond_not_full.wait(lock, [&frameQueue]
                                          { return frameQueue.frames.size() < frameQueue.max_size; });
            frameQueue.frames.push(std::make_pair(frame.clone(), frame2.clone()));
            lock.unlock();
            frameQueue.cond_not_empty.notify_one();
        }

        // Draw text using C++ API
        cv::Mat displayFrame = frame;

        sprintf(str, "RESOLUCAO: %dx%d", width, height);
        cv::putText(displayFrame, str, cv::Point(20, 25), fontFace, fontScale, colorBlack, thickness_bkg);
        cv::putText(displayFrame, str, cv::Point(20, 25), fontFace, fontScale, colorWhite, thickness);

        sprintf(str, "TOTAL DE FRAMES: %d", ntotalframes);
        cv::putText(displayFrame, str, cv::Point(20, 50), fontFace, fontScale, colorBlack, thickness_bkg);
        cv::putText(displayFrame, str, cv::Point(20, 50), fontFace, fontScale, colorWhite, thickness);

        sprintf(str, "FRAME RATE: %d", fps);
        cv::putText(displayFrame, str, cv::Point(20, 75), fontFace, fontScale, colorBlack, thickness_bkg);
        cv::putText(displayFrame, str, cv::Point(20, 75), fontFace, fontScale, colorWhite, thickness);

        sprintf(str, "N. FRAME: %d", nframe);
        cv::putText(displayFrame, str, cv::Point(20, 100), fontFace, fontScale, colorBlack, thickness_bkg);
        cv::putText(displayFrame, str, cv::Point(20, 100), fontFace, fontScale, colorWhite, thickness);

        // Display the frame
        cv::imshow("VC - TP2", displayFrame);

        // Wait for key press
        key = cv::waitKey(1);
    }

    // Signal threads to finish and wait for them
    frameQueue.finish = true;
    frameQueue.cond_not_empty.notify_all();
    for (auto &worker : workers)
    {
        worker.join();
    }

    // Calculate and print totals
    results.total[8] = results.total[0] + results.total[1] + results.total[2] + results.total[3] +
                       results.total[4] + results.total[5] + results.total[6] + results.total[7];
    float tot = results.total[0] * 0.01f + results.total[1] * 0.02f + results.total[2] * 0.05f +
                results.total[3] * 0.10f + results.total[4] * 0.20f + results.total[5] * 0.50f +
                results.total[6] * 1.00f + results.total[7] * 2.00f;

    printf("\n--- Resultados ---\n");
    printf("Total de moedas:%d\n", results.total[8]);
    printf("Total de cada moeda:\n");
    printf("  1 centimo: %d\n", results.total[0]);
    printf("  2 centimos: %d\n", results.total[1]);
    printf("  5 centimos: %d\n", results.total[2]);
    printf(" 10 centimos: %d\n", results.total[3]);
    printf(" 20 centimos: %d\n", results.total[4]);
    printf(" 50 centimos: %d\n", results.total[5]);
    printf("  1 euro: %d\n", results.total[6]);
    printf("  2 euros: %d\n", results.total[7]);
    printf("Valor total: %.2f euros\n", tot);

#ifdef _WIN32
    system("pause");
#else
    printf("Pressione Enter para continuar...");
    getchar(); // Consume potential leftover newline
    getchar(); // Wait for Enter
#endif

    // Clean up
    cv::destroyWindow("VC - TP2");
    capture.release();
    free(excluir);

    return 0;
}