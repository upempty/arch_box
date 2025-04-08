#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <condition_variable>
#include <chrono>
#include <csignal>
#include <string>
#include <fstream>
#include <ctime>
#include <iomanip>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
}

#define ERROR_STR(errnum) \
    char errbuf[AV_ERROR_MAX_STRING_SIZE]; \
    av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, errnum);

using namespace std::chrono;

enum LogLevel {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_NO
};

class Logger {
private:
    std::ofstream log_file_;
    std::mutex log_mutex_;
    LogLevel min_level_ = LOG_INFO;
    bool console_output_ = true;
    bool file_output_ = false;

    const char* levelToString(LogLevel level) {
        switch(level) {
            case LOG_DEBUG: return "DEBUG";
            case LOG_INFO: return "INFO";
            case LOG_WARNING: return "WARNING";
            case LOG_ERROR: return "ERROR";
            case LOG_NO: return "NO";
            default: return "UNKNOWN";
        }
    }

    std::string getCurrentTime() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
        return ss.str();
    }

public:
    Logger() = default;
    ~Logger() {
        if (log_file_.is_open()) {
            log_file_.close();
        }
    }

    void init(const std::string& filename, LogLevel level = LOG_INFO, bool console = true) {
        min_level_ = level;
        console_output_ = console;
        
        if (!filename.empty()) {
            log_file_.open(filename, std::ios::app);
            if (log_file_.is_open()) {
                file_output_ = true;
                log(LOG_INFO, "Logger initialized");
            } else {
                std::cerr << "Failed to open log file: " << filename << std::endl;
            }
        }
    }

    void log(LogLevel level, const std::string& message) {
        if (level < min_level_) return;

        std::lock_guard<std::mutex> lock(log_mutex_);
        std::string formatted = "[" + getCurrentTime() + "] [" + levelToString(level) + "] " + message;

        if (console_output_) {
            if (level >= LOG_WARNING) {
                std::cerr << formatted << std::endl;
            } else {
                std::cout << formatted << std::endl;
            }
        }

        if (file_output_ && log_file_.is_open()) {
            log_file_ << formatted << std::endl;
        }
    }
};

Logger g_logger;

// Parameters to be configured 
struct Config {
    std::string input_url;
    std::string output_url;
    bool enable_filter = false;
    int input_fps = 15;
    int output_fps = 30;
    std::string video_size = "1280x1024";
    std::string log_file = "streamer.log";
    LogLevel log_level = LOG_INFO;
    bool console_log = true;
};

class VideoStreamer {
public:
    VideoStreamer(const Config& config) : config_(config) {
        g_logger.init(config_.log_file, config_.log_level, config_.console_log);
    }
    ~VideoStreamer() { cleanup(); }

    int init() {
        g_logger.log(LOG_INFO, "Initializing video streamer...");
        
        while (!init_input()) {
            if (should_stop_) return -1;
            g_logger.log(LOG_WARNING, "Input initialization failed, retrying in 5 seconds...");
            std::this_thread::sleep_for(5s); // Retry
        }

        if (config_.enable_filter) {
            if (init_framerate_filter() < 0) return -1;
        }

        if (init_encoder() < 0) return -1;

        while (!init_output()) {
            if (should_stop_) return -1;
            g_logger.log(LOG_WARNING, "Output initialization failed, retrying in 5 seconds...");
            std::this_thread::sleep_for(5s); // Retry
        }

        g_logger.log(LOG_INFO, "Video streamer initialized successfully");
        return 0;
    }

    void run() {
        g_logger.log(LOG_INFO, "Starting video streamer threads...");
        std::thread capture_thread(&VideoStreamer::capture_loop, this);
        std::thread encode_thread(&VideoStreamer::encode_loop, this);

        capture_thread.join();
        encode_thread.join();
        
        g_logger.log(LOG_INFO, "Video streamer threads stopped");
    }

    void stop() {
        g_logger.log(LOG_INFO, "Stopping video streamer...");
        should_stop_ = true;
        frame_queue_.wake_and_quit();
    }

private:
    const Config config_;
    std::atomic<bool> should_stop_{false};

