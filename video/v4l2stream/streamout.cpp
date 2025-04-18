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
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/videodev2.h>
#include <sys/mman.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavutil/pixdesc.h>
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
    double input_fps = 18; // which is xpi rk3566 zero
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

    int v4l2_fd_ = -1;
    AVFormatContext* input_ctx_ = nullptr;
    AVCodecContext* encoder_ctx_ = nullptr;
    AVFormatContext* output_ctx_ = nullptr;
    int video_stream_index_ = -1;
    int64_t frame_count_ = 0;

    int64_t last_pts_ = AV_NOPTS_VALUE;

    AVFilterGraph* filter_graph_ = nullptr;
    AVFilterContext* buffersrc_ctx_ = nullptr;
    AVFilterContext* buffersink_ctx_ = nullptr;

    // V4L2 buffer info
    struct V4L2BufferInfo {
        void* start[VIDEO_MAX_PLANES] = {nullptr};
        size_t length[VIDEO_MAX_PLANES] = {0};
        int bytesperline[VIDEO_MAX_PLANES] = {0};
    };
    std::vector<V4L2BufferInfo> v4l2_buffers_;
    int v4l2_width_ = 1280;
    int v4l2_height_ = 1024;
    AVPixelFormat v4l2_pix_fmt_ = AV_PIX_FMT_NV12;

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

    int init_v4l2_device() {
        // Parse video size
        size_t delimiter = config_.video_size.find('x');
        if (delimiter != std::string::npos) {
            v4l2_width_ = std::stoi(config_.video_size.substr(0, delimiter));
            v4l2_height_ = std::stoi(config_.video_size.substr(delimiter + 1));
        }

        // Open the V4L2 device
        v4l2_fd_ = open(config_.input_url.c_str(), O_RDWR | O_NONBLOCK);
        if (v4l2_fd_ < 0) {
            g_logger.log(LOG_ERROR, "Failed to open V4L2 device: " + std::string(strerror(errno)));
            return -1;
        }

        // Query device capabilities
        struct v4l2_capability cap;
        if (ioctl(v4l2_fd_, VIDIOC_QUERYCAP, &cap) < 0) {
            g_logger.log(LOG_ERROR, "Failed to query V4L2 capabilities: " + std::string(strerror(errno)));
            close(v4l2_fd_);
            v4l2_fd_ = -1;
            return -1;
        }

        if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE)) {
            g_logger.log(LOG_ERROR, "Device does not support MPLANE capture");
            close(v4l2_fd_);
            v4l2_fd_ = -1;
            return -1;
        }

        // Set format
        struct v4l2_format fmt = {};
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        fmt.fmt.pix_mp.width = v4l2_width_;
        fmt.fmt.pix_mp.height = v4l2_height_;
        fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12;
        fmt.fmt.pix_mp.field = V4L2_FIELD_ANY;
        fmt.fmt.pix_mp.num_planes = 1;

        if (ioctl(v4l2_fd_, VIDIOC_S_FMT, &fmt) < 0) {
            g_logger.log(LOG_ERROR, "Failed to set V4L2 format: " + std::string(strerror(errno)));
            close(v4l2_fd_);
            v4l2_fd_ = -1;
            return -1;
        }

        // Get actual format
        struct v4l2_format actual_fmt = {};
        actual_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        if (ioctl(v4l2_fd_, VIDIOC_G_FMT, &actual_fmt) == 0) {
            for (int j = 0; j < actual_fmt.fmt.pix_mp.num_planes; j++) {
                g_logger.log(LOG_INFO, "Actual V4L2 format: plane " + std::to_string(j) + 
                    ", bytesperline=" + std::to_string(actual_fmt.fmt.pix_mp.plane_fmt[j].bytesperline) + 
                    ", width=" + std::to_string(actual_fmt.fmt.pix_mp.width) + 
                    ", height=" + std::to_string(actual_fmt.fmt.pix_mp.height) + 
                    ", fmt=" + std::to_string(actual_fmt.fmt.pix_mp.pixelformat));
            }
        }

        g_logger.log(LOG_INFO, "V4L2 MPlane device initialized successfully");
        return 0;
    }

    int init_v4l2_buffers() {
        // Request buffers
        struct v4l2_requestbuffers req = {};
        req.count = 4;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        req.memory = V4L2_MEMORY_MMAP;

        if (ioctl(v4l2_fd_, VIDIOC_REQBUFS, &req) < 0) {
            g_logger.log(LOG_ERROR, "Failed to request V4L2 buffers: " + std::string(strerror(errno)));
            return -1;
        }

        v4l2_buffers_.resize(req.count);

        int bytesperlinei[VIDEO_MAX_PLANES] = {0};
	    struct v4l2_format fmt2 = {0};
            fmt2.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
            if (ioctl(v4l2_fd_, VIDIOC_G_FMT, &fmt2) == 0) {
                for (int j = 0; j < fmt2.fmt.pix_mp.num_planes; j++) {
	       	    bytesperlinei[j] = fmt2.fmt.pix_mp.plane_fmt[j].bytesperline;

                }
            }


        // Map buffers and queue them
        for (unsigned int i = 0; i < req.count; ++i) {
            struct v4l2_buffer buf = {};
            struct v4l2_plane planes[VIDEO_MAX_PLANES] = {};
            
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;
            buf.length = VIDEO_MAX_PLANES;
            buf.m.planes = planes;


        
            if (ioctl(v4l2_fd_, VIDIOC_QUERYBUF, &buf) < 0) {
                g_logger.log(LOG_ERROR, "Failed to query V4L2 buffer: " + std::string(strerror(errno)));
                return -1;
            }

            // Map each plane
            for (int j = 0; j < VIDEO_MAX_PLANES; j++) {
                if (planes[j].length == 0) break;

                v4l2_buffers_[i].start[j] = mmap(NULL, planes[j].length,
                                               PROT_READ | PROT_WRITE, MAP_SHARED,
                                               v4l2_fd_, planes[j].m.mem_offset);
                if (v4l2_buffers_[i].start[j] == MAP_FAILED) {
                    g_logger.log(LOG_ERROR, "Failed to mmap V4L2 buffer: " + std::string(strerror(errno)));
                    return -1;
                }
                v4l2_buffers_[i].length[j] = planes[j].length;
                //v4l2_buffers_[i].bytesperline[j] = planes[j].bytesperline;
                //v4l2_buffers_[i].bytesperline[j] = 1280; //tbc 
                v4l2_buffers_[i].bytesperline[j] = bytesperlinei[j]; 
            }

            // Queue the buffer
            if (ioctl(v4l2_fd_, VIDIOC_QBUF, &buf) < 0) {
                g_logger.log(LOG_ERROR, "Failed to queue V4L2 buffer: " + std::string(strerror(errno)));
                return -1;
            }
        }

        // Start streaming
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        if (ioctl(v4l2_fd_, VIDIOC_STREAMON, &type) < 0) {
            g_logger.log(LOG_ERROR, "Failed to start V4L2 streaming: " + std::string(strerror(errno)));
            return -1;
        }

        g_logger.log(LOG_INFO, "V4L2 buffers initialized successfully");
        return 0;
    }

    void cleanup_v4l2_buffers() {
        if (v4l2_fd_ < 0) return;

        // Stop streaming
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        ioctl(v4l2_fd_, VIDIOC_STREAMOFF, &type);

        // Unmap buffers
        for (auto& buf : v4l2_buffers_) {
            for (int j = 0; j < VIDEO_MAX_PLANES; j++) {
                if (buf.start[j]) {
                    munmap(buf.start[j], buf.length[j]);
                    buf.start[j] = nullptr;
                }
            }
        }
        v4l2_buffers_.clear();
    }

    int init_input() {
        if (is_rtsp_source()) {
            // Keep the original RTSP initialization
            input_ctx_ = avformat_alloc_context();
            if (!input_ctx_) {
                g_logger.log(LOG_ERROR, "Failed to allocate input context");
                return AVERROR(ENOMEM);
            }
            
            AVDictionary* options = nullptr;
            av_dict_set(&options, "rtsp_transport", "tcp", 0);
            av_dict_set(&options, "stimeout", "5000000", 0); 

            int ret = avformat_open_input(&input_ctx_, config_.input_url.c_str(), nullptr, &options);
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
                             " | Resolution: " + config_.video_size +
                             " | video_stream_index_: " + std::to_string(video_stream_index_));
                    break;
                }
            }
        } else {
            // Initialize V4L2 MPlane device
            if (init_v4l2_device() < 0) {
                return false;
            }
            
            // Initialize V4L2 buffers
            if (init_v4l2_buffers() < 0) {
                close(v4l2_fd_);
                v4l2_fd_ = -1;
                return false;
            }

            // For V4L2, we'll set some default values since we're not using AVFormatContext
            video_stream_index_ = 0;
            g_logger.log(LOG_INFO, std::string("Input source: ") + config_.input_url + 
                     " | Format: NV12" +
                     " | Resolution: " + config_.video_size);
        }

        g_logger.log(LOG_INFO, "Input initialized successfully");
        return true;
    }

    int init_framerate_filter() {
        AVRational time_base;
        
        if (is_rtsp_source()) {
            AVStream* stream = input_ctx_->streams[video_stream_index_];
            time_base = stream->time_base;
        } else {
            time_base = (AVRational){1, static_cast<int>(config_.input_fps)};
        }

        const AVFilter* buffersrc = avfilter_get_by_name("buffer");
        const AVFilter* buffersink = avfilter_get_by_name("buffersink");
        filter_graph_ = avfilter_graph_alloc();

        AVRational sar = (AVRational){1, 1}; // default square pixels
        g_logger.log(LOG_INFO, std::to_string(sar.num) + "/" + std::to_string(sar.den));
        
        std::string args = "video_size=" + std::to_string(v4l2_width_) + "x" + 
                          std::to_string(v4l2_height_) + 
                          ":pix_fmt=" + std::to_string(v4l2_pix_fmt_) + 
                          ":time_base=" + std::to_string(time_base.num) + "/" + 
                          std::to_string(time_base.den) +  
                          ":pixel_aspect=" + std::to_string(sar.num) + "/" + 
                          std::to_string(sar.den);

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

        AVFilterContext* hflip_ctx;
        const AVFilter* hflip_filter = avfilter_get_by_name("hflip");
        ret = avfilter_graph_create_filter(&hflip_ctx, hflip_filter, "hflip", nullptr, nullptr, filter_graph_);
        if (ret < 0) {
            ERROR_STR(ret);
            g_logger.log(LOG_ERROR, "Failed to create hflip filter");
            return ret;
        }

        ret = avfilter_link(buffersrc_ctx_, 0, fps_ctx, 0);
        if (ret < 0) {
            ERROR_STR(ret);
            g_logger.log(LOG_ERROR, "Failed to link filters src to fps");
            return ret;
        }

        ret = avfilter_link(fps_ctx, 0, hflip_ctx, 0);
        if (ret < 0) {
            ERROR_STR(ret);
            g_logger.log(LOG_ERROR, "Failed to link filters fps to hflip");
            return ret;
        }

        ret = avfilter_link(hflip_ctx, 0, buffersink_ctx_, 0);
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

        encoder_ctx_->width = v4l2_width_;
        encoder_ctx_->height = v4l2_height_;
        encoder_ctx_->time_base = {1, config_.output_fps};
        encoder_ctx_->framerate = {config_.output_fps, 1};
        encoder_ctx_->pix_fmt = v4l2_pix_fmt_;
        encoder_ctx_->gop_size = config_.output_fps;
        encoder_ctx_->max_b_frames = 0;
        av_opt_set(encoder_ctx_->priv_data, "preset", "fast", 0);
        av_opt_set(encoder_ctx_->priv_data, "tune", "zerolatency", 0);

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
        if (is_rtsp_source()) {
            capture_loop_rtsp();
        } else {
            capture_loop_v4l2();
        }
    }

    void capture_loop_rtsp() {
        AVPacket* packet = av_packet_alloc();
        AVFrame* frame = av_frame_alloc();
        AVFrame* filtered_frame = av_frame_alloc();

        static int64_t ii=0;
        static int64_t retry_num=0;

        g_logger.log(LOG_INFO, "Capture thread started (RTSP)");

        while (!should_stop_) {
            auto capture_start = high_resolution_clock::now();

            if (input_ctx_->pb && input_ctx_->pb->error) {
               g_logger.log(LOG_ERROR, "I/O error detected, retry?");
            }

            retry_num++;
            int ret = av_read_frame(input_ctx_, packet);
            ii++; 
            if (ii%15 == 0)
            {
                //std::cout << "ERROR in av read frame ret=" << ret << "ii=" <<ii <<std::endl;
            }
            if (ret == AVERROR(ETIMEDOUT)) {
                g_logger.log(LOG_ERROR, "read timeout"+std::to_string(ii));
                std::cout << "timeout in av read frame ret=" << ret << "ii=" <<ii <<std::endl;
                std::this_thread::sleep_for(1ms); 
                continue;
            }

            if (ret == AVERROR_EXIT) {
                g_logger.log(LOG_WARNING, "av_read_frame timeout,,,.");
                continue;
            }

            if (ret == AVERROR(EAGAIN)) {
                std::this_thread::sleep_for(10ms);
                if (retry_num >= 10) {
                    g_logger.log(LOG_ERROR, "Input error ------------, reconnecting...,after retry_num="+std::to_string(retry_num));
                    avformat_close_input(&input_ctx_);
                    while (!init_input() && !should_stop_) {
                        g_logger.log(LOG_ERROR, "init_input try----------, reconnecting...,after retry_num="+std::to_string(retry_num));
                        std::this_thread::sleep_for(30ms);
                    }
                    retry_num = 0;
                }
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

            retry_num=0;
    
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

                AVRational input_rate = stream->avg_frame_rate.den > 0 ? 
                                        stream->avg_frame_rate : 
                                        stream->r_frame_rate;
                
                if (input_rate.den == 0 || input_rate.num == 0) {
                    input_rate = (AVRational){25, 469}; // num=25, den=469
                }
                
                AVRational time_base = (AVRational){input_rate.num, input_rate.den};
                frame->pts = av_rescale_q(frame_count_++, time_base, stream->time_base);

                auto capture_us = duration_cast<microseconds>(
                    high_resolution_clock::now() - capture_start).count();
                
                g_logger.log(LOG_DEBUG, std::string("Captured frame PTS: ") + std::to_string(frame->pts) + 
                          " | Capture time: " + std::to_string(capture_us) + "us");

                if (config_.enable_filter) {
                    g_logger.log(LOG_INFO, "Processing frame with filter");
                    process_with_filter(frame, filtered_frame);
                } else {

				    AVFrame* new_frame = av_frame_clone(frame); 
                    if (!new_frame) {
                        g_logger.log(LOG_ERROR, "Failed to clone filtered frame");
                        continue;
                    }
					frame_queue_.push(new_frame);
					
                }
            }
            av_packet_unref(packet);
        }

        av_packet_free(&packet);
        av_frame_free(&frame);
        av_frame_free(&filtered_frame);
        frame_queue_.wake_and_quit();
        
        g_logger.log(LOG_INFO, "Capture thread (RTSP) stopped");
    }

    void capture_loop_v4l2() {
        AVFrame* frame = av_frame_alloc();
        AVFrame* filtered_frame = av_frame_alloc();
        struct v4l2_buffer buf = {};
        struct v4l2_plane planes[VIDEO_MAX_PLANES] = {};

        g_logger.log(LOG_INFO, "Capture thread started (V4L2 MPlane)");

        const int max_retries = 3;
        int retry_count = 0;
        bool needs_reinit = false;

        while (!should_stop_) {
            auto capture_start = high_resolution_clock::now();

            if (needs_reinit) {
                cleanup_v4l2_buffers();
                close(v4l2_fd_);
                v4l2_fd_ = -1;

                if (init_input()) {
                    needs_reinit = false;
                    retry_count = 0;
                    g_logger.log(LOG_INFO, "V4L2 device reinitialized successfully");
                } else {
                    g_logger.log(LOG_ERROR, "Failed to reinitialize V4L2 device, retrying in 1 second...");
                    std::this_thread::sleep_for(1s);
                    continue;
                }
            }

            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(v4l2_fd_, &fds);

            struct timeval tv = {};
            tv.tv_sec = 0;  // 2 second timeout
            tv.tv_usec = 50000;

            int ret = select(v4l2_fd_ + 1, &fds, NULL, NULL, &tv);
            if (ret == -1) {
                g_logger.log(LOG_ERROR, "V4L2 select error: " + std::string(strerror(errno)));
                needs_reinit = true;
                continue;
            }
            if (ret == 0) {
                g_logger.log(LOG_WARNING, "V4L2 select timeout, retry_count"+std::to_string(retry_count));
                if (++retry_count >= max_retries) {
                    g_logger.log(LOG_ERROR, "Max select timeouts reached, reinitializing V4L2 device");
                    needs_reinit = true;
                }
                continue;
            }

            // Dequeue buffer
            memset(&buf, 0, sizeof(buf));
            memset(planes, 0, sizeof(planes));
            
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.length = VIDEO_MAX_PLANES;
            buf.m.planes = planes;

            if (ioctl(v4l2_fd_, VIDIOC_DQBUF, &buf) < 0) {
                g_logger.log(LOG_ERROR, "Failed to dequeue V4L2 buffer: " + std::string(strerror(errno)));
                needs_reinit = true;
                continue;
            }

            // Reset retry count on successful capture
            retry_count = 0;

            // Prepare AVFrame
            av_frame_unref(frame);
            frame->width = v4l2_width_;
            frame->height = v4l2_height_;
            frame->format = v4l2_pix_fmt_;
            frame->pts = frame_count_++;

            // For MPlane, we need to set data and linesize for each plane
            for (int i = 0; i < 1; i++) {
                frame->data[i] = static_cast<uint8_t*>(v4l2_buffers_[buf.index].start[i]) + planes[i].data_offset;
                frame->linesize[i] = v4l2_buffers_[buf.index].bytesperline[i];
            }
            frame->data[1] = frame->data[0] + (v4l2_buffers_[buf.index].bytesperline[0] * v4l2_height_);
            frame->linesize[1] = v4l2_buffers_[buf.index].bytesperline[0]; // testing

            auto capture_us = duration_cast<microseconds>(
                high_resolution_clock::now() - capture_start).count();
            
            g_logger.log(LOG_DEBUG, std::string("Captured frame PTS: ") + std::to_string(frame->pts) + 
                      " | Capture time: " + std::to_string(capture_us) + "us");

            if (config_.enable_filter) {
                g_logger.log(LOG_DEBUG, "Processing frame with filter");
                process_with_filter(frame, filtered_frame);
            } else {
				AVFrame* new_frame = av_frame_clone(frame); 
                if (!new_frame) {
                    g_logger.log(LOG_ERROR, "Failed to clone filtered frame");
                    continue;
                }
				frame_queue_.push(new_frame);
            }

            // Requeue the buffer
            if (ioctl(v4l2_fd_, VIDIOC_QBUF, &buf) < 0) {
                g_logger.log(LOG_ERROR, "Failed to requeue V4L2 buffer: " + std::string(strerror(errno)));
                needs_reinit = true;
                continue;
            }
        }

        // Cleanup
        cleanup_v4l2_buffers();
        av_frame_free(&frame);
        av_frame_free(&filtered_frame);
        frame_queue_.wake_and_quit();
        
        g_logger.log(LOG_INFO, "Capture thread (V4L2 MPlane) stopped");
    }

    void process_with_filter(AVFrame* frame, AVFrame* filtered_frame) {
        auto filter_start = high_resolution_clock::now();
        
        if (av_buffersrc_add_frame(buffersrc_ctx_, frame) < 0) {
            g_logger.log(LOG_ERROR, "Error feeding frame to filter");
            return;
        }

        while (true) {
            int ret = av_buffersink_get_frame(buffersink_ctx_, filtered_frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) { 
                break;
            }
            
            if (ret < 0) {
                ERROR_STR(ret);
                g_logger.log(LOG_ERROR, "Error getting filtered frame");
                break;
            }

            auto filter_us = duration_cast<microseconds>(
                high_resolution_clock::now() - filter_start).count();
            
            g_logger.log(LOG_DEBUG, std::string("Filtered frame PTS: ") + std::to_string(filtered_frame->pts) +
                      " | Filter time: " + std::to_string(filter_us) + "us");

			AVFrame* new_frame = av_frame_clone(filtered_frame);
            if (!new_frame) {
                g_logger.log(LOG_ERROR, "Failed to clone filtered frame");
                av_frame_unref(filtered_frame);
                continue;
            }
			
			
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

                if (last_pts_ != AV_NOPTS_VALUE && pkt->pts <= last_pts_) {
                    pkt->pts = last_pts_ + av_rescale_q(1, encoder_ctx_->time_base,
                                                     output_ctx_->streams[0]->time_base);
                }
                last_pts_ = pkt->pts; 

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
        
        cleanup_v4l2_buffers();
        
        if (v4l2_fd_ >= 0) {
            close(v4l2_fd_);
            v4l2_fd_ = -1;
        }
        
        if (encoder_ctx_) {
            avcodec_close(encoder_ctx_);
            avcodec_free_context(&encoder_ctx_);
        }
		
        if (input_ctx_) avformat_close_input(&input_ctx_);
        if (output_ctx_) {
            if (output_ctx_->pb) avio_closep(&output_ctx_->pb);
            avformat_free_context(output_ctx_);
        }
        if (filter_graph_) avfilter_graph_free(&filter_graph_);
		if (filter_graph_) {
            avfilter_graph_free(&filter_graph_);
            buffersrc_ctx_ = nullptr;
            buffersink_ctx_ = nullptr;
        }
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
    config.enable_filter = true;
    config.log_file = "streamer.log";
    config.log_level = LOG_INFO;
    config.console_log = true;

    if (argc > 3) {
        config.log_file = argv[3];
    }

    VideoStreamer streamer(config);
    if (streamer.init() < 0) {
        return 1;
    }

    signal(SIGINT, [](int) { exit(0); });

    streamer.run();
    return 0;
}


/*

g++ v4l2streamout.cpp -o v4l2streamout -I/userdata/stream/myusr/include -fpermissive -Wall -Wextra -L/userdata/stream/myusr/lib -Wl,-rpath,/userdata/stream/myusr/lib -lavformat -lavfilter -lavcodec -lavutil -lavdevice -lswscale -lavfilter -lpthread -D_GNU_SOURCE -fpermissive
./v4l2streamout /dev/video0 rtsp://192.168.1.86:554/live/stream

*/
