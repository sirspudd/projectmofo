#include <libprojectM/projectM.hpp>
#include "abomination.hpp"

#include "projectmwindow.h"

/* Adopted from PulseAudio by carmelo.piccione+projectm@gmail.com

   Copyright 2004-2006 Lennart Poettering
   Copyright 2006 Pierre Ossman <ossman@cendio.se> for Cendio AB


 ***/

#include <signal.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>

#include <QSettings>
#include <QCoreApplication>

#define TIME_EVENT_USEC 50000

#if PA_API_VERSION < 9
#error Invalid PulseAudio API version
#endif

#define BUFSIZE 1024

AbominationFromTheDarkLordsTailPipe* AbominationFromTheDarkLordsTailPipe::pInstance = nullptr;

pa_context *AbominationFromTheDarkLordsTailPipe::context = NULL;
pa_stream *AbominationFromTheDarkLordsTailPipe::stream = NULL;
pa_mainloop_api *AbominationFromTheDarkLordsTailPipe::mainloop_api = NULL;
pa_time_event *AbominationFromTheDarkLordsTailPipe::time_event = NULL;
float *AbominationFromTheDarkLordsTailPipe::buffer = NULL;
size_t AbominationFromTheDarkLordsTailPipe::buffer_length = 0, AbominationFromTheDarkLordsTailPipe::buffer_index = 0;
pa_threaded_mainloop * AbominationFromTheDarkLordsTailPipe::mainloop = NULL;
pa_io_event * AbominationFromTheDarkLordsTailPipe::stdio_event = NULL;
char * AbominationFromTheDarkLordsTailPipe::server = NULL;
char * AbominationFromTheDarkLordsTailPipe::stream_name = NULL, *AbominationFromTheDarkLordsTailPipe::client_name = NULL, *AbominationFromTheDarkLordsTailPipe::device =0;
QMutex * AbominationFromTheDarkLordsTailPipe::s_audioMutex;
int AbominationFromTheDarkLordsTailPipe::verbose = 0;

pa_volume_t AbominationFromTheDarkLordsTailPipe::volume = PA_VOLUME_NORM;

pa_channel_map AbominationFromTheDarkLordsTailPipe::channel_map;
int AbominationFromTheDarkLordsTailPipe::channel_map_set = 0;
pa_sample_spec AbominationFromTheDarkLordsTailPipe::sample_spec ;

AbominationFromTheDarkLordsTailPipe::SourceContainer AbominationFromTheDarkLordsTailPipe::s_sourceList;
AbominationFromTheDarkLordsTailPipe::SourceContainer::const_iterator AbominationFromTheDarkLordsTailPipe::s_sourcePosition;

AbominationFromTheDarkLordsTailPipe::SourceContainer::const_iterator AbominationFromTheDarkLordsTailPipe::readSettings()
{

    QSettings settings ( "projectM", "qprojectM-pulseaudio" );

    bool tryFirst = settings.value
        ( "tryFirstAvailablePlaybackMonitor", true ).toBool() ;

    if ( tryFirst )
    {

        return s_sourceList.end();
    } else {

        QString deviceName = settings.value("pulseAudioDeviceName", QString()).toString();
        qDebug() << "device name is " << deviceName;
        for (SourceContainer::const_iterator pos = s_sourceList.begin();
                pos != s_sourceList.end(); ++pos) {
            if (*pos == deviceName) {
                return pos;
            }
        }
    }
    return s_sourceList.end();
}

void AbominationFromTheDarkLordsTailPipe::cleanup()
{
    pa_threaded_mainloop_stop ( mainloop );

    if ( stream )
        pa_stream_unref ( stream );

    if ( context )
        pa_context_unref ( context );


    if ( stdio_event )
    {
        assert ( mainloop_api );
        mainloop_api->io_free ( stdio_event );
    }


    if ( time_event )
    {
        assert ( mainloop_api );
        mainloop_api->time_free ( time_event );
    }

    if ( mainloop_api )
        mainloop_api->quit ( mainloop_api, 0 );

    if ( mainloop )
    {
        pa_signal_done();
        pa_threaded_mainloop_free ( mainloop );
    }

    if ( buffer )
        pa_xfree ( buffer );

    if ( server )
        pa_xfree ( server );

    if ( device )
        pa_xfree ( device );

    if ( client_name )
        pa_xfree ( client_name );

    if ( stream_name )
        pa_xfree ( stream_name );

    return ;
}