    AVFormatContext* input_ctx_ = nullptr;
    AVCodecContext* encoder_ctx_ = nullptr;
    AVFormatContext* output_ctx_ = nullptr;
    int video_stream_index_ = -1;
    int64_t frame_count_ = 0;

    AVFilterGraph* filter_graph_ = nullptr;
    AVFilterContext* buffersrc_ctx_ = nullptr;
    AVFilterContext* buffersink_ctx_ = nullptr;

    struct FrameQueue {
        std::queue<AVFrame*> queue;
        std::mutex mtx;
        std::condition_variable cond;
        std::atomic<bool> quit{false};

        void push(AVFrame* frame) {
            std::lock_guard<std::mutex> lock(mtx);
            queue.push(frame);
            cond.notify_one();
        }

        AVFrame* pop() {
            std::unique_lock<std::mutex> lock(mtx);
            while (queue.empty() && !quit) {
                cond.wait(lock);
            }
            if (quit) return nullptr;
            AVFrame* frame = queue.front();
            queue.pop();
            return frame;
        }

        void wake_and_quit() {
            quit = true;
            cond.notify_all();
        }
    } frame_queue_;

    bool is_rtsp_source() const {
        return config_.input_url.find("rtsp://") == 0;
    }

    int init_input() {
        AVDictionary* options = nullptr;
        av_dict_set(&options, "framerate", std::to_string(config_.input_fps).c_str(), 0);
        av_dict_set(&options, "video_size", config_.video_size.c_str(), 0);
        
        if (!is_rtsp_source()) {
            av_dict_set(&options, "input_format", "yuv420p", 0);
        }

        if (is_rtsp_source()) {
            av_dict_set(&options, "rtsp_transport", "tcp", 0);
            av_dict_set(&options, "stimeout", "5000000", 0); 
        }

        const AVInputFormat* input_format = is_rtsp_source() ? nullptr : av_find_input_format("v4l2");
        
        int ret = avformat_open_input(&input_ctx_, config_.input_url.c_str(), input_format, &options);
        if (ret < 0) {
            ERROR_STR(ret);
            g_logger.log(LOG_ERROR, std::string("Failed to open input ") + config_.input_url + ": " + errbuf);
            av_dict_free(&options);
            return false;
        }

        ret = avformat_find_stream_info(input_ctx_, nullptr);
        if (ret < 0) {
            ERROR_STR(ret);
            g_logger.log(LOG_ERROR, std::string("Failed to find stream info: ") + errbuf);
            return false;
        }

        for (unsigned i = 0; i < input_ctx_->nb_streams; i++) {
            if (input_ctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                video_stream_index_ = i;
                AVPixelFormat input_fmt = static_cast<AVPixelFormat>(input_ctx_->streams[i]->codecpar->format);
                g_logger.log(LOG_INFO, std::string("Input source: ") + config_.input_url + 
                         " | Format: " + av_get_pix_fmt_name(input_fmt) +
                         " | Resolution: " + config_.video_size);
                break;
            }
        }

        g_logger.log(LOG_INFO, "Input initialized successfully");
        return true;
    }

