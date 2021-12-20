#include "TranzSegmenter.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
//#include <stdatomic.h>
#include <stdint.h>
#include <time.h>
#include <stdio.h>

#if HAVE_IO_H
#include <io.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_SYS_RESOURCE_H
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#elif HAVE_GETPROCESSTIMES
#include <windows.h>
#endif
#if HAVE_GETPROCESSMEMORYINFO
#include <windows.h>
#include <psapi.h>
#endif
#if HAVE_SETCONSOLECTRLHANDLER
#include <windows.h>
#endif


#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#if HAVE_TERMIOS_H
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <termios.h>
#elif HAVE_KBHIT
#include <conio.h>
#endif

//#if HAVE_PTHREADS
#include <pthread.h>
//#endif

#include<signal.h>
#include<unistd.h>
#include <time.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <semaphore.h>
#include <fcntl.h>
#include <syslog.h>

#define ALWYAS_LOG  AV_LOG_ERROR
#define WAIT_COUNT  500
#define HLS_SEGMENT_DURATION    3
#define HLS_LIST_SIZE           2
#define HLS_WRAP                10
#define HLS_START_NUMBER        1

exit_evt_app_param_t     g_evt_param;
static int8_t   g_stop_processing = 0;

tStream_OutputFeed_t     *g_tout_av_feed = NULL;
tStream_InputFeed_t      *g_tranz_input_feed = NULL;
int g_hb_to = 0;
int g_log_level = 0;


static int8_t read_arg(inout_file_t * a_avout_file, inout_file_t * a_vout_file,
                       audio_file_t * a_aout_file, inout_file_t * a_in_file,
                       int a_argc, char *a_argv[]);

int stream_conversion(inout_file_t * a_avout_file, inout_file_t * a_in_file_name);

static int open_tranz_ingest_feed(void);
static int8_t open_inputs(inout_file_t * a_in_file);


static int8_t open_outputs(inout_file_t * a_avout_file);
static int open_output_audio_video(void);


static int8_t read_inputs(pthread_t **a_tid);
static void *read_tranz_input_video(void *a_thread_arg);

static break_continue_t transcode_audio_video_packet(void);

static void *heartbeat(void *a_thread_arg);
static void notify_exit_event_to_processor(void);
static void log_packet(char *str, const AVFormatContext *fmt_ctx, const AVPacket *pkt);

void sig_handler_app_close(int signo)
{
    if(signo == SIGSEGV) {
        if(g_evt_param.m_argument >= 0) {
            notify_exit_event_to_processor();
        }
        g_stop_processing = 1;
        av_usleep(200000);
        av_log(NULL, AV_LOG_ERROR, "Opoos app crashed......\n"); //Error
        exit(0);
    }
    if (signo == SIGUSR1 || signo == SIGINT){
        av_log(NULL, AV_LOG_ERROR, "signal to end\n"); //Not error
        g_stop_processing = 1;
        av_usleep(200000);
        av_log(NULL, AV_LOG_ERROR, "Sengenter Quiting\n");  //Not error
        exit(0);
    }
}

static void log_callback_null(void *ptr, int level, const char *fmt, va_list vl)
{
    char buff[1024] = {0};
    int print_prefix = 0;
    int len = 0;
    char c = 0;
    if(level > g_log_level) return;

    len = sprintf(buff, "[job_id = %s ] : ", g_evt_param.m_id_str);
    av_log_format_line(ptr, level, fmt, vl, buff + len, sizeof(buff) - len, &print_prefix);

    c = *(buff + len);
    if((c == '\0') || (c == '\n') || (c=='\r')) return;
    syslog (LOG_INFO | LOG_LOCAL2, "%s", (const char *)buff);

}


int main(int argc, char **argv)
{
    int8_t l_ret = 0;
    uint8_t l_index = 0;
    inout_file_t l_vout_file;
    audio_file_t l_aout_file;
    inout_file_t l_avout_file;
    inout_file_t l_in_file;
    //uint8_t l_in_file[TRANZVERY_LONG_LENGTH];

    l_avout_file.m_file_num = 0;
    l_vout_file.m_file_num = 0;
    l_aout_file.m_file_num = 0;
    l_in_file.m_file_num = 0;

    uint64_t l_time = av_gettime();
    uint64_t l_rtime = av_gettime_relative();
    printf("main start --> Time: %" PRId64 " " "RTime: %" PRId64 "\n", l_time, l_rtime);

    memset(g_evt_param.m_id_str, 0, sizeof(g_evt_param.m_id_str));
    memset(g_evt_param.m_hb_str, 0, sizeof(g_evt_param.m_hb_str));
    g_evt_param.m_argument = 0;
    g_evt_param.m_client_socket = 0;
    g_evt_param.m_hb_argument = 0;
    memset(&g_evt_param.m_remote, 0, sizeof(g_evt_param.m_remote));

    signal(SIGHUP, SIG_IGN);
    if (signal(SIGUSR1, sig_handler_app_close) == SIG_ERR ||
        signal(SIGINT, sig_handler_app_close) == SIG_ERR  ||
        signal(SIGSEGV, sig_handler_app_close) == SIG_ERR)
    {
        av_log(NULL, AV_LOG_ERROR,"Can't catch sig_handler_app_close\n");
        return 0;
    }

    l_aout_file.m_file_name = (uint8_t **)malloc(sizeof(uint8_t*));

    if((l_ret = read_arg(&l_avout_file, &l_vout_file, &l_aout_file, &l_in_file, argc, argv)) < 0) {
        return l_ret;
    }

    //Log setting....
    //av_log_set_callback(log_callback_null);
    openlog ("Segmenter", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL2);

    g_log_level = AV_LOG_ERROR;
    //g_log_level = AV_LOG_DEBUG;
    av_log_set_level(g_log_level);

    av_log(NULL, ALWYAS_LOG,"tSegmenter started\n" );

    stream_conversion(&l_avout_file, &l_in_file);

    if(l_vout_file.m_file_num > 0) {
        free(l_vout_file.m_file_name);
    }
    if(l_in_file.m_file_num > 0) {
        free(l_in_file.m_file_name);
    }

    if(l_aout_file.m_file_num > 0)
    {
        for(l_index = 0;l_index < l_aout_file.m_file_num;l_index ++){
            free(l_aout_file.m_file_name[l_index]);
        }
        free(l_aout_file.m_file_name);
    }

    if(g_evt_param.m_argument >= 0) {
        av_log(NULL, AV_LOG_ERROR,"OK notify_exit_event_to_processor sending\n" );
        notify_exit_event_to_processor();
    }
    av_log(NULL, AV_LOG_INFO,"Thank you for using tSegmenter\n" );
    return 0;
}

