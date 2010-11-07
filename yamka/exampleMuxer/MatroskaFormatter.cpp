// -*- Mode: c++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

#ifdef _WIN32
#define _USE_MATH_DEFINES
#define NOMINMAX
#endif

// local includes:
#include "MatroskaFormatter.h"

// yamka includes:
#include <yamka.h>

// Ogg includes:
#include <ogg/ogg.h>

// system includes:
#include <algorithm>
#include <map>
#include <math.h>


//----------------------------------------------------------------
// TIMECODE_SCALE
// 
static const Yamka::uint64 TIMECODE_SCALE = 1000000;

//----------------------------------------------------------------
// NANOSEC_PER_SEC
// 
static const Yamka::uint64 NANOSEC_PER_SEC = 1000000000;

//----------------------------------------------------------------
// TMatroskaFormatter::TMatroskaFormatter
// 
TMatroskaFormatter::TMatroskaFormatter(TFileFormat fileFormat):
    m_matroska(fileFormat)
{
	m_videoCodec = NULL;
	m_audioCodec = NULL;
}

//----------------------------------------------------------------
// TMatroskaFormatter::~TMatroskaFormatter
// 
TMatroskaFormatter::~TMatroskaFormatter()
{
    endFormatting();
}

//----------------------------------------------------------------
// TMatroskaFormatter::setVideoCompressor
// 
void
TMatroskaFormatter::setVideoCompressor(const AVideoCompressor * vc)
{
    m_videoCodec = vc;
}

//----------------------------------------------------------------
// TMatroskaFormatter::setAudioCompressor
// 
void
TMatroskaFormatter::setAudioCompressor(const AAudioCompressor * ac)
{
    m_audioCodec = ac;
}

//----------------------------------------------------------------
// TMatroskaFormatter::beginFormatting
// 
TResult
TMatroskaFormatter::beginFormatting()
{
    // make sure we know what we are muxing:
    const char * videoCodecID = getVideoCodecID();
    if (m_videoCodec && !videoCodecID)
    {
        return OOPS_FORMAT_UNKNOWN_VIDEO_CODEC;
    }
    
    const char * audioCodecID = getAudioCodecID();
    if (m_audioCodec && !audioCodecID)
    {
        return OOPS_FORMAT_UNKNOWN_AUDIO_CODEC;
    }
    
    std::string filenameUtf8(TFormatterBase::m_outputFile.c_str());
    m_yamkaTmp = Yamka::FileStorage(filenameUtf8 + ".yamka",
                                    Yamka::File::kReadWrite);
    if (!m_yamkaTmp.file_.isOpen())
	{
		return OOPS_FORMAT_FILE_CREATE_FAILED;
	}
    
    m_yamkaOut = Yamka::FileStorage(filenameUtf8, Yamka::File::kReadWrite);
    if (!m_yamkaOut.file_.isOpen())
	{
		return OOPS_FORMAT_FILE_CREATE_FAILED;
	}
    
    // truncate the output files, in case they already existed:
    m_yamkaTmp.file_.setSize(0);
    m_yamkaOut.file_.setSize(0);
    
    // reset the matroska document:
    m_matroska = Yamka::WebMediaDoc(m_matroska.fileFormat_);
    
    // add a segment:
    m_matroska.segments_.push_back(TSegment());
    TSegment & segment = currentSegment();
    
    // add first seekhead, written before clusters:
    segment.payload_.seekHeads_.push_back(TSeekHead());
    TSeekHead & seekHead1 = currentSeekHead();
    
    // fill the mandatory segment info element:
    TSegInfo & segInfo = segment.payload_.info_;
    segInfo.payload_.timecodeScale_.payload_.set(TIMECODE_SCALE);
    segInfo.payload_.writingApp_.payload_.set("MyGreatMuxerApp 1.0");
    
    Yamka::TByteVec segUID = Yamka::createUID(16);
    segInfo.payload_.segUID_.payload_.set(Yamka::Bytes(segUID));
    
    // shortcuts:
    TTracks & tracks = segment.payload_.tracks_;
    
    // index the first set of top-level elements:
    seekHead1.payload_.indexThis(&segment, &segInfo, m_yamkaTmp);
    seekHead1.payload_.indexThis(&segment, &tracks, m_yamkaTmp);
    
    if (m_encodeVideo)
    {
        // add video track, fill video track entry params:
        tracks.payload_.tracks_.push_back(TTrackEntry());
        Yamka::Track & videoTrack = tracks.payload_.tracks_.back().payload_;
        
        Yamka::TByteVec trackUID = Yamka::createUID(16);
        videoTrack.trackUID_.payload_.set(Yamka::uintDecode(trackUID, 16) >> 7);
        
        videoTrack.frameDuration_.payload_.set(Yamka::uint64(double(NANOSEC_PER_SEC) *
                                                             getVideoFrameDuration()));
        videoTrack.timecodeScale_.payload_.set(1);
        videoTrack.trackNumber_.payload_.set(kVideoTrackNumber);
        
        // set track type:
        videoTrack.trackType_.payload_.set(Yamka::Track::kTrackTypeVideo);
        
        // set track codec ID:
        videoTrack.codecID_.payload_.set(videoCodecID);
        
        // set track codec name:
        videoTrack.codecName_.payload_.set(m_videoCodec->getName());
        
        // set video specific params:
        Yamka::Video & video = videoTrack.video_.payload_;
        video.pixelWidth_.payload_.set(getVideoPixelWidth());
        video.pixelHeight_.payload_.set(getVideoPixelHeight());
    }
    
    if (m_encodeAudio)
    {
        // add audio track, fill audio track entry params:
        tracks.payload_.tracks_.push_back(TTrackEntry());
        Yamka::Track & audioTrack = tracks.payload_.tracks_.back().payload_;
        
        Yamka::TByteVec trackUID = Yamka::createUID(16);
        audioTrack.trackUID_.payload_.set(Yamka::uintDecode(trackUID, 16) >> 7);
        
        audioTrack.timecodeScale_.payload_.set(1);
        audioTrack.trackNumber_.payload_.set(kAudioTrackNumber);
        
        if (m_matroska.fileFormat_ == kFileFormatWebMedia)
        {
            // NOTE: WebM doesn't support lacing, yet:
            audioTrack.flagLacing_.payload_.set(0);
        }
        else
        {
            audioTrack.flagLacing_.payload_.set(1);
        }
        
        // set track type:
        audioTrack.trackType_.payload_.set(Yamka::Track::kTrackTypeAudio);
        
        // set track codec ID:
        audioTrack.codecID_.payload_.set(audioCodecID);
        
        // set track codec name:
        audioTrack.codecName_.payload_.set(m_audioCodec->getName());
        
        // set audio specific params:
        Yamka::Audio & audio = audioTrack.audio_.payload_;
        audio.sampFreq_.payload_.set(double(getAudioSampleRate()));
        audio.channels_.payload_.set(getAudioChannels());
    }
    
    return NO_PROBLEM;
}

