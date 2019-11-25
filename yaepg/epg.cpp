// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Aug 31 14:20:04 MDT 2019
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard:
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

// boost:
#ifndef Q_MOC_RUN
#include <boost/locale.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#endif

// yae:
#include "yae/api/yae_log.h"
#include "yae/thread/yae_ring_buffer.h"
#include "yae/thread/yae_worker.h"
#include "yae/utils/yae_data.h"
#include "yae/utils/yae_utils.h"
#include "yae/video/yae_mpeg_ts.h"

// epg:
#include "yae_hdhomerun.h"
#include "yae_signal_handler.h"


// namespace shortcut:
namespace fs = boost::filesystem;


namespace yae
{

  //----------------------------------------------------------------
  // Capture
  //
  struct Capture : ICapture
  {
    Capture();
    ~Capture();

    //----------------------------------------------------------------
    // Work
    //
    struct Work
    {
      Work():
        rb_(188 * 4096)
      {}

      ~Work()
      {
        rb_.close();
        worker_.stop();
        worker_.wait_until_finished();
      }

      yae::Worker worker_;
      yae::TOpenFilePtr capture_;
      yae::mpeg_ts::Context ctx_;
      yae::RingBuffer rb_;
    };

    //----------------------------------------------------------------
    // TWorkPtr
    //
    typedef boost::shared_ptr<Work> TWorkPtr;

    //----------------------------------------------------------------
    // Task
    //
    struct Task : yae::Worker::Task
    {
      Task(const std::string & tuner_name,
           const std::string & frequency,
           std::size_t size,
           Work & work);

      // virtual:
      void execute(const yae::Worker & worker);

      std::string tuner_name_;
      std::string frequency_;
      std::size_t size_;
      Work & work_;
    };

    //----------------------------------------------------------------
    // push
    //
    TResponse
    push(const std::string & tuner_name,
         const std::string & frequency,
         const void * data,
         std::size_t size);

    std::map<std::string, TWorkPtr> work_;
  };


  //----------------------------------------------------------------
  // Capture::Capture
  //
  Capture::Capture()
  {}

  //----------------------------------------------------------------
  // Capture::~Capture
  //
  Capture::~Capture()
  {
    for (std::map<std::string, TWorkPtr>::const_iterator
           i = work_.begin(); i != work_.end(); ++i)
    {
      const std::string & frequency = i->first;
      const Work & work = *(i->second);
      const yae::mpeg_ts::Context & ctx = work.ctx_;
      ctx.dump();
    }
  }

  //----------------------------------------------------------------
  // Capture::Task::Task
  //
  Capture::Task::Task(const std::string & tuner_name,
                      const std::string & frequency,
                      std::size_t size,
                      Work & work):
    tuner_name_(tuner_name),
    frequency_(frequency),
    size_(size),
    work_(work)
  {}

  //----------------------------------------------------------------
  // Capture::Task::execute
  //
  void
  Capture::Task::execute(const yae::Worker & worker)
  {
    (void)worker;

    yae::RingBuffer & rb = work_.rb_;
    TOpenFile & file = *(work_.capture_);
    yae::mpeg_ts::Context & ctx = work_.ctx_;

    std::size_t done = 0;
    while (true)
    {
      std::size_t todo = std::min<std::size_t>(188 * 7, size_ - done);
      if (todo < 188)
      {
        YAE_EXPECT(!todo);
        break;
      }

      yae::Data data(todo);
      std::size_t size = rb.pull(data.get(), data.size());
      YAE_EXPECT(size == todo);
      done += size;

      data.truncate(size);
      file.write(data.get(), size);
#if 1
      // parse the transport stream:
      yae::Bitstream bitstream(data);
      while (!bitstream.exhausted())
      {
        try
        {
          yae::mpeg_ts::TSPacket pkt;
          ctx.load(bitstream, pkt);
        }
        catch (const std::exception & e)
        {
          std::string data_hex =
            yae::to_hex(data.get(), std::min<std::size_t>(size, 32), 4);

          yae_wlog("failed to parse %s, tuner %s, %sHz: %s",
                   data_hex.c_str(),
                   tuner_name_.c_str(),
                   frequency_.c_str(),
                   e.what());
        }
        catch (...)
        {
          std::string data_hex =
            yae::to_hex(data.get(), std::min<std::size_t>(size, 32), 4);

          yae_wlog("failed to parse %s..., tuner %s, %sHz %s: "
                   "unexpected exception",
                   data_hex.c_str(),
                   tuner_name_.c_str(),
                   frequency_.c_str());
        }
      }
#endif
    }
  }