static int8_t read_arg(inout_file_t * a_avout_file, inout_file_t * a_vout_file, audio_file_t * a_aout_file, inout_file_t * a_in_file, int a_argc, char *a_argv[])
{
    int l_index = 0;
    int i;

    for (i = 0;i < a_argc; i ++) {

        if (strcmp(a_argv[i],"-i")==0) {
            if ( i + 1 >= a_argc ) {
                av_log(NULL, AV_LOG_ERROR,"Error: No input specified after -i \n" );
                return -1;
            }

            i++;
            if(0 < a_in_file->m_file_num) {
                av_log(NULL, AV_LOG_ERROR,"Error: Multiple input not allowed\n" );
                return -1;
            }

            a_in_file->m_file_num ++;

            a_in_file->m_file_name = (uint8_t *)malloc(TRANZ_VERY_LONG_LENGTH * sizeof(uint8_t));

            strcpy(a_in_file->m_file_name, a_argv[i]); //all input file will be accepted
                                                       //But first file will take into account

            //strcpy(a_in_file_name, a_argv[i]);

        } else if (strcmp(a_argv[i], "-o") == 0 ) {
            if ( i + 1 >= a_argc ) {
                av_log(NULL, AV_LOG_ERROR,"Error No output specified after -v \n" );
                return -2;
            }
            i++;

            if(0 < a_avout_file->m_file_num) {
                av_log(NULL, AV_LOG_ERROR,"Error: Multiple HLS output not allowed\n" );
                return -1;
            }
            a_avout_file->m_file_num ++;
            a_avout_file->m_file_name = (uint8_t *)malloc(TRANZ_VERY_LONG_LENGTH * sizeof(uint8_t));
            strcpy(a_avout_file->m_file_name, a_argv[i]); //all output file will be accepted
        }else if (strcmp(a_argv[i], "-id") == 0 ) {
            if ( i + 1 >= a_argc ) {
                av_log(NULL, AV_LOG_ERROR,"Error No ID string specified after -id \n" );
                return -2;
            }
            i++;

            strcpy(g_evt_param.m_id_str, a_argv[i]);
        } else if (strcmp(a_argv[i], "-hb") == 0 ) {
            if ( i + 1 >= a_argc ) {
                av_log(NULL, AV_LOG_ERROR,"Error No string specified after -hb \n" );
                return -2;
            }
            i++;
            strcpy(g_evt_param.m_hb_str, a_argv[i]);

        } else if (strcmp(a_argv[i], "-e") == 0 ) {
            if ( i + 1 >= a_argc ) {
                av_log(NULL, AV_LOG_ERROR,"Error No event number specified after -e \n" );
                return -2;
            }
            i++;
            g_evt_param.m_argument = atoi(a_argv[i]);

        } else if (strcmp(a_argv[i], "-h") == 0 ) {
            if ( i + 1 >= a_argc ) {
                av_log(NULL, AV_LOG_ERROR,"Error No string specified after  -h \n" );
                return -2;
            }
            i++;
            g_evt_param.m_hb_argument = atoi(a_argv[i]);

        } else if (strcmp(a_argv[i], "-ht") == 0 ) {
            if ( i + 1 >= a_argc ) {
                av_log(NULL, AV_LOG_ERROR,"Error No heartbit timer specified after  -ht \n" );
                return -2;
            }
            i++;
            g_hb_to = atoi(a_argv[i]);
        }
    }
    return 0;
}