//----------------------------------------------------------------
// TMatroskaFormatter::addVideoSample
// 
TResult
TMatroskaFormatter::addVideoSample(TEncImage * encImage)
{
    if (!encImage)
    {
        // EOF:
        return NO_PROBLEM;
    }
    
    m_videoFrames.push_back(Yamka::Frame());
    Yamka::Frame & frame = m_videoFrames.back();

    double fps = getVideoFrameRate();
    Yamka::uint64 frameDuration = (fps == floor(fps)) ? 1000 : 1001;
    Yamka::uint64 frameTimeBase = Yamka::uint64(double(frameDuration) * fps +
                                                0.5);
    
    Yamka::uint64 frameTime =
        frameTimeBase == srcImage->unTimeScale ?
        srcImage->unTime :
        Yamka::uint64(double(srcImage->unTime) * frameDuration /
                      double(srcImage->unTimeScale));
    
    frame.trackNumber_ = kVideoTrackNumber;
    frame.ts_.start_ = frameTime;
    frame.ts_.extent_ = frameDuration;
    frame.ts_.base_ = frameTimeBase;
    frame.isKeyframe_ = (encImage->unFlag == eFT_KEY_FRAME);
    
    frame.data_.set(Yamka::Bytes(encImage->pImage, encImage->unBitStreamLength),
                    m_yamkaOut);
    
    return NO_PROBLEM;
}