    int init_framerate_filter() {
        AVStream* stream = input_ctx_->streams[video_stream_index_];
        const AVFilter* buffersrc = avfilter_get_by_name("buffer");
        const AVFilter* buffersink = avfilter_get_by_name("buffersink");
        filter_graph_ = avfilter_graph_alloc();

        std::string args = "video_size=" + std::to_string(stream->codecpar->width) + "x" + 
                          std::to_string(stream->codecpar->height) + 
                          ":pix_fmt=" + std::to_string(stream->codecpar->format) + 
                          ":time_base=1/" + std::to_string(config_.input_fps) + 
                          ":pixel_aspect=1/1";

        int ret = avfilter_graph_create_filter(&buffersrc_ctx_, buffersrc, "in", 
                                             args.c_str(), nullptr, filter_graph_);
        if (ret < 0) {
            ERROR_STR(ret);
            g_logger.log(LOG_ERROR, "Failed to create buffer source");
            return ret;
        }

        ret = avfilter_graph_create_filter(&buffersink_ctx_, buffersink, "out", 
                                         nullptr, nullptr, filter_graph_);
        if (ret < 0) {
            ERROR_STR(ret);
            g_logger.log(LOG_ERROR, "Failed to create buffer sink");
            return ret;
        }

        AVFilterContext* fps_ctx;
        const AVFilter* fps_filter = avfilter_get_by_name("fps");
        std::string fps_args = "fps=" + std::to_string(config_.output_fps);

        ret = avfilter_graph_create_filter(&fps_ctx, fps_filter, "fps", 
                                         fps_args.c_str(), nullptr, filter_graph_);
        if (ret < 0) {
            ERROR_STR(ret);
            g_logger.log(LOG_ERROR, "Failed to create fps filter");
            return ret;
        }

        // 0408
        AVFilterContext* hflip_ctx;
        const AVFilter* hflip_filter = avfilter_get_by_name("hflip");
        ret = avfilter_graph_create_filter(&hflip_ctx, hflip_filter, "hflip", nullptr, nullptr, filter_graph_);
        if (ret < 0) {
            ERROR_STR(ret);
            g_logger.log(LOG_ERROR, "Failed to create hflip filter");
            return ret;
        }

        // 


        ret = avfilter_link(buffersrc_ctx_, 0, fps_ctx, 0);
        if (ret < 0) {
            ERROR_STR(ret);
            g_logger.log(LOG_ERROR, "Failed to link filters src to fps");
            return ret;
        }


	// 0408
        ret = avfilter_link(fps_ctx, 0, hflip_ctx, 0);
        if (ret < 0) {
            ERROR_STR(ret);
            g_logger.log(LOG_ERROR, "Failed to link filters fps to hflip");
            return ret;
        }



        ret = avfilter_link(hflip_ctx, 0, buffersink_ctx_, 0);

        //ret = avfilter_link(fps_ctx, 0, buffersink_ctx_, 0);
        if (ret < 0) {
            ERROR_STR(ret);
            g_logger.log(LOG_ERROR, "Failed to link filters hflip to sink");
            return ret;
        }

        ret = avfilter_graph_config(filter_graph_, nullptr);
        if (ret < 0) {
            ERROR_STR(ret);
            g_logger.log(LOG_ERROR, "Failed to configure filter graph");
            return ret;
        }

        g_logger.log(LOG_INFO, std::string("Framerate filter initialized (") + 
                  std::to_string(config_.input_fps) + "->" + std::to_string(config_.output_fps) + "fps)");
        return 0;
    }

    int init_encoder() {
        const AVCodec* codec = avcodec_find_encoder_by_name("h264_rkmpp");
        if (!codec) {
            g_logger.log(LOG_ERROR, "Failed to find h264_rkmpp encoder");
            return AVERROR(ENOSYS);
        }

        encoder_ctx_ = avcodec_alloc_context3(codec);
        if (!encoder_ctx_) {
            g_logger.log(LOG_ERROR, "Failed to allocate encoder context");
            return AVERROR(ENOMEM);
        }

        AVStream* in_stream = input_ctx_->streams[video_stream_index_];
        encoder_ctx_->width = in_stream->codecpar->width;
        encoder_ctx_->height = in_stream->codecpar->height;
        encoder_ctx_->time_base = {1, config_.output_fps};
        encoder_ctx_->framerate = {config_.output_fps, 1};
        encoder_ctx_->pix_fmt = static_cast<AVPixelFormat>(in_stream->codecpar->format);
        encoder_ctx_->gop_size = config_.output_fps;
        encoder_ctx_->max_b_frames = 0;
        av_opt_set(encoder_ctx_->priv_data, "preset", "fast", 0);

        int ret = avcodec_open2(encoder_ctx_, codec, nullptr);
        if (ret < 0) {
            ERROR_STR(ret);
            g_logger.log(LOG_ERROR, std::string("Failed to open encoder: ") + errbuf);
            return ret;
        }

        g_logger.log(LOG_INFO, "Encoder initialized successfully");
        return 0;
    }