int stream_conversion(inout_file_t * a_avout_file, inout_file_t * a_in_file)
{
    int64_t     l_running_count = 0; //quick fix
    int8_t  l_ret = 0;
    uint8_t l_flag = 0;
    uint8_t l_index = 0;
    pthread_t   *l_tid_input = NULL;
    break_continue_t status;

    static tStream_InputFeed_t      *l_tranz_input_video_feed = NULL;
    static tStream_OutputFeed_t     *l_tranz_output_video_feed = NULL;

    pthread_t   l_tid_heart_beat;
    //sem_t *l_sem_id;
    int l_hbl = 0;
    int l_idl = 0;


    g_stop_processing = 0;

    avcodec_register_all();
    avdevice_register_all();
    av_register_all();
    avformat_network_init();

    if(a_avout_file->m_file_num == 0) {
        av_log(NULL, AV_LOG_ERROR,"Correct AUDIO VIDEO out needed\n");
        return 0;
    }

    l_hbl = strlen(g_evt_param.m_hb_str);
    l_idl = strlen(g_evt_param.m_id_str);

    if(0 != l_hbl && 0 != l_idl ) {
        //l_sem_id = sem_open(g_evt_param.m_id_str, O_CREAT, 0600, 0);

        //if(l_sem_id == SEM_FAILED) {
        //    av_log(NULL, AV_LOG_ERROR,"Failed to create Semaphore\n");
            //g_stop_processing = 1;
            //return 0;
        //} else {
        //    av_log(NULL, AV_LOG_ERROR,"Semaphore Posted\n");
        //}

        //sem_post(l_sem_id);
        //sem_close(l_sem_id);

        l_ret = pthread_create(
                &l_tid_heart_beat, NULL, &heartbeat,
                NULL);
    }

    av_log(NULL, ALWYAS_LOG,"Opening the inputs\n" );

    printf("Opening the inputs\n" );
    open_inputs(a_in_file);
    if(TRANZ_OPEN_S_INPUT_SUCCESS != g_tranz_input_feed->m_status) {
        av_log(NULL, AV_LOG_ERROR,"Correct input needed\n");
        return 0;
    }

    av_log(NULL, ALWYAS_LOG,"Opening the output\n" );
    printf("Opening the output\n" );
    l_flag = 0;
    l_ret = open_outputs(a_avout_file);

    if(TRANZ_OPEN_S_OUTPUT_SUCCESS != g_tranz_input_feed->m_status) {
        av_log(NULL, AV_LOG_ERROR,"Error in output stream\n");
        if(l_ret < 0) {
            av_log(NULL, AV_LOG_ERROR,"Video Device: %s can not open\n", a_avout_file->m_file_name);
            return l_ret;
        }
        return 0;
    }

    av_log(NULL, ALWYAS_LOG,"Input Reading started\n" );
    printf("Input Reading started\n" );
    read_inputs(&l_tid_input);

    av_log(NULL, ALWYAS_LOG,"HLS started\n" );
    printf("HLS started\n" );

    while(0 == g_stop_processing || g_tranz_input_feed->m_running)
    {
        status =  transcode_audio_video_packet();
        //printf("transcode_audio_video_packet returned : %d\n", status);
        //quick fix ----------------
        if ( e_continue == status)
        {
            l_running_count ++;
            if(l_running_count >= WAIT_COUNT)
            {
                printf("quick fix may introduce larger error :D \n");
                break;
            }
        }
        else
        {
            l_running_count = 0;
        }
        //quick fix end ----------------

        if ( e_break == status){
            //write endlist tag..!
            //av_write_trailer(g_tout_av_feed->m_aud_vid_ofmt_ctx); // Sourav; removed EXT-X-ENDLIST
            av_log(NULL, AV_LOG_INFO, "EOF received Restarting connection !\n");
            //Close the existing and restart the new..!
            avformat_close_input(&g_tout_av_feed->m_aud_vid_ofmt_ctx);
            avformat_free_context(g_tout_av_feed->m_aud_vid_ofmt_ctx);

            //open_inputs(&l_tranz_input_video_feed, a_in_file);
            open_inputs(a_in_file);
            if(TRANZ_OPEN_S_INPUT_SUCCESS != g_tranz_input_feed->m_status) {
                av_log(NULL, AV_LOG_ERROR,"Correct input needed\n");
                return 0;
            }

            l_flag = 0;
            l_ret = open_outputs(a_avout_file);

            if(TRANZ_OPEN_S_OUTPUT_SUCCESS != g_tranz_input_feed->m_status) {
                av_log(NULL, AV_LOG_ERROR,"Error in output stream\n");
                if(l_ret < 0) {
                    av_log(NULL, AV_LOG_ERROR,"Video Device: %s can not open\n", a_avout_file->m_file_name);
                    return l_ret;
                }
                return 0;
            }

            read_inputs(&l_tid_input);

        } else if(e_write_error == status) {
                av_log(NULL, AV_LOG_ERROR, "Cannot write HLS output quiting..!\n");
                g_stop_processing = 1;
                break;
        } else if(e_unknown== status) {
                av_log(NULL, AV_LOG_ERROR, "Unknown error, HLS output quiting..!\n");
                g_stop_processing = 1;
                break;
        }
    }

    av_log(NULL, ALWYAS_LOG,"tSegmenter ended\n" );
    g_stop_processing = 1;

    //pthread_join(*l_tid_input, NULL);
    //pthread_join(l_tid_heart_beat, NULL);

    return 0;
}

static int8_t open_inputs(inout_file_t * a_in_file)