void AbominationFromTheDarkLordsTailPipe::connectHelper (SourceContainer::const_iterator pos)
{
    Q_ASSERT(stream);
    pa_stream_flags_t flags = ( pa_stream_flags_t ) 0;
    //	qDebug() << "start2 ";
    assert (pos != s_sourceList.end());
    qDebug() << "connectHelper: " << *pos;
    int r;
    if ( ( ( r = pa_stream_connect_record (stream, (*pos).toStdString().c_str(), NULL, flags ) ) ) < 0 )
    {
        fprintf ( stderr, "pa_stream_connect_record() failed: %s\n", pa_strerror ( pa_context_errno ( context ) ) );
    }


}

AbominationFromTheDarkLordsTailPipe::SourceContainer::const_iterator AbominationFromTheDarkLordsTailPipe::scanForPlaybackMonitor() {

    for ( SourceContainer::const_iterator pos = s_sourceList.begin(); pos != s_sourceList.end(); ++pos )
    {
        if ( ( *pos ).contains ( "monitor" ) && (*pos).contains("playback"))
        {
            return pos;
        }
    }

    return s_sourceList.end();

}

void AbominationFromTheDarkLordsTailPipe::connectDevice ( const QModelIndex & index )
{

    if (index.isValid())
        reconnect(s_sourceList.begin() + index.row());
    else
        reconnect(s_sourceList.end());

    emit(deviceChanged());
}


void AbominationFromTheDarkLordsTailPipe::reconnect(SourceContainer::const_iterator pos = s_sourceList.end()) {

    if (s_sourceList.empty())
        return;

    if (pos != s_sourceList.end()) {
        s_sourcePosition = pos;
        qDebug() << "reconnecting with" << *pos;
    }
    else
        s_sourcePosition = scanForPlaybackMonitor();

    if (s_sourcePosition == s_sourceList.end()) {
        s_sourcePosition = s_sourceList.begin();
    }

    if (stream && (pa_stream_get_state(stream) == PA_STREAM_READY))
    {
        //qDebug() << "disconnect";
        pa_stream_disconnect ( stream );
        //	pa_stream_unref(stream);
        //qDebug() << "* return *";

    }

    if ( ! ( stream = pa_stream_new ( context, stream_name, &sample_spec, channel_map_set ? &channel_map : NULL ) ) )
    {
        fprintf ( stderr, "pa_stream_new() failed: %s\n", pa_strerror ( pa_context_errno ( context ) ) );
        return;
    }

    pa_stream_set_state_callback
        ( stream, stream_state_callback, &s_sourceList );
    pa_stream_set_read_callback ( stream, stream_read_callback, &s_sourceList );
    pa_stream_set_moved_callback(stream, stream_moved_callback, &s_sourceList );

    switch (pa_stream_get_state(stream)) {
        case PA_STREAM_UNCONNECTED:// 	The stream is not yet connected to any sink or source.
            qDebug() << "unconnected: connecting...";
            connectHelper(s_sourcePosition);
            break;
        case PA_STREAM_CREATING 	://The stream is being created.
            break;
        case PA_STREAM_READY ://	The stream is established, you may pass audio data to it now.
            qDebug() << "stream is still ready, waiting for callback...";
            break;
        case PA_STREAM_FAILED ://	An error occured that made the stream invalid.
            qDebug() << "stream is now invalid. great.";
            break;
        case PA_STREAM_TERMINATED:// 	The stream has been terminated cleanly.
            qDebug() << "terminated...";
            break;

    }
}


void AbominationFromTheDarkLordsTailPipe::pa_stream_success_callback(pa_stream *s, int success, void * data) {

    static bool pausedFlag = true;

    if (pausedFlag)
        qDebug() << "pause";
    else {
        qDebug() << "play";
    }


    pausedFlag = !pausedFlag;

    // necessarily static
    // can do pulse stuff here...
}


