// -*- Mode: c++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

#ifndef MATROSKA_FORMATTER_H_
#define MATROSKA_FORMATTER_H_

// system includes:
#include <vector>
#include <list>

// yamka includes:
#include <yamka.h>

// forward declarations:
class AVideoCompressor;
class AAudioCompressor;


//----------------------------------------------------------------
// TFileFormat
// 
typedef enum
{
    kFileFormatMatroska,
    kFileFormatWebMedia
} TFileFormat;


namespace Yamka
{
    //----------------------------------------------------------------
    // WebMediaDoc
    // 
    // 1. WebMedia requires "webm" DocType
    // 2. WebMedia requires Cues to be saved ahead of the Clusters
    // 
    struct WebMediaDoc : public MatroskaDoc
    {
        WebMediaDoc(TFileFormat fileFormat);
        
        // virtual:
        IStorage::IReceiptPtr
        save(IStorage & storage) const;
        
        // helper:
        static IStorage::IReceiptPtr
        saveSegment(const Segment & segment, IStorage & storage);
        
        // which format are we writing to:
        TFileFormat fileFormat_;
    };
    
    //----------------------------------------------------------------
    // TimeSpan
    // 
    struct TimeSpan
    {
        TimeSpan();
        
        // return extreme points of this time span expressed in a given timebase:
        uint64 getStart(uint64 base) const;
        uint64 getExtent(uint64 base) const;
        uint64 getEnd(uint64 base) const;
        
        // set the start point of this time span, expressed in given time base:
        void setStart(uint64 t, uint64 base);
        
        // expand this time span to include a given time point.
        // NOTE: given timepoint must not precede the start point
        //       of this time span.
        void expand(uint64 t, uint64 base);
        
        // check whether a given timepoint (expressed in given time base)
        // falls within this time span:
        bool contains(uint64 t, uint64 base) const;
        
        // numerator (position in time base units):
        uint64 start_;

        // numerator (duration in time base units)
        uint64 extent_;
        
        // denominator (units per second):
        uint64 base_;
    };
    
    //----------------------------------------------------------------
    // Frame
    // 
    struct Frame
    {
        Frame();
        
        // what track does this Frame belong to:
        uint64 trackNumber_;
        
        // where is this Frame on the timeline, in seconds:
        TimeSpan ts_;
        
        // frame data:
        VBinary data_;
        
        // keyframe flag:
        bool isKeyframe_;
    };
    
    
    //----------------------------------------------------------------
    // kShortDistLimit
    // 
    extern const short int kShortDistLimit;
    
    
    //----------------------------------------------------------------
    // GroupOfFrames
    // 
    struct GroupOfFrames
    {
        GroupOfFrames(uint64 timebase);
        
        // check whether another frame may be added to this group
        // without exceeding the short distance limit (interpreted
        // in this groups time base) from frame start point
        // to the start point of this group:
        bool mayAdd(const Frame & frame) const;
        
        // add a frame to the group:
        void add(const Frame & frame);
        
        // where is this group on the timeline, in seconds:
        TimeSpan ts_;
        
        // min and max start point over all frames containes in this group,
        // expressed in this groups time base:
        uint64 minStart_;
        uint64 maxStart_;
        
        // a list of frames included in this group:
        std::list<Frame> frames_;
    };
    
    //----------------------------------------------------------------
    // MetaCluster
    // 
    struct MetaCluster
    {
        MetaCluster(bool allowMultipleKeyframes);
        
        // check whether another group of frames may be added to this cluster
        // without exceeding the short distance limit (interpreted in
        // this clusters time base) from min/max GOF start point
        // to the start point of this cluster:
        bool mayAdd(const GroupOfFrames & gof) const;
        
        // add a group of frames to this cluster:
        void add(const GroupOfFrames & gof);
        
        // where is this cluster on the timeline, in seconds:
        TimeSpan ts_;
        
        // a list of frames included in this group:
        std::list<Frame> frames_;
        
        // a flag to control whether multiple multiple keyframes may be stored
        // in a cluster (not appropriate for streaming, many players will only
        // seek to the first keyframe in the cluster):
        bool allowMultipleKeyframes_;
    };
}

// yamka shortcuts:
typedef Yamka::MatroskaDoc::TSegment TSegment;
typedef Yamka::Segment::TInfo TSegInfo;
typedef Yamka::Segment::TTracks TTracks;
typedef Yamka::Segment::TSeekHead TSeekHead;
typedef Yamka::Segment::TCluster TCluster;
typedef Yamka::Segment::TCues TCues;
typedef Yamka::Segment::TTags TTags;
typedef Yamka::Segment::TChapters TChapters;
typedef Yamka::Cues::TCuePoint TCuePoint;
typedef Yamka::Tags::TTag TTag;
typedef Yamka::Tag::TTargets TTagTargets;
typedef Yamka::Tag::TSimpleTag TSimpleTag;
typedef Yamka::TagTargets::TTrackUID TTagTrackUID;
typedef Yamka::Tracks::TTrack TTrackEntry;
typedef Yamka::Cluster::TSimpleBlock TSimpleBlock;
typedef Yamka::Chapters::TEdition TEdition;
typedef Yamka::Edition::TChapAtom TChapAtom;
typedef Yamka::ChapAtom::TDisplay TChapDisplay;


//----------------------------------------------------------------
// TMatroskaFormatter
// 
class TMatroskaFormatter : public TFormatterBase
{
public:
    TMatroskaFormatter(TFileFormat fileFormat);
    virtual ~TMatroskaFormatter();
    
    void setVideoCompressor(const AVideoCompressor * vc);
    void setAudioCompressor(const AAudioCompressor * ac);

    TResult beginFormatting();
    TResult addAudioSample(TEncAudio * audioFrame);
    TResult addVideoSample(TEncImage * videoFrame);
    TResult endFormatting();
    
private:
    // yamka helpers:
    enum kTrackNumber
    {
        kVideoTrackNumber = 1,
        kAudioTrackNumber = 2
    };
    
    TSegment & currentSegment();
    TCluster & currentCluster();
    TSeekHead & currentSeekHead();
    
    void finishCurrentBlock(Yamka::SimpleBlock & simpleBlock);
    
    // helpers:
    const char * getVideoCodecID() const;
    unsigned int getVideoPixelWidth() const;
    unsigned int getVideoPixelHeight() const;
    double getVideoFrameRate() const;
    double getVideoFrameDuration() const;
    
    const char * getAudioCodecID() const;
    unsigned int getAudioSampleRate() const;
    unsigned int getAudioChannels() const;
    
    // yamka data:
    Yamka::WebMediaDoc m_matroska;
    Yamka::FileStorage m_yamkaTmp;
    Yamka::FileStorage m_yamkaOut;
    std::list<Yamka::Frame> m_audioFrames;
    std::list<Yamka::Frame> m_videoFrames;
    
    // shortcuts to video/audio codec (so we can interrogate them for detail):
    const AVideoCompressor * m_videoCodec;
    const AAudioCompressor * m_audioCodec;
};


#endif // MATROSKA_FORMATTER_H_