//----------------------------------------------------------------
// TMatroskaFormatter::addAudioSample
// 
TResult
TMatroskaFormatter::addAudioSample(TEncAudio * encAudio)
{
    if (!encAudio || !encAudio->unBitStreamLength)
    {
        // EOF:
        return NO_PROBLEM;
    }
    
    const unsigned int audioSampleRate = getAudioSampleRate();
    
    TVendorId vendorId = 0;
    TCodecId  codecId = 0;
    m_audioCodec->getAudioCompressorInfo(vendorId, codecId);
    
    if (codecId == kAudioCodecVorbis)
    {
		const ogg_packet * oggPackets = (const ogg_packet *)(encAudio->pData);
		const uint32_st numOggPackets = encAudio->unNumberOfMediaSamples;
        
        if (!m_audioFrames.empty() && numOggPackets)
        {
            const ogg_packet & p0 = oggPackets[0];
            
            Yamka::Frame & frame = m_audioFrames.back();
            frame.ts_.extent_ = p0.granulepos - frame.ts_.start_;
        }
        
        for (uint32_st i = 1; i < numOggPackets; i++)
        {
            m_audioFrames.push_back(Yamka::Frame());
            Yamka::Frame & frame = m_audioFrames.back();
            frame.trackNumber_ = kAudioTrackNumber;
            
            const ogg_packet & p0 = oggPackets[i - 1];
            const ogg_packet & p1 = oggPackets[i];
            frame.ts_.start_ = p0.granulepos;
            frame.ts_.extent_ = p1.granulepos - p0.granulepos;
            frame.ts_.base_ = audioSampleRate;
            frame.data_.set(Yamka::Bytes(p0.packet, p0.bytes), m_yamkaOut);
        }
        
        // we don't know duration of the last packet until the next
        // batch of packets comes in:
        {
            m_audioFrames.push_back(Yamka::Frame());
            Yamka::Frame & frame = m_audioFrames.back();
            frame.trackNumber_ = kAudioTrackNumber;
            
            const ogg_packet & p0 = oggPackets[numOggPackets - 1];
            frame.ts_.start_ = p0.granulepos;
            frame.ts_.base_ = audioSampleRate;
            frame.data_.set(Yamka::Bytes(p0.packet, p0.bytes), m_yamkaOut);
        }
    }
    else
    {
        Yamka::Frame prevFrame =
            m_audioFrames.empty() ?
            Yamka::Frame() :
            m_audioFrames.back();
        
        m_audioFrames.push_back(Yamka::Frame());
        Yamka::Frame & frame = m_audioFrames.back();
        
        frame.trackNumber_ = kAudioTrackNumber;
        frame.ts_.start_ = prevFrame.ts_.start_ + prevFrame.ts_.extent_;
        frame.ts_.extent_ = encAudio->unNumberSamples;
        frame.ts_.base_ = audioSampleRate;
        frame.data_.set(Yamka::Bytes(encAudio->pData, encAudio->unBitStreamLength),
                        m_yamkaOut);
    }
    
    return NO_PROBLEM;
}

//----------------------------------------------------------------
// TCompareFrames
// 
static bool
TCompareFrames(const Yamka::Frame & a, const Yamka::Frame & b)
{
    double ta = double(a.ts_.start_) / double(a.ts_.base_);
    double tb = double(b.ts_.start_) / double(b.ts_.base_);
    return ta < tb;
}

//----------------------------------------------------------------
// TChapter
// 
struct TChapter
{
    TChapter(const char * label = "",
               double time = 0.0):
        label_(label),
        time_(time)
    {}
    
    std::string label_;
    double time_;
};