{
    uint8_t     l_ret = 0;
    uint8_t     l_index = 0;
    pthread_t   *l_tid = NULL;
    tStream_InputFeed_t * l_in_feed;

    //Only one input will be opened(that is first input)
    l_in_feed = (tStream_InputFeed_t *)malloc(sizeof(tStream_InputFeed_t));

    //Start "open input"
    uint64_t l_time = av_gettime();
    uint64_t l_rtime = av_gettime_relative();
    printf("open_inputs --> Time: %" PRId64 " " "RTime: %" PRId64 "\n", l_time, l_rtime);

    l_in_feed->m_index = l_index;
    l_in_feed->m_is_eof = 0;
    l_in_feed->m_pts = 0;

    //This need to free
    l_in_feed->m_filename = strdup(a_in_file->m_file_name);
    l_in_feed->m_protocol = (char *)malloc(5*sizeof(char*));

    strcpy(l_in_feed->m_protocol, "tcp");

    //(*a_in_feed) = l_in_feed;
    g_tranz_input_feed = l_in_feed;

    //open_tranz_input_video(&(*a_in_feed)[l_index]);
    open_tranz_ingest_feed();

    return 0;
}

//-------------------------------------------------------------------------------------------------
// Openings RTMP server for input...
//-------------------------------------------------------------------------------------------------
static int open_tranz_ingest_feed()
{
    int32_t         l_ret = 0;
    int32_t         l_index = 0;
    AVDictionary    *l_opts = NULL;
    AVFormatContext *l_ifmt_ctx = NULL;
    AVInputFormat   *l_inFrmt = NULL;
    int h264_flag = 0;
    int aac_flag = 0;

    printf("open_tranz_ingest_feed\n");
    av_log(NULL, AV_LOG_INFO, "Opening ingest..!\n");
    //av_usleep(100);
    g_tranz_input_feed->m_ifmt_ctx =  NULL;

   if(strstr(g_tranz_input_feed->m_filename, "rtmp")) {
        av_dict_set(&l_opts, "listen", "1", 0);
        av_dict_set(&l_opts, "rtmp_buffer", "100", 0);
        av_dict_set(&l_opts, "rtmp_live", "live", 0);
        //-analyzeduration 1 -probesize 1000
        av_dict_set(&l_opts, "analyzeduration", "32", 0);
        //av_dict_set(&l_opts, "probesize", "500M", 0);
        av_dict_set(&l_opts, "probesize", "32", 0);
        //show_streams

        //l_ret = av_dict_set(&opts, "analyzeduration", "0", 0);
        l_ret = av_dict_set(&l_opts, "sync", "1", 0);


        l_inFrmt= av_find_input_format("flv");
        av_log(NULL, AV_LOG_INFO,"av_find_input_format: %p\n", l_inFrmt);
        printf("av_find_input_format\n");
    }


    av_dict_set(&l_opts, "fflags", "nobuffer", 1); //-fflags nobuffer
    av_dict_set(&l_opts, "fflags", "flush_packets", 1); //-fflags flush_packets
    //av_dict_set(&l_opts, "fflags", "sortdts", 1); //-fflags sortdts
    //av_dict_set(&l_opts, "fflags", "genpts", 1); //-fflags nofillin

    //av_opt_set(l_ifmt_ctx,"f","flv", AV_OPT_SEARCH_CHILDREN);
    av_opt_set(l_ifmt_ctx,"codec:v","h264",AV_OPT_SEARCH_CHILDREN);
    //av_opt_set(l_ifmt_ctx,"probesize","32M", AV_OPT_SEARCH_CHILDREN);

    if ((l_ret = avformat_open_input(&l_ifmt_ctx, g_tranz_input_feed->m_filename, l_inFrmt, &l_opts)) < 0) {

        av_log(NULL, AV_LOG_ERROR,"Error: Opening ingest Failed: error:%s\n", av_err2str(l_ret));

        g_tranz_input_feed->m_ifmt_ctx = NULL;
        g_tranz_input_feed->m_status = l_ret;
        return l_ret;
    }
    printf("l_ifmt_ctx->probesize:: %" PRId64 "\n", l_ifmt_ctx->probesize);
    printf("avformat_open_input success\n");
    //l_ifmt_ctx->use_wallclock_as_timestamps = 1;
    g_tranz_input_feed->m_ifmt_ctx = l_ifmt_ctx; //Save the context

    if ((l_ret = avformat_find_stream_info(l_ifmt_ctx, NULL)) < 0) {
    //if ((l_ret = avformat_find_stream_info(l_ifmt_ctx, &l_opts)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information: %s\n", av_err2str(l_ret));
        g_tranz_input_feed->m_status = l_ret;
        return l_ret;
    }
    printf("avformat_find_stream_info success\n");
    //--------------------------------------------------------------------------
    av_log(NULL, AV_LOG_INFO, "Inside Open Input To the for loop\n");

    h264_flag = 0;
    aac_flag = 0;
    for (l_index = 0; l_index < l_ifmt_ctx->nb_streams; l_index ++) {
        AVStream *ll_stream = l_ifmt_ctx->streams[l_index];
        if (ll_stream->codecpar->codec_id == AV_CODEC_ID_H264 ) {
            h264_flag = 1;
        } else if (ll_stream->codecpar->codec_id == AV_CODEC_ID_AAC ){
            aac_flag = 1;
        }
    }
    if (h264_flag == 0 && aac_flag == 0) {
        av_log(NULL, AV_LOG_ERROR, "H264 and AAC not found in ingest stream \n");
        l_ret = AVERROR_DECODER_NOT_FOUND;
        return l_ret;
    }

    av_dump_format(l_ifmt_ctx, 0, g_tranz_input_feed->m_filename, 0);

    printf("open_tranz_ingest_feed success end\n");

    g_tranz_input_feed->m_status = TRANZ_OPEN_S_INPUT_SUCCESS;
    return g_tranz_input_feed->m_status;
}


