// -*- Mode: c++; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil -*-
// NOTE: the first line of this file sets up source code indentation rules
// for Emacs; it is also a hint to anyone modifying this file.

// Created   : Sat Aug 31 14:20:04 MDT 2019
// Copyright : Pavel Koshevoy
// License   : MIT -- http://www.opensource.org/licenses/mit-license.php

// standard:
#include <iomanip>
#include <iostream>
#include <stdio.h>
#include <vector>

// boost:
#ifndef Q_MOC_RUN
#include <boost/locale.hpp>
#include <boost/filesystem.hpp>
#endif

// ffmpeg:
extern "C" {
#include <libavformat/avio.h>
#include <libavcodec/get_bits.h>
}

// hdhomerun:
extern "C" {
#include <hdhomerun.h>
}

// jsoncpp:
#include "json/json.h"

// yae:
#include "yae/api/yae_shared_ptr.h"
#include "yae/utils/yae_utils.h"

// namespace shortcut:
namespace fs = boost::filesystem;


//----------------------------------------------------------------
// hdhomerun_device_deallocator
//
struct hdhomerun_device_deallocator
{
  inline static void destroy(hdhomerun_device_t * hd)
  {
    if (hd)
    {
      hdhomerun_device_destroy(hd);
    }
  }
};

//----------------------------------------------------------------
// hdhomerun_devptr_t
//
typedef yae::shared_ptr<hdhomerun_device_t,
                        hdhomerun_device_t,
                        hdhomerun_device_deallocator> hdhomerun_devptr_t;



//----------------------------------------------------------------
// main_may_throw
//
int
main_may_throw(int argc, char ** argv)
{
  // discover HDHomeRun devices:
  std::vector<struct hdhomerun_discover_device_t> devices(64);

  int num_found =
    hdhomerun_discover_find_devices_custom_v2
    (0, // target_ip, 0 to auto-detect IP address(es)
     HDHOMERUN_DEVICE_TYPE_TUNER,
     HDHOMERUN_DEVICE_ID_WILDCARD,
     &(devices[0]),
     devices.size()); // max devices

  for (int i = 0; i < num_found; i++)
  {
    const hdhomerun_discover_device_t & found = devices[i];
    printf("hdhomerun device %08X found at %u.%u.%u.%u\n",
           (unsigned int)found.device_id,
           (unsigned int)(found.ip_addr >> 24) & 0x0FF,
           (unsigned int)(found.ip_addr >> 16) & 0x0FF,
           (unsigned int)(found.ip_addr >> 8) & 0x0FF,
           (unsigned int)(found.ip_addr >> 0) & 0x0FF);

    for (int tuner = 0; tuner < found.tuner_count; tuner++)
    {
      hdhomerun_devptr_t hd_ptr(hdhomerun_device_create(found.device_id,
                                                        found.ip_addr,
                                                        tuner,
                                                        NULL));

      if (!hd_ptr)
      {
        continue;
      }

      hdhomerun_device_t & hd = *hd_ptr;
      const char * name = hdhomerun_device_get_name(&hd);
      const char * model = hdhomerun_device_get_model_str(&hd);
      uint32_t target_addr = hdhomerun_device_get_local_machine_addr(&hd);

      printf("%s, id: %s, target addr: %u.%u.%u.%u",
             model,
             name,
             (unsigned int)(target_addr >> 24) & 0x0FF,
             (unsigned int)(target_addr >> 16) & 0x0FF,
             (unsigned int)(target_addr >> 8) & 0x0FF,
             (unsigned int)(target_addr >> 0) & 0x0FF);

      std::string cache_dir = yae::get_user_folder_path(".yaepg");
      YAE_ASSERT(yae::mkdir_p(cache_dir));

      std::string cache_path = (fs::path(cache_dir) / name).string();
      yae::TOpenFile cache_file(cache_path.c_str(), "rb");
      if (cache_file.is_open())
      {
        // load from cache
      }
      else
      {
        // scan channels
      }

      char * status_str = NULL;
      hdhomerun_tuner_status_t status = { 0 };
      if (hdhomerun_device_get_tuner_status(&hd,
                                            &status_str,
                                            &status) == 1)
      {
        printf(", status: %s", status_str);
      }

      char * owner_str = NULL;
      if (hdhomerun_device_get_tuner_lockkey_owner(&hd, &owner_str) == 1)
      {
        printf(", owner: %s", owner_str);
      }

      printf("\n");
    }
  }

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