//----------------------------------------------------------------
// TMatroskaFormatter::endFormatting
// 
TResult
TMatroskaFormatter::endFormatting()
{
    if (m_matroska.segments_.empty())
    {
        return NO_PROBLEM;
    }
    
    try
    {
        // shortcuts:
        TSegment & segment = currentSegment();
        TTracks & tracks = segment.payload_.tracks_;
        TSeekHead & seekHead1 = segment.payload_.seekHeads_.front();
        TCues & cues = segment.payload_.cues_;
        
        TVendorId vendorId = 0;
        TCodecId  codecId = 0;
        if (m_audioCodec)
        {
            m_audioCodec->getAudioCompressorInfo(vendorId, codecId);
        }
        
        // place the first 3 Vorbis packets into CodecPrivate element payload:
        std::deque<Yamka::Frame> vorbisHeader;
        for (unsigned int i = 0; i < 3 && !m_audioFrames.empty(); i++)
        {
            vorbisHeader.push_back(m_audioFrames.front());
            m_audioFrames.pop_front();
        }
        
        if (vorbisHeader.size() == 3)
        {
            Yamka::Bytes codecPrivate;
            
            std::size_t lastFrameIndex = vorbisHeader.size() - 1;
            codecPrivate << Yamka::TByte(lastFrameIndex);
            
            for (std::size_t i = 0; i < lastFrameIndex; i++)
            {
                // shortcut:
                const Yamka::Frame & frame = vorbisHeader[i];
                
                // retrieve packet data:
                Yamka::Bytes packet;
                frame.data_.get(packet);
                
                // store packet size:
                std::size_t frameSize = packet.size();
                while (true)
                {
                    Yamka::TByte sz =
                        (frameSize < 0xFF) ?
                        Yamka::TByte(frameSize) :
                        Yamka::TByte(0xFF);
                    
                    codecPrivate << sz;
                    frameSize -= sz;
                    
                    if (sz < 0xFF)
                    {
                        break;
                    }
                }
            }
            
            for (std::size_t i = 0; i <= lastFrameIndex; i++)
            {
                // shortcut:
                const Yamka::Frame & frame = vorbisHeader[i];
                
                // retrieve packet data:
                Yamka::Bytes packet;
                frame.data_.get(packet);
                
                // store packet:
                codecPrivate << packet;
            }
            
            Yamka::Track & track = tracks.payload_.tracks_.back().payload_;
            track.codecPrivate_.payload_.set(codecPrivate, m_yamkaTmp);
        }
        
        // update segment duration:
        TSegInfo & segInfo = segment.payload_.info_;
        double seconds = 0.0;
        if (!m_videoFrames.empty())
        {
            const Yamka::Frame & lastFrame = m_videoFrames.back();
            seconds =
                double(lastFrame.ts_.start_ + lastFrame.ts_.extent_) /
                double(lastFrame.ts_.base_);
        }
        else if (!m_audioFrames.empty())
        {
            const Yamka::Frame & lastFrame = m_audioFrames.back();
            seconds =
                double(lastFrame.ts_.start_ + lastFrame.ts_.extent_) /
                double(lastFrame.ts_.base_);
        }
        
        double duration = (double(NANOSEC_PER_SEC) /
                           double(TIMECODE_SCALE)) * seconds;
        segInfo.payload_.duration_.payload_.set(duration);
        
        // create Chapters:
        if (m_matroska.fileFormat_ == kFileFormatMatroska &&
            m_chapterMarkers)
        {
            const int numMarkers = m_chapterMarkers->getSize();
            std::list<TChapter> chapters;
            for (int i = 0; i < numMarkers; i++)
            {
                const TMarker * marker = m_chapterMarkers->getMarker(i);
                if (marker->getType() != TMarker::kMarkerChapter)
                {
                    continue;
                }
                
                std::string name = marker->getLabel();
                double time = marker->getEncodedFrameTime();
                
                chapters.push_back(TChapter(name.c_str(), time));
            }
            
            if (!chapters.empty())
            {
                // shortcuts:
                TChapters & eltChapters = segment.payload_.chapters_;
                
                eltChapters.payload_.editions_.push_back(TEdition());
                TEdition & edition = eltChapters.payload_.editions_.back();
                
                Yamka::TByteVec editionUID = Yamka::createUID(16);
                edition.payload_.UID_.payload_.set(Yamka::uintDecode(editionUID, 16));
                
                std::list<TChapAtom> & chapAtoms = edition.payload_.chapAtoms_;
                
                // must have a chapter at the very beginning:
                if (chapters.front().time_ > 0.0)
                {
                    chapters.push_front(TChapter("Intro"));
                }
                
                for (std::list<TChapter>::const_iterator i = chapters.begin();
                     i != chapters.end(); ++i)
                {
                    const TChapter & chapter = *i;
                    
                    chapAtoms.push_back(TChapAtom());
                    TChapAtom & chapAtom = chapAtoms.back();
                    
                    Yamka::TByteVec UID = Yamka::createUID(16);
                    chapAtom.payload_.UID_.payload_.set(Yamka::uintDecode(UID, 16));
                    
                    Yamka::uint64 ns = Yamka::uint64(chapter.time_ * double(NANOSEC_PER_SEC));
                    chapAtom.payload_.timeStart_.payload_.set(ns);
                    
                    chapAtom.payload_.display_.push_back(TChapDisplay());
                    TChapDisplay & chapDisp = chapAtom.payload_.display_.back();
                    
                    chapDisp.payload_.string_.payload_.set(chapter.label_);
                    
                    // find an audio frame containing given time point:
                    for (std::list<Yamka::Frame>::iterator j = m_audioFrames.begin();
                         j != m_audioFrames.end(); j++)
                    {
                        Yamka::Frame & f = *j;
                        if (f.ts_.contains(ns, NANOSEC_PER_SEC))
                        {
                            // this frame contains a chapter marker,
                            // mark it as key frame so that we would generate
                            // a cue point for it:
                            f.isKeyframe_ = true;
                        }
                    }
                }
                
                if (eltChapters.mustSave())
                {
                    seekHead1.payload_.indexThis(&segment,
                                                 &eltChapters,
                                                 m_yamkaTmp);
                }
            }
        }
        
        // sort all frames:
        m_videoFrames.sort(&TCompareFrames);
        m_audioFrames.sort(&TCompareFrames);
        
        // split frames into groups:
        const Yamka::uint64 clusterTimeBase = NANOSEC_PER_SEC / TIMECODE_SCALE;
        std::list<Yamka::GroupOfFrames> gofs;
        
        while (!m_videoFrames.empty())
        {
            Yamka::Frame videoFrame = m_videoFrames.front();
            m_videoFrames.pop_front();
            
            if (videoFrame.isKeyframe_ ||
                gofs.empty() ||
                !gofs.back().mayAdd(videoFrame))
            {
                // start a new group of frames:
                gofs.push_back(Yamka::GroupOfFrames(clusterTimeBase));
            }
            
            Yamka::GroupOfFrames & gof = gofs.back();
            gof.add(videoFrame);
            
            // add audio frames whose time span end point is contained
            // in the current group of frames:
            while (!m_audioFrames.empty())
            {
                Yamka::Frame audioFrame = m_audioFrames.front();
                if (!gof.ts_.contains(audioFrame.ts_.getEnd(gof.ts_.base_),
                                      gof.ts_.base_))
                {
                    break;
                }
                
                if (!gofs.back().mayAdd(audioFrame))
                {
                    // start a new group of frames:
                    gofs.push_back(Yamka::GroupOfFrames(clusterTimeBase));
                }
                
                m_audioFrames.pop_front();
                gof.add(audioFrame);
            }
        }
        
        // split remaining audio frames into groups of frames:
        while (!m_audioFrames.empty())
        {
            Yamka::Frame audioFrame = m_audioFrames.front();
            m_audioFrames.pop_front();
            
            if (gofs.empty() ||
                !gofs.back().mayAdd(audioFrame))
            {
                // start a new group of frames:
                gofs.push_back(Yamka::GroupOfFrames(clusterTimeBase));
            }
            
            Yamka::GroupOfFrames & gof = gofs.back();
            gof.add(audioFrame);
        }
        
        // assemble groups of frames into clusters:
        std::list<Yamka::MetaCluster> metaClusters;
        while (!gofs.empty())
        {
            Yamka::GroupOfFrames gof = gofs.front();
            gofs.pop_front();
            
            if (metaClusters.empty() ||
                !metaClusters.back().mayAdd(gof))
            {
                // start a new meta cluster:
                bool allowManyKeyframes =
                    (m_matroska.fileFormat_ != kFileFormatWebMedia);
                
                metaClusters.push_back(Yamka::MetaCluster(allowManyKeyframes));
            }
            
            Yamka::MetaCluster & metaCluster = metaClusters.back();
            metaCluster.add(gof);
        }

        // convert meta clusters into matroska Clusters and SimpleBlocks,
        // create Cues along the way:
        while (!metaClusters.empty())
        {
            Yamka::MetaCluster metaCluster = metaClusters.front();
            metaClusters.pop_front();
            
            // sort frames in ascending timecode order, per WebMedia guideline:
            metaCluster.frames_.sort(&TCompareFrames);
            
            segment.payload_.clusters_.push_back(TCluster());
            TCluster & cluster = currentCluster();
            
            cluster.payload_.timecode_.payload_.set(metaCluster.ts_.start_);
            cluster.payload_.position_.payload_.setOrigin(&segment);
            cluster.payload_.position_.payload_.setElt(&cluster);
            
            TSeekHead & seekHead = currentSeekHead();
            seekHead.payload_.indexThis(&segment, &cluster, m_yamkaTmp);
            
            std::list<Yamka::Frame> & frames = metaCluster.frames_;
            if (!m_encodeVideo && !frames.empty())
            {
                // audio-only files should include a CuePoint
                // to the first audio frame of each cluster:
                Yamka::Frame & frame = frames.front();
                frame.isKeyframe_ = true;
            }
            
            Yamka::SimpleBlock simpleBlock;
            while (!frames.empty())
            {
                Yamka::Frame frame = frames.front();
                frames.pop_front();
                
                // NOTE: WebM doesn't support lacing, yet:
                if (m_matroska.fileFormat_ == kFileFormatWebMedia ||
                    simpleBlock.getTrackNumber() != frame.trackNumber_ ||
                    simpleBlock.getNumberOfFrames() > 7 ||
                    frame.trackNumber_ == kVideoTrackNumber ||
                    frame.isKeyframe_)
                {
                    finishCurrentBlock(simpleBlock);
                }
                
                Yamka::uint64 absTimecode = frame.ts_.getStart(metaCluster.ts_.base_);
                Yamka::int64 relTimecode = absTimecode - metaCluster.ts_.start_;
                assert(relTimecode < Yamka::kShortDistLimit);
                if (simpleBlock.getNumberOfFrames() == 0)
                {
                    simpleBlock.setRelativeTimecode((short int)relTimecode);
                }
                
                simpleBlock.setTrackNumber(frame.trackNumber_);
                if (frame.isKeyframe_)
                {
                    simpleBlock.setKeyframe(true);
                    
                    // add block to cues:
                    cues.payload_.points_.push_back(TCuePoint());
                    TCuePoint & cuePoint = cues.payload_.points_.back();
                    
                    // set cue timepoint:
                    cuePoint.payload_.time_.payload_.set(absTimecode);
                    
                    // add a track position for this timepoint:
                    cuePoint.payload_.trkPosns_.resize(1);
                    Yamka::CueTrkPos & pos = cuePoint.payload_.trkPosns_.back().payload_;
                    
                    std::size_t blockIndex = cluster.payload_.simpleBlocks_.size() + 1;
                    pos.block_.payload_.set(blockIndex);
                    pos.track_.payload_.set(frame.trackNumber_);
                    
                    pos.cluster_.payload_.setOrigin(&segment);
                    pos.cluster_.payload_.setElt(&cluster);
                }
                else if (frame.trackNumber_ == kAudioTrackNumber)
                {
                    simpleBlock.setKeyframe(true);
                }
                
                Yamka::Bytes frameData;
                frame.data_.get(frameData);
                simpleBlock.addFrame(frameData);
            }
            
            // finish the last block in this cluster:
            finishCurrentBlock(simpleBlock);
        }
        
        if (cues.mustSave())
        {
            seekHead1.payload_.indexThis(&segment, &cues, m_yamkaTmp);
        }
        
        if (m_matroska.fileFormat_ == kFileFormatMatroska)
        {
            // enable CRC-32 for Level-1 elements:
            m_matroska.setCrc32(true);
        }
        
        // save the file:
        m_yamkaOut.file_.setSize(0);
        Yamka::IStorage::IReceiptPtr receipt = m_matroska.save(m_yamkaOut);
        
        // save yamka temp file name, so it can be removed:
        std::string yamkaTmpUTF8 = m_yamkaTmp.file_.filename();
        
        // close open file handles:
        m_matroska = Yamka::WebMediaDoc(m_matroska.fileFormat_);
        m_yamkaTmp = Yamka::FileStorage();
        m_yamkaOut = Yamka::FileStorage();
        
        // remove yamka temp file:
        helpers::remove_UTF8(yamkaTmpUTF8.c_str());
    }
    catch (...)
    {
        return OOPS_FORMAT_FILE_CLOSE_FAILED;
    }
    
    return NO_PROBLEM;
}