QMutex * AbominationFromTheDarkLordsTailPipe::mutex() {
    return s_audioMutex;
}

AbominationFromTheDarkLordsTailPipe *AbominationFromTheDarkLordsTailPipe::instance(QObject *p)
{
    if (!pInstance) {
        pInstance = new AbominationFromTheDarkLordsTailPipe(p);
    }
    return pInstance;
}

void AbominationFromTheDarkLordsTailPipe::cork()
{
    int b = 0;

    pa_operation* op =
        pa_stream_cork(stream, b, pa_stream_success_callback, this);

    if (op)
        pa_operation_unref(op);
    else
        qDebug() << "cork operation null";
}


/* A shortcut for terminating the application */
void AbominationFromTheDarkLordsTailPipe::pulseQuit ( int ret )
{
    assert ( mainloop_api );
    mainloop_api->quit ( mainloop_api, ret );

}


/* This is called whenever new data may is available */
void AbominationFromTheDarkLordsTailPipe::stream_read_callback ( pa_stream *s, size_t length, void *userdata )
{
    const void *data;
    assert ( s && length );

    if ( stdio_event )
        mainloop_api->io_enable ( stdio_event, PA_IO_EVENT_OUTPUT );

    if ( pa_stream_peek ( s, &data, &length ) < 0 )
    {
        fprintf ( stderr, "pa_stream_peek() failed: %s\n", pa_strerror ( pa_context_errno ( context ) ) );
        pulseQuit ( 1 );
        return
            ;
    }

    assert ( data && length );

    if ( buffer )
    {
        fprintf ( stderr, "Buffer overrun, dropping incoming data\n" );
        if ( pa_stream_drop ( s ) < 0 )
        {
            fprintf ( stderr, "pa_stream_drop() failed: %s\n", pa_strerror ( pa_context_errno ( context ) ) );
            pulseQuit ( 1 );
        }
        return;
    }

    emit pInstance->pcmDataGenerated((float*)(data), length /(sizeof (float)));

    //buffer = ( float* ) pa_xmalloc ( buffer_length = length );
    //memcpy ( buffer, data, length );
    buffer_index = 0;
    pa_stream_drop ( s );
}

/* This routine is called whenever the stream state changes */
void AbominationFromTheDarkLordsTailPipe::stream_state_callback ( pa_stream *s, void *userdata )
{
    assert ( s );
    AbominationFromTheDarkLordsTailPipe * thread = (AbominationFromTheDarkLordsTailPipe *)userdata;

    switch ( pa_stream_get_state ( s ) )
    {
        case PA_STREAM_UNCONNECTED:
            qDebug() << "UNCONNECTED";
            break;
        case PA_STREAM_CREATING:
            qDebug() << "CREATED";
            break;
        case PA_STREAM_TERMINATED:
            qDebug() << "TERMINATED";
            break;
        case PA_STREAM_READY:
            qDebug() << "READY";
            if ( verbose )
            {
                const pa_buffer_attr *a;
                fprintf ( stderr, "Stream successfully created.\n" );

                if ( ! ( a = pa_stream_get_buffer_attr ( s ) ) )
                    fprintf ( stderr, "pa_stream_get_buffer_attr() failed: %s\n", pa_strerror ( pa_context_errno ( pa_stream_get_context ( s ) ) ) );
                else
                {
                    fprintf(stderr, "Buffer metrics: maxlength=%u, fragsize=%u\n", a->maxlength, a->fragsize);
                }
            }
            break;
        case PA_STREAM_FAILED:
            qDebug() << "FAILED";
        default:
            fprintf ( stderr, "Stream error: %s\n", pa_strerror ( pa_context_errno ( pa_stream_get_context ( s ) ) ) );
            pulseQuit ( 1 );
    }
}


void AbominationFromTheDarkLordsTailPipe::stream_moved_callback(pa_stream *s, void *userdata) {
    Q_ASSERT(s);

    if (verbose)
        fprintf(stderr, "Stream moved to device %s (%u, %ssuspended).\n", pa_stream_get_device_name(s), pa_stream_get_device_index(s), pa_stream_is_suspended(s) ? "" : "not ");
}