    bool init_output() {
        int ret = avformat_alloc_output_context2(&output_ctx_, nullptr, "rtsp", config_.output_url.c_str());
        if (!output_ctx_) {
            g_logger.log(LOG_ERROR, "Failed to create output context");
            return false;
        }

        AVStream* stream = avformat_new_stream(output_ctx_, nullptr);
        if (!stream) {
            g_logger.log(LOG_ERROR, "Failed to create output stream");
            return false;
        }

        avcodec_parameters_from_context(stream->codecpar, encoder_ctx_);
        stream->time_base = encoder_ctx_->time_base;

        if (!(output_ctx_->oformat->flags & AVFMT_NOFILE)) {
            ret = avio_open(&output_ctx_->pb, output_ctx_->url, AVIO_FLAG_WRITE);
            if (ret < 0) {
                ERROR_STR(ret);
                g_logger.log(LOG_ERROR, std::string("Failed to open output IO: ") + errbuf);
                return false;
            }
        }

        ret = avformat_write_header(output_ctx_, nullptr);
        if (ret < 0) {
            ERROR_STR(ret);
            g_logger.log(LOG_ERROR, std::string("Failed to write header: ") + errbuf);
            return false;
        }

        g_logger.log(LOG_INFO, std::string("Output initialized to ") + config_.output_url);
        return true;
    }

    void capture_loop() {
        AVPacket* packet = av_packet_alloc();
        AVFrame* frame = av_frame_alloc();
        AVFrame* filtered_frame = av_frame_alloc();

        g_logger.log(LOG_INFO, "Capture thread started");

        while (!should_stop_) {
            auto capture_start = high_resolution_clock::now();
            int ret = av_read_frame(input_ctx_, packet);
            
            if (ret == AVERROR(EAGAIN)) {
                continue;
            }
            if (ret < 0) {
                ERROR_STR(ret);
                g_logger.log(LOG_ERROR, "Input error, reconnecting...");
                avformat_close_input(&input_ctx_);
                while (!init_input() && !should_stop_) {
                    std::this_thread::sleep_for(5s);
                }
                continue;
            }

            if (packet->stream_index == video_stream_index_) {
                AVStream* stream = input_ctx_->streams[video_stream_index_];
                
                frame->width = stream->codecpar->width;
                frame->height = stream->codecpar->height;
                frame->format = stream->codecpar->format;
                
                if (av_image_fill_arrays(frame->data, frame->linesize,
                                       packet->data,
                                       static_cast<AVPixelFormat>(frame->format),
                                       frame->width, frame->height, 1) < 0) {
                    g_logger.log(LOG_ERROR, "Failed to fill image arrays");
                    continue;
                }

                frame->pts = frame_count_++;
                auto capture_us = duration_cast<microseconds>(
                    high_resolution_clock::now() - capture_start).count();
                
                g_logger.log(LOG_DEBUG, std::string("Captured frame PTS: ") + std::to_string(frame->pts) + 
                          " | Capture time: " + std::to_string(capture_us) + "us");

                if (config_.enable_filter) {
                    g_logger.log(LOG_DEBUG, "Processing frame with filter");
                    process_with_filter(frame, filtered_frame);
                } else {
                    AVFrame* new_frame = av_frame_alloc();
                    av_frame_ref(new_frame, frame);
                    frame_queue_.push(new_frame);
                }
            }
            av_packet_unref(packet);
        }

        av_packet_free(&packet);
        av_frame_free(&frame);
        av_frame_free(&filtered_frame);
        frame_queue_.wake_and_quit();
        
        g_logger.log(LOG_INFO, "Capture thread stopped");
    }