//----------------------------------------------------------------
// TMatroskaFormatter::currentSegment
// 
TSegment &
TMatroskaFormatter::currentSegment()
{
    assert(!m_matroska.segments_.empty());
    TSegment & segment = m_matroska.segments_.back();
    return segment;
}

//----------------------------------------------------------------
// TMatroskaFormatter::currentCluster
// 
TCluster &
TMatroskaFormatter::currentCluster()
{
    TSegment & segment = currentSegment();
    
    assert(!segment.payload_.clusters_.empty());
    TCluster & cluster = segment.payload_.clusters_.back();
    return cluster;
}

//----------------------------------------------------------------
// TMatroskaFormatter::currentSeekHead
// 
TSeekHead &
TMatroskaFormatter::currentSeekHead()
{
    TSegment & segment = currentSegment();
    
    assert(!segment.payload_.seekHeads_.empty());
    TSeekHead & seekHead = segment.payload_.seekHeads_.back();
    return seekHead;
}

//----------------------------------------------------------------
// TMatroskaFormatter::finishCurrentBlock
// 
void
TMatroskaFormatter::finishCurrentBlock(Yamka::SimpleBlock & simpleBlock)
{
    if (!simpleBlock.getNumberOfFrames())
    {
        return;
    }
    
    TCluster & cluster = currentCluster();
    
    cluster.payload_.simpleBlocks_.push_back(TSimpleBlock());
    TSimpleBlock & block = cluster.payload_.simpleBlocks_.back();
    
    Yamka::Bytes data;
    simpleBlock.setAutoLacing();
    simpleBlock.exportData(data);
    block.payload_.set(data, m_yamkaTmp);
    
    simpleBlock = Yamka::SimpleBlock();
}