/* This is called whenever the context status changes */
void AbominationFromTheDarkLordsTailPipe::context_state_callback ( pa_context *c, void *userdata )
{
    assert ( c );

    switch ( pa_context_get_state ( c ) )
    {
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
            break;

        case PA_CONTEXT_READY:
            {
                int r;

                assert ( c && !stream );

                if ( verbose )
                    fprintf ( stderr, "Connection established.\n" );
                /*
                   if ( ! ( stream = pa_stream_new ( c, stream_name, &sample_spec, channel_map_set ? &channel_map : NULL ) ) )
                   {
                   fprintf ( stderr, "pa_stream_new() failed: %s\n", pa_strerror ( pa_context_errno ( c ) ) );
                   goto fail;
                   }
                   */
                initialize_callbacks ( ( AbominationFromTheDarkLordsTailPipe * ) userdata );

                //			pa_stream_set_state_callback
                //				( stream, stream_state_callback, userdata );
                //			pa_stream_set_read_callback ( stream, stream_read_callback, userdata );
                //			pa_stream_set_moved_callback(stream, stream_moved_callback, userdata );
                break;
            }

        case PA_CONTEXT_TERMINATED:
            pulseQuit ( 0 );
            break;

        case PA_CONTEXT_FAILED:
        default:
            fprintf ( stderr, "Connection failure: %s\n", pa_strerror ( pa_context_errno ( c ) ) );
            goto fail;
    }

    return;

fail:
    pulseQuit ( 1 );

}

/* Connection draining complete */
void AbominationFromTheDarkLordsTailPipe::context_drain_complete ( pa_context*c, void *userdata )
{
    pa_context_disconnect ( c );
}

/* Some data may be written to STDOUT */
void AbominationFromTheDarkLordsTailPipe::stdout_callback ( pa_mainloop_api*a, pa_io_event *e, int fd, pa_io_event_flags_t f, void *userdata )
{

    assert ( a == mainloop_api && e && stdio_event == e );

    if ( !buffer )
    {
        mainloop_api->io_enable ( stdio_event, PA_IO_EVENT_NULL );
        return;
    }
    else
    {


        //s_audioMutex->lock();

        //QProjectM * prjm = (*qprojectMWidgetPtr)->qprojectM();

        //if (prjm == 0) {
        //	s_audioMutex->unlock();
        //	return;
        //}

        //Q_ASSERT(prjm);
        //Q_ASSERT(prjm->pcm());
        Q_ASSERT(buffer);

        emit pInstance->pcmDataGenerated((float*)(buffer+buffer_index), buffer_length /(sizeof (float)));
        //s_audioMutex->unlock();
        //qDebug() << "UNLOCK: add pcm";
        //assert ( buffer_length );

        pa_xfree ( buffer );
        buffer = NULL;
        buffer_length = buffer_index = 0;
    }
}

/* UNIX signal to quit recieved */
void AbominationFromTheDarkLordsTailPipe::exit_signal_callback ( pa_mainloop_api*m, pa_signal_event *e, int sig, void *userdata )
{
    if ( verbose )
        fprintf ( stderr, "Got signal, exiting.\n" );

    pulseQuit ( 0 );
}

/* Show the current latency */
void AbominationFromTheDarkLordsTailPipe::stream_update_timing_callback ( pa_stream *s, int success, void *userdata )
{
    pa_usec_t latency, usec;
    int negative = 0;

    assert ( s );

    if ( !success ||
            pa_stream_get_time ( s, &usec ) < 0 ||
            pa_stream_get_latency ( s, &latency, &negative ) < 0 )
    {
        fprintf ( stderr, "Failed to get latency: %s\n", pa_strerror ( pa_context_errno ( context ) ) );
        pulseQuit ( 1 );
        return;
    }

    fprintf ( stderr, "Time: %0.3f sec; Latency: %0.0f usec.  \r",
            ( float ) usec / 1000000,
            ( float ) latency * ( negative?-1:1 ) );
}

/* Someone requested that the latency is shown */
void AbominationFromTheDarkLordsTailPipe::sigusr1_signal_callback ( pa_mainloop_api*m, pa_signal_event *e, int sig, void *userdata )
{

    if ( !stream )
        return;

    pa_operation_unref ( pa_stream_update_timing_info ( stream, stream_update_timing_callback, NULL ) );
}