//-------------------------------------------------------------
static int8_t open_outputs(inout_file_t * a_avout_file)
{
    //Start All "open output" threads

    uint64_t l_time = av_gettime();
    uint64_t l_rtime = av_gettime_relative();
    printf("open_outputs --> Time: %" PRId64 " " "RTime: %" PRId64 "\n", l_time, l_rtime);

    //Current implementation need to know the correct input context
    if(TRANZ_OPEN_S_INPUT_SUCCESS == g_tranz_input_feed->m_status) {
        av_log(NULL, AV_LOG_INFO, "in Open Output, -----input is OK\n");
    }

    g_tout_av_feed = (tStream_OutputFeed_t *)malloc(sizeof(tStream_OutputFeed_t));
    //This need to free
    g_tout_av_feed->m_aud_vid_filename= strdup(a_avout_file->m_file_name);
    open_output_audio_video();

    return 0;
}

static int open_output_audio_video()
{
    uint8_t      l_index = 0;
    int         l_ret = 0;
    int         hls_flag = 0;
    AVDictionary    *l_opts = NULL;
    AVCodecContext  *l_dec_ctx = NULL;
    AVStream        *l_out_stream = NULL;
    AVStream        *l_in_stream = NULL;
    AVCodecContext  *l_enc_ctx = NULL;
    AVCodec *l_encoder = NULL;
    AVOutputFormat *oformat;

    g_tout_av_feed->m_aud_vid_ofmt_ctx = NULL;

    if(strstr(g_tout_av_feed->m_aud_vid_filename, "m3u8")) {
         avformat_alloc_output_context2(
            &g_tout_av_feed->m_aud_vid_ofmt_ctx,
            NULL,
            "hls",
            g_tout_av_feed->m_aud_vid_filename);

            hls_flag = 1;

    } else {
        avformat_alloc_output_context2(
            &g_tout_av_feed->m_aud_vid_ofmt_ctx,
            NULL,
            NULL,
            g_tout_av_feed->m_aud_vid_filename);
    }



    if (!g_tout_av_feed->m_aud_vid_ofmt_ctx) {
        av_log(NULL, AV_LOG_ERROR, "Could not create Segmenter audio_video output context\n");
        return AVERROR_UNKNOWN;
    }

    if (hls_flag ) {
        // All the HLS options

        //TO Modify: tempurary Changes for dev audd sync
        //av_opt_set_int(g_tout_av_feed->m_aud_vid_ofmt_ctx->priv_data, "-strftime", 1, 0);
        //av_opt_set_int(g_tout_av_feed->m_aud_vid_ofmt_ctx->priv_data, "use_localtime", 1, 0);

        //av_opt_set(g_tout_av_feed->m_aud_vid_ofmt_ctx->priv_data, "hls_segment_filename", "file-%Y%m%d-%s.ts", 0);

        //av_dict_set(&l_opts, "fflags", "genpts", 1); //-fflags nofillin

        //TO Modify: tempurary Changes for dev audd sync
        //av_dict_set(&l_opts, "fflags", "-autobsf", 0);

        //av_opt_set_int(g_tout_av_feed->m_aud_vid_ofmt_ctx->priv_data, "flags", "global_header", 0);


        av_opt_set(g_tout_av_feed->m_aud_vid_ofmt_ctx->priv_data, "event", "1", 0);
        av_opt_set_int(g_tout_av_feed->m_aud_vid_ofmt_ctx->priv_data, "start_number", HLS_START_NUMBER, 0);
        av_opt_set_double(g_tout_av_feed->m_aud_vid_ofmt_ctx->priv_data, "hls_time", HLS_SEGMENT_DURATION, 0);
        av_opt_set_int(g_tout_av_feed->m_aud_vid_ofmt_ctx->priv_data, "hls_list_size", HLS_LIST_SIZE, 0);
        av_opt_set_int(g_tout_av_feed->m_aud_vid_ofmt_ctx->priv_data, "hls_wrap", HLS_WRAP, 0);
        //TO Modify: tempurary Changes for dev audd sync
        //av_opt_set(g_tout_av_feed->m_aud_vid_ofmt_ctx->priv_data, "hls_flags", "delete_segments", 1);
    }
    for (l_index = 0; l_index < g_tranz_input_feed->m_ifmt_ctx->nb_streams; l_index ++) {

        l_in_stream = g_tranz_input_feed->m_ifmt_ctx->streams[l_index];

        if (l_in_stream->codecpar->codec_id == AV_CODEC_ID_H264 ||
            l_in_stream->codecpar->codec_id == AV_CODEC_ID_AAC ) {
            l_out_stream = avformat_new_stream(g_tout_av_feed->m_aud_vid_ofmt_ctx, NULL);
            if (!l_out_stream) {
                av_log(NULL, AV_LOG_ERROR, "Failed allocating output audio_video stream\n");
                return AVERROR_UNKNOWN;
            }

            l_ret = avcodec_parameters_copy(l_out_stream->codecpar, l_in_stream->codecpar);

            if (l_ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "audio_video Copying parameters for stream #%u failed\n", l_index);
                return l_ret;
            }
            l_out_stream->codecpar->codec_tag = 0;
            l_out_stream->time_base = l_in_stream->time_base;
             // Set the encoder context to null
            g_tout_av_feed->m_out_av_enc_ctx[l_index] = NULL;
        }
    }

    //if (g_tout_live_av_feed->m_aud_vid_ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
    //    l_enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    //}
    //g_tout_av_feed->m_aud_vid_ofmt_ctx->oformat->flags |= AVFMT_GLOBALHEADER;

    av_dump_format(g_tout_av_feed->m_aud_vid_ofmt_ctx, 0, g_tout_av_feed->m_aud_vid_filename, 1);

    if (!(g_tout_av_feed->m_aud_vid_ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        l_ret = avio_open2(&g_tout_av_feed->m_aud_vid_ofmt_ctx->pb, g_tout_av_feed->m_aud_vid_filename, AVIO_FLAG_WRITE, NULL, &l_opts);

        if (l_ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Could not open audio_video output file '%s'", g_tout_av_feed->m_aud_vid_filename);
            return l_ret;
        }
    }

    /* init muxer, write output file header */
    //l_ret = avformat_write_header(g_tout_av_feed->m_aud_vid_ofmt_ctx, &l_opts);
    l_ret = avformat_write_header(g_tout_av_feed->m_aud_vid_ofmt_ctx, NULL);
    if (l_ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error occurred when opening audio_video output file\n");
        return l_ret;
    }

    g_tout_av_feed->m_status = TRANZ_OPEN_S_OUTPUT_SUCCESS;
    return l_ret;
}