//----------------------------------------------------------------
// TMatroskaFormatter::getVideoCodecID
// 
const char *
TMatroskaFormatter::getVideoCodecID() const
{
    if (!m_videoCodec)
    {
        return NULL;
    }
    
    TVendorId vendorId = 0;
    TCodecId  codecId = 0;
    m_videoCodec->getVideoCompressorInfo(vendorId, codecId);
    
    switch (codecId)
    {
        case kVideoCodecVP8:
            return "V_VP8";
            
        default:
            break;
    }
    
    assert(false);
    return "V_FIXME";
}

//----------------------------------------------------------------
// TMatroskaFormatter::getVideoPixelWidth
// 
unsigned int
TMatroskaFormatter::getVideoPixelWidth() const
{
    unsigned int w = m_videoCodec->getOutputWidthSetting()->getValue();
    return w;
}

//----------------------------------------------------------------
// TMatroskaFormatter::getVideoPixelHeight
// 
unsigned int
TMatroskaFormatter::getVideoPixelHeight() const
{
    unsigned int h = m_videoCodec->getOutputHeightSetting()->getValue();
    return h;
}

//----------------------------------------------------------------
// TMatroskaFormatter::getVideoFrameRate
// 
double
TMatroskaFormatter::getVideoFrameRate() const
{
    double fps = m_videoCodec->getFrameRateSetting()->getValue();
    return fps;
}

//----------------------------------------------------------------
// TMatroskaFormatter::getVideoFrameDuration
// 
double
TMatroskaFormatter::getVideoFrameDuration() const
{
    double fps = getVideoFrameRate();
    double secPerFrame = 1.0 / fps;
    return secPerFrame;
}