void AbominationFromTheDarkLordsTailPipe::pa_source_info_callback ( pa_context *c, const pa_source_info *i, int eol, void *userdata )
{

    assert ( userdata );
    AbominationFromTheDarkLordsTailPipe * pulseThread = ( AbominationFromTheDarkLordsTailPipe* ) userdata;

    if ( i )
    {
        int index = i->index;
        QString name = i->name;

        pulseThread->insertSource ( index,name );
    }

    else
    {
        assert ( eol );

        SourceContainer::const_iterator pos = readSettings();
        reconnect(pos);
    }

}

void AbominationFromTheDarkLordsTailPipe::subscribe_callback ( struct pa_context *c, enum pa_subscription_event_type t, uint32_t index, void *userdata )
{

    AbominationFromTheDarkLordsTailPipe * thread = static_cast<AbominationFromTheDarkLordsTailPipe *>(userdata);
    switch ( t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK )
    {
        case PA_SUBSCRIPTION_EVENT_SINK:
            break;
        case PA_SUBSCRIPTION_EVENT_SOURCE:
            {
                if ( ( t & PA_SUBSCRIPTION_EVENT_TYPE_MASK ) == PA_SUBSCRIPTION_EVENT_REMOVE ) {
                    qDebug() << "Warning! untested code. email carmelo.piccione@gmail.com if it explodes";
                    SourceContainer::const_iterator pos = s_sourceList.find(index);
                    if (pos == s_sourcePosition) {
                        s_sourceList.remove(index);
                        reconnect();
                        thread->deviceChanged();
                    } else {
                        s_sourceList.remove(index);
                        thread->deviceChanged();
                    }
                }
                else
                {
                    pa_operation_unref ( pa_context_get_source_info_by_index ( context, index, pa_source_info_callback, userdata ) );
                }
                break;
            }
        case PA_SUBSCRIPTION_EVENT_MODULE:
            break;
        case PA_SUBSCRIPTION_EVENT_CLIENT:
            break;
        case PA_SUBSCRIPTION_EVENT_SINK_INPUT:
            break;
        case PA_SUBSCRIPTION_EVENT_SOURCE_OUTPUT:
            break;
        case PA_SUBSCRIPTION_EVENT_SAMPLE_CACHE:
            break;
        case PA_SUBSCRIPTION_EVENT_SERVER:
            break;
        default:
            fprintf ( stderr, "OTHER EVENT\n" );
            break;

    }
}


void AbominationFromTheDarkLordsTailPipe::time_event_callback ( pa_mainloop_api*m, pa_time_event *e, const struct timeval *tv, void *userdata )
{
    struct timeval next;

    if ( stream && pa_stream_get_state ( stream ) == PA_STREAM_READY )
    {
        pa_operation *o;
        if ( ! ( o = pa_stream_update_timing_info ( stream, stream_update_timing_callback, NULL ) ) )
            fprintf ( stderr, "pa_stream_update_timing_info() failed: %s\n", pa_strerror ( pa_context_errno ( context ) ) );
        else
            pa_operation_unref ( o );
    }

    pa_gettimeofday ( &next );
    pa_timeval_add ( &next, TIME_EVENT_USEC );

    m->time_restart ( e, &next );
}