//-------------------------------------------------------------------------------------------------
// Creates threads for reading the streams
//-------------------------------------------------------------------------------------------------
static int8_t read_inputs(pthread_t **a_tid)
{
    int8_t l_ret = 0;

    (*a_tid) = (pthread_t *)malloc(sizeof(pthread_t));

    g_tranz_input_feed->m_running = 0;

    if(TRANZ_OPEN_S_INPUT_SUCCESS == g_tranz_input_feed->m_status) {
        l_ret = av_thread_message_queue_alloc(&(g_tranz_input_feed->in_thread_audio_video_queue), TRANZ_MSGQ_LENGTH, sizeof(AVPacket));

        l_ret = pthread_create(
                        (*a_tid), NULL, &read_tranz_input_video,
                        g_tranz_input_feed);
    }
    g_tranz_input_feed->m_tid = (*a_tid);

    return l_ret;
}

//-------------------------------------------------------------------------------------------------
// Reads the input stream and stores in Queue
//-------------------------------------------------------------------------------------------------
static void *read_tranz_input_video(void *a_thread_arg)
{
    int l_ret = 0;
    tStream_InputFeed_t *l_input_feed = (tStream_InputFeed_t *)a_thread_arg;
    unsigned l_flags = l_input_feed->non_blocking = AV_THREAD_MESSAGE_NONBLOCK;
    int32_t l_stream_index = 0;
    enum AVMediaType l_codec_type;

    uint64_t l_time = av_gettime();
    uint64_t l_rtime = av_gettime_relative();
    printf("read_tranz_input_video --> Time: %" PRId64 " " "RTime: %" PRId64 "\n", l_time, l_rtime);

    while(0 == g_stop_processing) {
        AVPacket    l_packet;

        l_ret = av_read_frame(l_input_feed->m_ifmt_ctx, &l_packet);

        if (l_ret == AVERROR(EAGAIN)) {
            //l_input_feed->m_is_eof = 0;
            av_log(NULL, AV_LOG_WARNING, "AVERROR(EAGAIN) received\n");
            av_usleep(TRANZ_SLEEP_TIME);
            continue;
        }
        //return l_packet;
        if (l_ret < 0) {
            //in_thread_video_queue

            av_thread_message_queue_set_err_recv(l_input_feed->in_thread_audio_video_queue, l_ret);

            l_input_feed->m_is_eof = 1;
            av_log(NULL, AV_LOG_ERROR, "ERROR or EOF: %s\n", av_err2str(l_ret));
            break;
        }

        l_stream_index = l_packet.stream_index;
        l_codec_type = g_tranz_input_feed->m_ifmt_ctx->streams[l_stream_index]->codecpar->codec_type;

        l_ret = av_thread_message_queue_send(l_input_feed->in_thread_audio_video_queue, &l_packet, l_flags);
        if (l_flags && l_ret == AVERROR(EAGAIN)) {
            l_flags = 0;
            l_ret = av_thread_message_queue_send(l_input_feed->in_thread_audio_video_queue, &l_packet, l_flags);
            av_log(NULL, AV_LOG_INFO, "Thread Audio Video message queue blocking; consider raising the"
                                      "(thread_queue_size option current value: %d)\n", TRANZ_MSGQ_LENGTH);
        } else {
            av_log(NULL, AV_LOG_INFO, "No drop: video message read and send\n");
        }
        if (l_ret < 0) {
            if (l_ret != AVERROR_EOF)
                av_log(NULL, AV_LOG_WARNING, "Unable to send packet to main thread: %s\n",
                       av_err2str(l_ret));
            av_packet_unref(&l_packet);
            av_thread_message_queue_set_err_recv(l_input_feed->in_thread_audio_video_queue, l_ret);
            //break;
        }

        l_input_feed->m_running = 1;
    }
    l_input_feed->m_running = 0;
}