  //----------------------------------------------------------------
  // Capture::push
  //
  ICapture::TResponse
  Capture::push(const std::string & tuner_name,
                const std::string & frequency,
                const void * data,
                std::size_t size)
  {
#if 1
    std::string capture_path =
      (fs::path("/tmp") / (frequency + "-v1.ts")).string();

    boost::shared_ptr<TOpenFile> file_ptr =
      get_open_file(capture_path.c_str(), "wb");
    YAE_THROW_IF(!file_ptr);

    TOpenFile & capture = *file_ptr;
    YAE_THROW_IF(!capture.is_open());

    capture.write(data, size);
#endif

    TWorkPtr & work_ptr = work_[frequency];
    if (!work_ptr)
    {
      work_ptr.reset(new Work());
      Work & work = *work_ptr;

      std::string capture_path =
        (fs::path("/tmp") / (frequency + ".ts")).string();

      TOpenFilePtr file = get_open_file(capture_path.c_str(), "wb");
      YAE_THROW_IF(!(file && file->is_open()));
      work.capture_ = file;
    }

    Work & work = *work_ptr;
    yae::RingBuffer & rb = work.rb_;

    if (!size)
    {
      rb.close();
      return STOP_E;
    }

#if 0
    std::string data_hex =
      yae::to_hex(data, std::min<std::size_t>(size, 32), 4);

    yae_dlog("%s %sHz: %5i %s...",
             tuner_name.c_str(),
             frequency.c_str(),
             int(size),
             data_hex.c_str());
#endif

    yae::shared_ptr<Task, yae::Worker::Task> task;
    task.reset(new Task(tuner_name, frequency, size, work));
    work.worker_.add(task);

    TResponse response = MORE_E;
    if (rb.push(data, size) != size)
    {
      response = STOP_E;
    }

    return response;
  }

}


