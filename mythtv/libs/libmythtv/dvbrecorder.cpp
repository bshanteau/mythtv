/*
 *  Class DVBRecorder
 *
 *  Copyright (C) Daniel Thor Kristjansson 2010
 *
 *   This class glues the DVBStreamHandler which handles the DVB devices
 *   to the DTVRecorder that handles recordings in MythTV.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// MythTV includes
#include "dvbstreamhandler.h"
#include "mpegstreamdata.h"
#include "dvbrecorder.h"
#include "dvbchannel.h"
#include "tv_rec.h"

#define LOC      QString("DVBRec(%1:%2): ") \
                 .arg(tvrec->GetCaptureCardNum()).arg(videodevice)
#define LOC_WARN QString("DVBRec(%1:%2) Warning: ") \
                 .arg(tvrec->GetCaptureCardNum()).arg(videodevice)
#define LOC_ERR  QString("DVBRec(%1:%2) Error: ") \
                 .arg(tvrec->GetCaptureCardNum()).arg(videodevice)

DVBRecorder::DVBRecorder(TVRec *rec, DVBChannel *channel)
    : DTVRecorder(rec), _channel(channel), _stream_handler(NULL)
{
    videodevice = QString::null;
}

bool DVBRecorder::Open(void)
{
    if (IsOpen())
    {
        VERBOSE(VB_GENERAL, LOC_WARN + "Card already open");
        return true;
    }

    if (videodevice.isEmpty())
        return false;

    memset(_stream_id,  0, sizeof(_stream_id));
    memset(_pid_status, 0, sizeof(_pid_status));
    memset(_continuity_counter, 0xff, sizeof(_continuity_counter));

    _stream_handler = DVBStreamHandler::Get(videodevice);

    VERBOSE(VB_RECORD, LOC + "Card opened successfully");

    return true;
}

bool DVBRecorder::IsOpen(void) const
{
    return (NULL != _stream_handler);
}

void DVBRecorder::Close(void)
{
    VERBOSE(VB_RECORD, LOC + "Close() -- begin");

    DVBStreamHandler::Return(_stream_handler);

    VERBOSE(VB_RECORD, LOC + "Close() -- end");
}

void DVBRecorder::StartRecording(void)
{
    if (!Open())
    {
        _error = "Failed to open DVB device";
        VERBOSE(VB_IMPORTANT, LOC_ERR + _error);
        return;
    }

    _continuity_error_count = 0;

    {
        QMutexLocker locker(&pauseLock);
        request_recording = true;
        recording = true;
        recordingWait.wakeAll();
    }

    // Listen for time table on DVB standard streams
    if (_channel && (_channel->GetSIStandard() == "dvb"))
        _stream_data->AddListeningPID(DVB_TDT_PID);

    // Make sure the first things in the file are a PAT & PMT
    bool tmp = _wait_for_keyframe_option;
    _wait_for_keyframe_option = false;
    HandleSingleProgramPAT(_stream_data->PATSingleProgram());
    HandleSingleProgramPMT(_stream_data->PMTSingleProgram());
    _wait_for_keyframe_option = tmp;

    _stream_data->AddAVListener(this);
    _stream_data->AddWritingListener(this);
    _stream_handler->AddListener(_stream_data, false, true);

    while (IsRecordingRequested() && !IsErrored())
    {
        if (PauseAndWait())
            continue;

        {   // sleep 100 milliseconds unless StopRecording() or Unpause()
            // is called, just to avoid running this too often.
            QMutexLocker locker(&pauseLock);
            if (!request_recording || request_pause)
                continue;
            unpauseWait.wait(&pauseLock, 100);
        }

        if (!_input_pmt)
        {
            VERBOSE(VB_GENERAL, LOC_WARN +
                    "Recording will not commence until a PMT is set.");
            usleep(5000);
            continue;
        }

        if (!_stream_handler->IsRunning())
        {
            _error = "Stream handler died unexpectedly.";
            VERBOSE(VB_IMPORTANT, LOC_ERR + _error);
        }
    }

    _stream_handler->RemoveListener(_stream_data);
    _stream_data->RemoveWritingListener(this);
    _stream_data->RemoveAVListener(this);

    Close();

    FinishRecording();

    QMutexLocker locker(&pauseLock);
    recording = false;
    recordingWait.wakeAll();
}

bool DVBRecorder::PauseAndWait(int timeout)
{
    QMutexLocker locker(&pauseLock);
    if (request_pause)
    {
        if (!IsPaused(true))
        {
            _stream_handler->RemoveListener(_stream_data);

            paused = true;
            pauseWait.wakeAll();
            if (tvrec)
                tvrec->RecorderPaused();
        }

        unpauseWait.wait(&pauseLock, timeout);
    }

    if (!request_pause && IsPaused(true))
    {
        paused = false;
        _stream_handler->AddListener(_stream_data, false, true);
        unpauseWait.wakeAll();
    }

    return IsPaused(true);
}

QString DVBRecorder::GetSIStandard(void) const
{
    return _channel->GetSIStandard();
}

void DVBRecorder::SetCAMPMT(const ProgramMapTable *pmt)
{
    _channel->SetPMT(pmt);
}

void DVBRecorder::UpdateCAMTimeOffset(void)
{
    _channel->SetTimeOffset(GetStreamData()->TimeOffset());
}

/* vim: set expandtab tabstop=4 shiftwidth=4: */