//-------------------------------------------------------------------------------------------------
// Get the frame from Queue
//-------------------------------------------------------------------------------------------------
static int get_input_audio_video_packet_q(tStream_InputFeed_t *f, AVPacket *pkt)
{
    return av_thread_message_queue_recv(f->in_thread_audio_video_queue, pkt,
                                        f->non_blocking ?
                                        AV_THREAD_MESSAGE_NONBLOCK : 0);
}


static break_continue_t transcode_audio_video_packet(void) {
    int l_ret = e_continue;
    int32_t     l_stream_index = 0;
    enum AVMediaType    l_codec_type;

    AVPacket    l_pkt;
    AVFrame *l_frame = NULL;

    l_ret = get_input_audio_video_packet_q(g_tranz_input_feed, &l_pkt);
    if (l_ret == AVERROR(EAGAIN)) {
        av_usleep(TRANZ_SLEEP_TIME);
        l_ret = e_continue;
        //printf("e_continue ---> transcode_audio_video_packet\n");
        if(1 == g_tranz_input_feed->m_is_eof) {
           l_ret = e_break;
        }
        return l_ret;
    }

    if (l_ret < 0) {
        if (l_ret == AVERROR_EOF){
            av_log(NULL, AV_LOG_ERROR, "EOF Reached\n");
            l_ret = e_break;
        } else {
            av_log(NULL, AV_LOG_ERROR, "Segmenter Queue Unknown error occured err: %s\n",
                                        av_err2str(l_ret));
            l_ret = e_unknown;
        }

        return l_ret;
     }

    l_stream_index = l_pkt.stream_index;
    l_codec_type = g_tranz_input_feed->m_ifmt_ctx->streams[l_stream_index]->codecpar->codec_type;


     //This section is for copying codecs
    if (g_tranz_input_feed->m_ifmt_ctx->streams[l_stream_index]->codecpar->codec_id == AV_CODEC_ID_H264 ||
        g_tranz_input_feed->m_ifmt_ctx->streams[l_stream_index]->codecpar->codec_id == AV_CODEC_ID_AAC) {

        l_pkt.pts = av_rescale_q_rnd(l_pkt.pts, g_tranz_input_feed->m_ifmt_ctx->streams[l_stream_index]->time_base,
                        g_tout_av_feed->m_aud_vid_ofmt_ctx->streams[l_stream_index]->time_base,
                        AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        l_pkt.dts = av_rescale_q_rnd(l_pkt.dts, g_tranz_input_feed->m_ifmt_ctx->streams[l_stream_index]->time_base,
                        g_tout_av_feed->m_aud_vid_ofmt_ctx->streams[l_stream_index]->time_base,
                        AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        l_pkt.duration = av_rescale_q(l_pkt.duration, g_tranz_input_feed->m_ifmt_ctx->streams[l_stream_index]->time_base,
                        g_tout_av_feed->m_aud_vid_ofmt_ctx->streams[l_stream_index]->time_base);
        l_pkt.pos = -1;

        av_log(
                NULL,
                AV_LOG_DEBUG,
                "Pkt: index: %d, duration: %" PRId64 "pts: %" PRId64 ", dts: %" PRId64 "\n",
                l_pkt.stream_index,
                l_pkt.duration,
                l_pkt.pts,
                l_pkt.dts);

        log_packet("Segmenter pts: ", g_tout_av_feed->m_aud_vid_ofmt_ctx, &l_pkt);

        l_ret = av_interleaved_write_frame(g_tout_av_feed->m_aud_vid_ofmt_ctx, &l_pkt);
        if (l_ret < 0) {
            if(AVERROR(EINVAL) == l_ret) {
                av_log(
                    NULL,
                    AV_LOG_ERROR,
                    "ERROR Pkt: index: %d, duration: %" PRId64 "pts: %" PRId64 ", dts: %" PRId64 "\n",
                    l_pkt.stream_index,
                    l_pkt.duration,
                    l_pkt.pts,
                    l_pkt.dts);
            } else {
                //av_packet_unref(&l_pkt);
                av_log(NULL, AV_LOG_ERROR, "ERROR HLS UNKNOWN write Error: err:%d, errstr:%s\n",
                        l_ret, av_err2str(l_ret));
                l_ret = e_write_error;
                return l_ret;
            }
        }

        if (l_codec_type == AVMEDIA_TYPE_VIDEO ) {
                    av_log(NULL, AV_LOG_DEBUG, "Copying Video Packets... \n");
        }
        else if (l_codec_type == AVMEDIA_TYPE_AUDIO ) {
            av_log(NULL, AV_LOG_DEBUG, "Copying Audio Packets... \n");
        }
        {
            static uint8_t l_x = 0;
            if(l_x == 0)
            {
                uint64_t l_time = av_gettime();
                uint64_t l_rtime = av_gettime_relative();
                printf("First Write Success --> Time: %" PRId64 " " "RTime: %" PRId64 "\n", l_time, l_rtime);
            }
            l_x ++;
        }
        l_ret = e_ok;
    }

    return l_ret;
}



static void *heartbeat(void *a_thread_arg)
{
    //#define HEART_SERVER_PATH "192.168.1.10"
    #define HEART_SERVER_PATH "tpf_unix_sock.server"
    #define HEART_DATA "hello\n"

    int l_count = 0;
    int client_socket, rc;
    //struct sockaddr_un remote;
    char l_hb_str[256] = {0};
    char buf[256] = {0};
    int l_hbl = 0;
    int l_idl = 0;
    sevent_header_t header;

    memset(&g_evt_param.m_remote, 0, sizeof(struct sockaddr_un));
    l_hbl = strlen(g_evt_param.m_hb_str);
    l_idl = strlen(g_evt_param.m_id_str);

    if(0 == l_hbl || 0 == l_idl ){
        return (void *) 0;
    } else {
        memset(l_hb_str, 0, sizeof(l_hb_str));
        memset(buf, 0, sizeof(buf));
        strcpy(buf, g_evt_param.m_id_str);
        strcpy(l_hb_str, g_evt_param.m_hb_str);
    }

#define HBIT_CYCLE  100 //1sec cycle
#define HBIT_DELAY  10000 //10 milli second

    while(0 == g_stop_processing) {
        l_count ++;
        av_usleep(HBIT_DELAY);
        if(l_count >= g_hb_to * HBIT_CYCLE) {
            l_count = 0;

            client_socket = socket(AF_UNIX, SOCK_DGRAM, 0);

            if (client_socket == -1) {
                av_log(NULL, AV_LOG_INFO, "Segmenter Heart bit SOCKET ERROR =\n");
                continue;
            }
            g_evt_param.m_client_socket = client_socket;

            g_evt_param.m_remote.sun_family = AF_UNIX;
            strcpy(g_evt_param.m_remote.sun_path, l_hb_str);

            strcpy(header.job_id, g_evt_param.m_id_str);
            header.event_type = g_evt_param.m_hb_argument;


            av_log(NULL, AV_LOG_INFO, "Heart beat Sending data...\n");
            //rc = sendto(client_socket, buf, strlen(buf), 0, (struct sockaddr *) &g_evt_param.m_remote,
            //            sizeof(g_evt_param.m_remote));
            rc = sendto(client_socket, &header, sizeof(header), 0, (struct sockaddr *) &g_evt_param.m_remote,
                        sizeof(g_evt_param.m_remote));
            if (rc == -1) {
                av_log(NULL, AV_LOG_INFO, "Segmenter Heart beat SENDTO ERROR\n");
                close(client_socket);

                continue;
            }
            else {
                av_log(NULL, AV_LOG_INFO, "Heart beat Data sent!\n");
            }
            rc = close(client_socket);
        }
    }
    g_stop_processing = 1;
    return (void *) 0;
}

static void notify_exit_event_to_processor(void)
{
    //--------------------------------------------------------------------------
    int client_socket, rc;
    char l_hb_str[256] = {0};
    int l_hbl = 0;
    int l_idl = 0;
    struct sockaddr_un remote;
    //--------------------------------------------------------------------------
    unsigned char buff[1024] = {0};
    int size = 0;
    sengine_exit_t smp_exit;

    smp_exit.restart_not_required = 0;

    sevent_header_t* header = (sevent_header_t*)buff;

    //------------------------------------------------------------------------------
    l_hbl = strlen(g_evt_param.m_hb_str);
    l_idl = strlen(g_evt_param.m_id_str);

    if(0 == l_hbl || 0 == l_idl ){
        return;
    } else {
        memset(l_hb_str, 0, sizeof(l_hb_str));
        strcpy(l_hb_str, g_evt_param.m_hb_str);
    }

    client_socket = socket(AF_UNIX, SOCK_DGRAM, 0);
    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, l_hb_str);

    //------------------------------------------------------------------------------


    //header->event_type = arg->exit_event; //number from argument -e
    header->event_type = g_evt_param.m_argument;
    strcpy(header->job_id, g_evt_param.m_id_str);


    size =  sizeof(*header);
    memcpy(buff + size, &smp_exit, sizeof(sengine_exit_t));
    size +=  sizeof(sengine_exit_t);


     sendto(client_socket, buff, size, 0, (struct sockaddr *) &remote,
            sizeof(struct sockaddr_un));

     rc = close(client_socket);

     av_log(NULL, AV_LOG_ERROR, "Engine quiting.\n");
}


static void log_packet(char *str, const AVFormatContext *fmt_ctx, const AVPacket *pkt)
{
    FILE *fp = NULL;



    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

    fp = fopen("segmenter_pts_log.txt","ab");

    fprintf(fp, str);
    fprintf(fp, ": ");
    /*
    fprintf(fp, "pts:%s pts_time:%s dts:%s dts_time:%s dur:%s dur_t:%s s_idx:%d, vdec: %d venc: %d "
            "snd smpl: %d, rev smpl: %d:: %d, %d, rcv:%" PRId64 ",proc:% " PRId64  "\n",
            av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
            av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
            av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
            pkt->stream_index,
            g_live_video_decode_frame,
            g_live_video_encode_frame,
            g_live_send_samples,
            g_live_receive_samples,

            g_live_audio_pkt_rcv,
            g_live_audio_pkt_proc,

            a_live_pkt_rcv_dtime,
            a_live_pkt_proc_dtime
            );
    */
        //fprintf(fp, "pts:%s pts_time:%s dur:%s dur_t:%s s_idx:%d\n",
        //    av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
        //    av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
        //    pkt->stream_index
        //    );
    fclose(fp);
}