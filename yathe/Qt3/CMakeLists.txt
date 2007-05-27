cmake_minimum_required(VERSION 2.4.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
  $ENV{THE_SRC_DIR}
)
#MESSAGE("CMAKE_MODULE_PATH=[${CMAKE_MODULE_PATH}]")

#add_definitions(-DDEBUG_THREAD)
#add_definitions(-DDEBUG_MUTEX)
add_definitions(-DUSE_THE_TERMINATORS)
add_definitions(-DQT_THREAD_SUPPORT)

find_package(Qt3)
if (QT3_FOUND)
  include_directories(BEFORE
    ${QT3_INCLUDE_DIR}
    $ENV{THE_SRC_DIR}
    )
  
  set(moc3_hxx
    the_qt_trail.hxx	
    the_qt_view.hxx
    the_qt_thread.hxx
    )
  
  qt3_wrap_cpp(moc3_cxx ${moc3_hxx})
  
  add_library(the_ui_qt3 STATIC
    ${moc3_cxx}
    qimage_pixel_converter.cxx
    the_qt_input_device_event.cxx
    the_qt_thread.cxx
    the_qt_trail.cxx
    the_qt_view.cxx
    the_qt_mutex.cxx
    )
endif (QT3_FOUND)