    void process_with_filter(AVFrame* frame, AVFrame* filtered_frame) {
        auto filter_start = high_resolution_clock::now();
        
        if (av_buffersrc_add_frame(buffersrc_ctx_, frame) < 0) {
            g_logger.log(LOG_ERROR, "Error feeding frame to filter");
            return;
        }

        while (true) {
            int ret = av_buffersink_get_frame(buffersink_ctx_, filtered_frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
            
            if (ret < 0) {
                ERROR_STR(ret);
                g_logger.log(LOG_ERROR, "Error getting filtered frame");
                break;
            }

            auto filter_us = duration_cast<microseconds>(
                high_resolution_clock::now() - filter_start).count();
            
            g_logger.log(LOG_DEBUG, std::string("Filtered frame PTS: ") + std::to_string(filtered_frame->pts) +
                      " | Filter time: " + std::to_string(filter_us) + "us");

            AVFrame* new_frame = av_frame_alloc();
            av_frame_ref(new_frame, filtered_frame);
            frame_queue_.push(new_frame);
            av_frame_unref(filtered_frame);
        }
    }

    void encode_loop() {
        AVPacket* pkt = av_packet_alloc();
        g_logger.log(LOG_INFO, "Encode thread started");

        while (!should_stop_) {
            AVFrame* frame = frame_queue_.pop();
            if (!frame) continue;

            auto encode_start = high_resolution_clock::now();
            int ret = avcodec_send_frame(encoder_ctx_, frame);
            
            if (ret == AVERROR(EAGAIN)) {
                av_frame_free(&frame);
                continue;
            }
            if (ret < 0) {
                ERROR_STR(ret);
                g_logger.log(LOG_ERROR, std::string("Error sending frame: ") + errbuf);
                av_frame_free(&frame);
                continue;
            }

            while (true) {
                ret = avcodec_receive_packet(encoder_ctx_, pkt);
                if (ret == AVERROR(EAGAIN)) break;
                if (ret == AVERROR_EOF) break;
                if (ret < 0) {
                    ERROR_STR(ret);
                    g_logger.log(LOG_ERROR, std::string("Error encoding frame: ") + errbuf);
                    break;
                }

                pkt->stream_index = 0;
                av_packet_rescale_ts(pkt, encoder_ctx_->time_base,
                                   output_ctx_->streams[0]->time_base);

                auto encoded_us = duration_cast<microseconds>(
                    high_resolution_clock::now() - encode_start).count();
                
                g_logger.log(LOG_DEBUG, std::string("Encoded packet PTS: ") + std::to_string(pkt->pts) +
                          " | Encode time: " + std::to_string(encoded_us) + "us");

                auto send_start = high_resolution_clock::now();
                ret = av_interleaved_write_frame(output_ctx_, pkt);
                auto send_us = duration_cast<microseconds>(
                    high_resolution_clock::now() - send_start).count();

                if (ret < 0) {
                    ERROR_STR(ret);
                    g_logger.log(LOG_ERROR, std::string("Error writing packet: ") + errbuf);
                    // Retry
                    avformat_free_context(output_ctx_);
                    output_ctx_ = nullptr;
                    while (!init_output() && !should_stop_) {
                        std::this_thread::sleep_for(5s);
                    }
                    break;
                }
                
                g_logger.log(LOG_DEBUG, std::string("Sent packet PTS: ") + std::to_string(pkt->pts) +
                          " | Send time: " + std::to_string(send_us) + "us");
                
                av_packet_unref(pkt);
            }
            av_frame_free(&frame);
        }

        av_packet_free(&pkt);
        g_logger.log(LOG_INFO, "Encode thread stopped");
    }

    void cleanup() {
        g_logger.log(LOG_INFO, "Cleaning up resources...");
        if (encoder_ctx_) avcodec_free_context(&encoder_ctx_);
        if (input_ctx_) avformat_close_input(&input_ctx_);
        if (output_ctx_) {
            if (output_ctx_->pb) avio_closep(&output_ctx_->pb);
            avformat_free_context(output_ctx_);
        }
        if (filter_graph_) avfilter_graph_free(&filter_graph_);
    }
};

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input_url> <output_url>" << std::endl;
        std::cerr << "Example: " << argv[0] << " /dev/video0 rtsp://192.168.1.86:8554/live2" << std::endl;
        return 1;
    }

    avdevice_register_all();
    avformat_network_init();

    Config config;
    config.input_url = argv[1];
    config.output_url = argv[2];
    config.enable_filter = true; // to use filter 
    config.log_file = "streamer.log";
    //config.log_level = LOG_NO;
    config.log_level = LOG_INFO;
    config.console_log = true;

    if (argc > 3) {
        config.log_file = argv[3];
    }

    VideoStreamer streamer(config);
    if (streamer.init() < 0) {
        return 1;
    }

    // ctrl+C signal handling
    signal(SIGINT, [](int) { exit(0); });

    streamer.run();
    return 0;
}