void AbominationFromTheDarkLordsTailPipe::run()
{

    const char * error = "FOO";
    int ret = 1, r, c;
    const char *bn = "projectM";
    pa_operation * operation ;
    sample_spec.format = PA_SAMPLE_FLOAT32LE;
    sample_spec.rate = 44100;
    sample_spec.channels = 2;
    pa_context_flags_t flags = ( pa_context_flags_t ) 0;

    verbose = 0;

    if ( !pa_sample_spec_valid ( &sample_spec ) )
    {
        fprintf ( stderr, "Invalid sample specification\n" );
        goto quit;
    }

    if ( channel_map_set && channel_map.channels != sample_spec.channels )
    {
        fprintf ( stderr, "Channel map doesn't match sample specification\n" );
        goto quit;
    }

    if ( verbose )
    {
        char t[PA_SAMPLE_SPEC_SNPRINT_MAX];
        pa_sample_spec_snprint ( t, sizeof ( t ), &sample_spec );
        fprintf ( stderr, "Opening a %s stream with sample specification '%s'.\n", "recording" , t );
    }


    if ( !client_name )
        client_name = pa_xstrdup ( bn );

    //printf("client name:%s", client_name);
    if ( !stream_name )
        stream_name = pa_xstrdup ( client_name );

    /* Set up a new main loop */
    if ( ! ( mainloop = pa_threaded_mainloop_new() ) )
    {
        fprintf ( stderr, "pa_mainloop_new() failed.\n" );
        goto quit;
    }

    mainloop_api = pa_threaded_mainloop_get_api ( mainloop );

    r = pa_signal_init ( mainloop_api );
    assert ( r == 0 );
    pa_signal_new ( SIGINT, exit_signal_callback, NULL );
    pa_signal_new ( SIGTERM, exit_signal_callback, NULL );

#ifdef SIGUSR1
    pa_signal_new ( SIGUSR1, sigusr1_signal_callback, NULL );
#endif
#ifdef SIGPIPE
    signal ( SIGPIPE, SIG_IGN );
#endif
    /*
       if ( ! ( stdio_event = mainloop_api->io_new ( mainloop_api,
       STDOUT_FILENO,
       PA_IO_EVENT_OUTPUT,
       stdout_callback, s_qprojectM_MainWindowPtr ) ) )
       {
       fprintf ( stderr, "io_new() failed.\n" );
       goto quit;
       }
    /*
    /* Create a new connection context */
    if ( ! ( context = pa_context_new ( mainloop_api, client_name ) ) )
    {
        fprintf ( stderr, "pa_context_new() failed.\n" );
        goto quit;
    }

    pa_context_set_state_callback ( context, context_state_callback, &s_sourceList );
    pa_context_connect ( context, server, flags, NULL );

    if ( verbose )
    {
        struct timeval tv;

        pa_gettimeofday ( &tv );
        pa_timeval_add ( &tv, TIME_EVENT_USEC );

        if ( ! ( time_event = mainloop_api->time_new ( mainloop_api, &tv, time_event_callback, NULL ) ) )
        {
            fprintf ( stderr, "time_new() failed.\n" );
            goto quit;
        }
    }


    /* Run the main loop */
    if ( pa_threaded_mainloop_start ( mainloop ) < 0 )
    {
        fprintf ( stderr, "pa_mainloop_run() failed.\n" );
        goto quit;
    }
quit:
    emit(threadCleanedUp());
    return ;
}

void AbominationFromTheDarkLordsTailPipe::initialize_callbacks ( AbominationFromTheDarkLordsTailPipe * pulseThread )
{
    pa_operation * op;
    pa_operation_unref
        ( pa_context_get_source_info_list ( context, pa_source_info_callback, pulseThread ) );

    //pa_operation_unref(pa_context_get_server_info(&c, server_info_callback, this));
    //pa_operation_unref(pa_context_get_sink_info_list(&c, sink_info_callback, this));
    //pa_operation_unref(pa_context_get_source_info_list(&c, source_info_callback, this));
    //pa_operation_unref(pa_context_get_module_info_list(&c, module_info_callback, this));
    //pa_operation_unref(pa_context_get_client_info_list(&c, client_info_callback, this));
    //pa_operation_unref(pa_context_get_sink_input_info_list(&c, sink_input_info_callback, this));
    //pa_operation_unref(pa_context_get_source_output_info_list(&c, source_output_info_callback, this));
    //pa_operation_unref(pa_context_get_sample_info_list(&c, sample_info_callback, this));


    //pa_context_set_drain
    pa_context_set_subscribe_callback ( context, subscribe_callback, pulseThread );

    if ( op = pa_context_subscribe ( context, ( enum pa_subscription_mask )
                PA_SUBSCRIPTION_MASK_SOURCE_OUTPUT, NULL, NULL ) )
    {
        pa_operation_unref ( op );
    }
    else
    {
        qDebug() << "null op returned on subscribe";
    }

}