//----------------------------------------------------------------
// main_may_throw
//
int
main_may_throw(int argc, char ** argv)
{
  // install signal handler:
  yae::signal_handler();

#if 1
  static const yae::TTime sample_duration(30, 1);
  yae::HDHomeRun hdhr;
  yae::TCapturePtr callback(new yae::Capture());
  hdhr.capture_all(sample_duration, callback);

#else
#if 0
  const char * fn = "/tmp/473000000.ts"; // 10.1
  const char * fn = "/tmp/479000000.ts"; // 23.1 KBTU
  const char * fn = "/tmp/491000000.ts"; // 11.1 KBYU
  const char * fn = "/tmp/503000000.ts"; // 14.1 KJZZ
  const char * fn = "/tmp/509000000.ts"; // 20.1 KTMW
  const char * fn = "/tmp/515000000.ts"; // 50.1 KEJT
  const char * fn = "/tmp/527000000.ts"; // 5.1 KSL
  const char * fn = "/tmp/533000000.ts"; // 24.1 KPNZ
  const char * fn = "/tmp/539000000.ts"; // 25.1 KSVN
  const char * fn = "/tmp/551000000.ts"; // 7.1 KUED
  const char * fn = "/tmp/557000000.ts"; // 13.1 KSTU
  const char * fn = "/tmp/563000000.ts"; // 16.1 ION
  const char * fn = "/tmp/569000000.ts"; // 4.1 KTVX
  const char * fn = "/tmp/593000000.ts"; // 2.1 KUTV
  const char * fn = "/tmp/599000000.ts"; // 30.1 KUCW
#endif
  const char * fn = "/tmp/605000000.ts"; // 9.1 KUEN
#if 0
#endif

#if 0
  const char * fn = "/scratch/DataSets/Video/epg/473000000.ts"; // 10.1
  const char * fn = "/scratch/DataSets/Video/epg/479000000.ts"; // 23.1 KBTU
  const char * fn = "/scratch/DataSets/Video/epg/491000000.ts"; // 11.1 KBYU
  const char * fn = "/scratch/DataSets/Video/epg/503000000.ts"; // 14.1 KJZZ
  const char * fn = "/scratch/DataSets/Video/epg/509000000.ts"; // 20.1 KTMW
  const char * fn = "/scratch/DataSets/Video/epg/515000000.ts"; // 50.1 KEJT
  const char * fn = "/scratch/DataSets/Video/epg/527000000.ts"; // 5.1 KSL
  const char * fn = "/scratch/DataSets/Video/epg/533000000.ts"; // 24.1 KPNZ
  const char * fn = "/scratch/DataSets/Video/epg/539000000.ts"; // 25.1 KSVN
#endif
#if 0
  const char * fn = "/scratch/DataSets/Video/epg/551000000.ts"; // 7.1 KUED
  const char * fn = "/scratch/DataSets/Video/epg/557000000.ts"; // 13.1 KSTU
  const char * fn = "/scratch/DataSets/Video/epg/563000000.ts"; // 16.1 ION
  const char * fn = "/scratch/DataSets/Video/epg/569000000.ts"; // 4.1 KTVX
  const char * fn = "/scratch/DataSets/Video/epg/593000000.ts"; // 2.1 KUTV
  const char * fn = "/scratch/DataSets/Video/epg/599000000.ts"; // 30.1 KUCW
  const char * fn = "/scratch/DataSets/Video/epg/605000000.ts"; // 9.1 KUEN
#endif

  yae::TOpenFile src(fn, "rb");
  YAE_THROW_IF(!src.is_open());

  yae::mpeg_ts::Context ts_ctx;
  while (!src.is_eof())
  {
    yae::Data data(12 + 7 * 188);
    uint64_t pos = yae::ftell64(src.file_);

    std::size_t n = src.read(data.get(), data.size());
    if (n < 188)
    {
      break;
    }

    data.truncate(n);

    std::size_t offset = 0;
    while (offset + 188 <= n)
    {
      // find to the the sync byte:
      if (data[offset] == 0x47 &&
          (n - offset == 188 || data[offset + 188] == 0x47))
      {
        try
        {
          // attempt to parse the packet:
          yae::Bitstream bs(data.get(offset, 188));

          yae::mpeg_ts::TSPacket pkt;
          ts_ctx.load(bs, pkt);
        }
        catch (const std::exception & e)
        {
          yae_wlog("failed to parse TS packet at %" PRIu64 ", %s",
                   pos + offset, e.what());
        }
        catch (...)
        {
          yae_wlog("failed to parse TS packet at %" PRIu64
                   ", unexpected exception",
                   pos + offset);
        }

        // skip to next packet:
        offset += 188;
      }
      else
      {
        offset++;
      }
    }

    yae::fseek64(src.file_, pos + offset, SEEK_SET);
  }

  // FIXME: pkoshevoy:
  yae_dlog("-------------- remaining incomplete packets below --------------");
  for (std::map<uint16_t, std::list<yae::mpeg_ts::TSPacket> >::iterator
         i = ts_ctx.pes_.begin(); i != ts_ctx.pes_.end(); ++i)
  {
    uint16_t pid = i->first;
    std::list<yae::mpeg_ts::TSPacket> & pes = i->second;
    ts_ctx.consume(pid, pes, false);
  }

  // FIXME: pkoshevoy:
  ts_ctx.dump();
#endif

  return 0;
}

//----------------------------------------------------------------
// main
//
int
main(int argc, char ** argv)
{
  int r = 0;

  try
  {
    // Create and install global locale (UTF-8)
    {
#ifndef _WIN32
      const char * lc_type = getenv("LC_TYPE");
      const char * lc_all = getenv("LC_ALL");
      const char * lang = getenv("LANG");

      if (!(lc_type || lc_all || lang))
      {
        // avoid crasing in boost+libiconv:
        setenv("LANG", "en_US.UTF-8", 1);
      }
#endif

      std::locale::global(boost::locale::generator().generate(""));
    }

    // Make boost.filesystem use global locale:
    boost::filesystem::path::imbue(std::locale());

    yae::set_console_output_utf8();
    yae::get_main_args_utf8(argc, argv);

    r = main_may_throw(argc, argv);
    std::cout << std::flush;
  }
  catch (const std::exception & e)
  {
    std::cerr << "ERROR: unexpected exception: " << e.what() << std::endl;
    return 1;
  }
  catch (...)
  {
    std::cerr << "ERROR: unknown exception" << std::endl;
    return 2;
  }

  return r;
}