//----------------------------------------------------------------
// TMatroskaFormatter::getAudioCodecID
// 
const char *
TMatroskaFormatter::getAudioCodecID() const
{
    if (!m_audioCodec)
    {
        return NULL;
    }
    
    TVendorId vendorId = 0;
    TCodecId  codecId = 0;
    m_audioCodec->getAudioCompressorInfo(vendorId, codecId);
    
    switch (codecId)
    {
        case kAudioCodecVorbis:
            return "A_VORBIS";
            
        default:
            break;
    }
    
    assert(false);
    return "A_FIXME";
}

//----------------------------------------------------------------
// TMatroskaFormatter::getAudioSampleRate
// 
unsigned int
TMatroskaFormatter::getAudioSampleRate() const
{
    if (!m_audioCodec)
    {
        return 0;
    }
    
    unsigned int r = m_audioCodec->getSampleRateSetting()->getValue();
    return r;
}

//----------------------------------------------------------------
// TMatroskaFormatter::getAudioChannels
// 
unsigned int
TMatroskaFormatter::getAudioChannels() const
{
    // TODO: return number of channels in the audio stream:
    return 1;
}

namespace Yamka
{
    
    //----------------------------------------------------------------
    // WebMediaSegmentSaver
    // 
    struct WebMediaSegmentSaver : public Segment::IDelegateSave
    {
        // virtual:
        IStorage::IReceiptPtr
        save(const Segment & segment, IStorage & storage)
        {
            IStorage::IReceiptPtr receipt = storage.receipt();
            
            typedef std::deque<TSeekHead>::const_iterator TSeekHeadIter;
            TSeekHeadIter seekHeadIter = segment.seekHeads_.begin();
            
            // save the first seekhead:
            if (seekHeadIter != segment.seekHeads_.end())
            {
                const TSeekHead & seekHead = *seekHeadIter;
                *receipt += seekHead.save(storage);
                ++seekHeadIter;
            }
            
            *receipt += segment.info_.save(storage);
            *receipt += segment.tracks_.save(storage);
            *receipt += segment.chapters_.save(storage);
            *receipt += segment.cues_.save(storage);
            *receipt += segment.attachments_.save(storage);
            *receipt += eltsSave(segment.tags_, storage);
            *receipt += eltsSave(segment.clusters_, storage);
            
            // save any remaining seekheads:
            for (; seekHeadIter != segment.seekHeads_.end(); ++seekHeadIter)
            {
                const TSeekHead & seekHead = *seekHeadIter;
                *receipt += seekHead.save(storage);
            }
            
            return receipt;
        }
    };
    
    
    //----------------------------------------------------------------
    // WebMediaDoc::WebMediaDoc
    // 
    WebMediaDoc::WebMediaDoc(TFileFormat fileFormat):
        MatroskaDoc(),
        fileFormat_(fileFormat)
    {
        if (fileFormat_ == kFileFormatWebMedia)
        {
            // WebMedia uses "webm" DocType.
            // However, most Matroska tools will not work
            // if DocType is not "matroska".
            // Therefore, we set DocType to "webm" and append
            // a Void element to it so that "webm" + Void element
            // can be replaced with "matroska" if necessary.
            
            EbmlDoc::head_.payload_.docType_.payload_.set("webm");
            EbmlDoc::head_.payload_.docType_.payload_.addVoid(2);
        }
        
        // Must set DocType...Version to 2 because we use SimpleBlock:
        EbmlDoc::head_.payload_.docTypeVersion_.payload_.set(2);
        EbmlDoc::head_.payload_.docTypeReadVersion_.payload_.set(2);
    }
    
    //----------------------------------------------------------------
    // WebMediaDoc::save
    // 
    IStorage::IReceiptPtr
    WebMediaDoc::save(IStorage & storage) const
    {
        if (fileFormat_ == kFileFormatWebMedia)
        {
            // shortcut:
            typedef std::list<TSegment>::const_iterator TSegmentIter;
            
            // override how the segments will be saved:
            for (TSegmentIter i = segments_.begin(); i != segments_.end(); ++i)
            {
                const TSegment & segment = *i;
                segment.payload_.delegateSave_.reset(new WebMediaSegmentSaver);
            }
        }
        
        // let the base class handle everything else as usual:
        return MatroskaDoc::save(storage);
    }
    
    
    //----------------------------------------------------------------
    // TimeSpan::TimeSpan
    // 
    TimeSpan::TimeSpan():
        start_(0),
        extent_(0),
        base_(0)
    {}
    
    //----------------------------------------------------------------
    // TimeSpan::getStart
    // 
    uint64
    TimeSpan::getStart(uint64 base) const
    {
        uint64 ta = uint64(double(start_ * base) / double(base_));
        return ta;
    }
    
    //----------------------------------------------------------------
    // TimeSpan::getExtent
    // 
    uint64
    TimeSpan::getExtent(uint64 base) const
    {
        uint64 te = uint64(double(extent_ * base) / double(base_));
        return te;
    }
    
