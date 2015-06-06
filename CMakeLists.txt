cmake_minimum_required(VERSION 3.1.0)

project(aeyae)

include_directories(AFTER
  .
  apprenticevideo
  )

add_library(aeyae SHARED

  yae/api/aeyae_api.hxx
  yae/api/aeyae_frame_observer_interface.hxx
  yae/api/aeyae_message_carrier_interface.hxx
  yae/api/aeyae_plugin_interface.hxx
  yae/api/aeyae_settings.hxx
  yae/api/aeyae_settings_interface.hxx

  yae/utils/aeyae_benchmark.cxx
  yae/utils/aeyae_benchmark.hxx
  yae/utils/aeyae_log.hxx
  yae/utils/aeyae_shared_ptr.hxx
  yae/utils/aeyae_type_name.hxx
  )

add_subdirectory(tests)

add_subdirectory(apprenticevideo)

set_property(TARGET aeyae PROPERTY CXX_STANDARD 11)