    //----------------------------------------------------------------
    // TimeSpan::getEnd
    // 
    uint64
    TimeSpan::getEnd(uint64 base) const
    {
        uint64 tb = uint64(double((start_ + extent_) * base) / double(base_));
        return tb;
    }
    
    //----------------------------------------------------------------
    // TimeSpan::setStart
    // 
    void
    TimeSpan::setStart(uint64 t, uint64 base)
    {
        start_ = uint64(double(t * base_) / double(base));
    }
    
    //----------------------------------------------------------------
    // TimeSpan::expand
    // 
    void
    TimeSpan::expand(uint64 t, uint64 base)
    {
        uint64 tb = uint64(double(t * base_) / double(base));
        if (tb > start_)
        {
            uint64 te = tb - start_;
            extent_ = std::max(te, extent_);
        }
    }
    
    //----------------------------------------------------------------
    // TimeSpan::contains
    // 
    bool
    TimeSpan::contains(uint64 t, uint64 base) const
    {
        uint64 ta = getStart(base);
        uint64 tb = getEnd(base);
        bool isInside = (t >= ta) && (t < tb);
        return isInside;
    }
    
    
    //----------------------------------------------------------------
    // Frame::Frame
    // 
    Frame::Frame():
        trackNumber_(0),
        isKeyframe_(false)
    {}
    
    
    //----------------------------------------------------------------
    // kShortDistLimit
    // 
    const short int kShortDistLimit = 0x7FFF;
    
    
    //----------------------------------------------------------------
    // GroupOfFrames::GroupOfFrames
    // 
    GroupOfFrames::GroupOfFrames(uint64 timebase):
        minStart_(std::numeric_limits<uint64>::max()),
        maxStart_(std::numeric_limits<uint64>::min())
    {
        ts_.base_ = timebase;
    }
    
    //----------------------------------------------------------------
    // GroupOfFrames::mayAdd
    // 
    bool
    GroupOfFrames::mayAdd(const Frame & frame) const
    {
        if (frames_.empty())
        {
            return true;
        }
        
        uint64 groupStart = ts_.start_;
        uint64 frameStart = frame.ts_.getStart(ts_.base_);
        uint64 distFromStart =
            frameStart > groupStart ?
            frameStart - groupStart :
            groupStart - frameStart;
        
        bool withinLimit = distFromStart < kShortDistLimit;
        return withinLimit;
    }
    
    //----------------------------------------------------------------
    // GroupOfFrames::add
    // 
    void
    GroupOfFrames::add(const Frame & frame)
    {
        uint64 frameStart = frame.ts_.getStart(ts_.base_);
        uint64 frameEnd = frame.ts_.getEnd(ts_.base_);
        
        if (frames_.empty())
        {
            ts_.setStart(frameStart, ts_.base_);
        }
        ts_.expand(frameEnd, ts_.base_);
        
        minStart_ = std::min(frameStart, minStart_);
        maxStart_ = std::max(frameStart, maxStart_);
        
        frames_.push_back(frame);
    }
    
    
    //----------------------------------------------------------------
    // MetaCluster::MetaCluster
    // 
    MetaCluster::MetaCluster(bool allowMultipleKeyframes):
        allowMultipleKeyframes_(allowMultipleKeyframes)
    {}
    
    //----------------------------------------------------------------
    // MetaCluster::mayAdd
    // 
    bool
    MetaCluster::mayAdd(const GroupOfFrames & gof) const
    {
        if (frames_.empty())
        {
            return true;
        }
        else if (!allowMultipleKeyframes_)
        {
            return false;
        }
        
        assert(ts_.base_ == gof.ts_.base_);
        
        uint64 clusterStart = ts_.start_;
        uint64 groupMin = gof.minStart_;
        uint64 groupMax = gof.maxStart_;
        
        uint64 distMin =
            groupMin > clusterStart ?
            groupMin - clusterStart :
            clusterStart - groupMin;
        
        uint64 distMax =
            groupMax > clusterStart ?
            groupMax - clusterStart :
            clusterStart - groupMax;
        
        bool withinLimit =
            (distMin < kShortDistLimit) &&
            (distMax < kShortDistLimit);
        
        return withinLimit;
    }
    
    //----------------------------------------------------------------
    // MetaCluster::add
    // 
    void
    MetaCluster::add(const GroupOfFrames & gof)
    {
        if (frames_.empty())
        {
            ts_ = gof.ts_;
        }
        
        assert(ts_.base_ == gof.ts_.base_);
        
        uint64 gofEnd = gof.ts_.getEnd(ts_.base_);
        ts_.expand(gofEnd, ts_.base_);
        
        frames_.insert(frames_.end(), gof.frames_.begin(), gof.frames_.end());
    }
    
}
