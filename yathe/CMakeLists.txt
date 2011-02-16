cmake_minimum_required(VERSION 2.8.0)

project(THE)

file(TO_CMAKE_PATH "$ENV{THE_SRC_DIR}" THE_SRC_DIR)
file(TO_CMAKE_PATH "$ENV{THE_BIN_DIR}" THE_BIN_DIR)

set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${THE_SRC_DIR}
)

find_package(Boost REQUIRED)
include_directories(AFTER
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${THE_SRC_DIR}
  ${Boost_INCLUDE_DIR}
  )

#add_definitions(-DDEBUG_THREAD)
#add_definitions(-DDEBUG_MUTEX)

add_definitions(-DUSE_THE_TERMINATORS)
if (WIN32)
  ADD_DEFINITIONS(-D_USE_MATH_DEFINES)
  ADD_DEFINITIONS(-DNOMINMAX)
endif (WIN32)

add_library(the_core STATIC
  math/m4x4.cxx
  math/the_aa_bbox.cxx
  math/the_bbox.cxx
  math/the_camera.cxx
  math/the_color_blend.cxx
  math/the_color.cxx
  math/the_coord_sys.cxx
  math/the_deviation.cxx
  math/the_plane.cxx
  math/the_ray.cxx
  math/the_transform.cxx
  math/the_view_volume.cxx
  thread/the_mutex_interface.cxx
  thread/the_terminator.cxx
  thread/the_thread_interface.cxx
  thread/the_thread_pool.cxx
  thread/the_thread_storage.cxx
  thread/the_transaction.cxx
  thread/the_boost_mutex.hxx
  thread/the_boost_mutex.cxx
  thread/the_boost_thread.hxx
  thread/the_boost_thread.cxx
  thread/the_boost_thread_storage.hxx
  utils/debug.cxx
  utils/debug.hxx
  utils/histogram.cxx
  utils/histogram.hxx
  utils/the_array.hxx
  utils/the_bit_tree.cxx
  utils/the_bit_tree.hxx
  utils/the_dlink.hxx
  utils/the_dloop.hxx
  utils/the_domain_array.hxx
  utils/the_dynamic_array.hxx
  utils/the_exception.hxx
  utils/the_fifo.hxx
  utils/the_indentation.cxx
  utils/the_indentation.hxx
  utils/the_log.cxx
  utils/the_log.hxx
  utils/the_text.cxx
  utils/the_text.hxx
  utils/the_unique_list.hxx
  utils/the_utils.cxx
  utils/the_utils.hxx
  utils/the_benchmark.cxx
  utils/the_benchmark.hxx
  utils/the_walltime.cxx
  utils/the_walltime.hxx
  utils/instance_method_call.hxx
  utils/instance_method_call.cxx
  io/io_base.hxx
  io/io_base.cxx
  io/the_file_io.hxx
  io/the_file_io.cxx
  doc/the_registry.hxx
  doc/the_registry.cxx
  doc/the_graph.hxx
  doc/the_graph.cxx
  doc/the_graph_node.hxx
  doc/the_graph_node.cxx
  doc/the_graph_node_ref.hxx
  doc/the_graph_node_ref.cxx
  )

install(TARGETS the_core DESTINATION lib)

find_package(GLEW)
if (GLEW_FOUND)
  find_package(OpenGL REQUIRED)

  include_directories(AFTER ${GLEW_INCLUDE_DIR})

#  find_package(Cg)
#  if (Cg_FOUND)
#    add_definitions(-DUSE_CG)
#    include_directories(AFTER ${Cg_INCLUDE_DIR})
#  endif (Cg_FOUND)
  
  add_library(the_ui STATIC
    doc/the_document.cxx
    doc/the_document_so.cxx
    doc/the_primitive.cxx
    doc/the_reference.cxx
    eh/the_input_device.cxx
    eh/the_input_device_eh.cxx
    eh/the_input_device_event.cxx
    eh/the_keybd_device.cxx
    eh/the_mouse_device.cxx
    eh/the_view_mgr_eh.cxx
    eh/the_wacom_device.cxx
    geom/the_bspline.cxx
    geom/the_curve.cxx
    geom/the_grid.cxx
    geom/the_point.cxx
    geom/the_polyline.cxx
    geom/the_rational_bezier.cxx
    geom/the_tensurf.cxx
    geom/the_triangle_mesh.cxx
    image/image_tile.cxx
    image/image_tile_generator.cxx
    image/texture.cxx
    image/texture_data.cxx
    opengl/glsl.cxx
    opengl/image_tile_dl_elem.cxx
    opengl/OpenGLCapabilities.cpp
    opengl/the_appearance.cxx
    opengl/the_ascii_font.cxx
    opengl/the_disp_list.cxx
    opengl/the_ep_grid.cxx
    opengl/the_font.cxx
    opengl/the_gl_context.cxx
    opengl/the_palette.cxx
    opengl/the_point_symbols.cxx
    opengl/the_symbols.cxx
    opengl/the_view.cxx
    opengl/the_view_mgr.cxx
    opengl/the_view_mgr_orientation.cxx
    sel/the_pick_list.cxx
    sel/the_pick_rec.cxx
    sel/the_selset.cxx
    sel/the_curve_selset.cxx
    ui/the_cursor.cxx
    ui/the_desktop_metrics.cxx
    ui/the_document_ui.cxx
    ui/the_trail.cxx
    )

  install(TARGETS the_ui DESTINATION lib)

  find_package(Qt4)
  if (QT4_FOUND)
    include(${QT_USE_FILE})
    
    include_directories(AFTER
      ${QT_INCLUDE_DIR}
      ${QT_QTOPENGL_INCLUDE_DIR}
      )
    
    set(moc_hxx
      Qt/the_qt_trail.hxx	
      Qt/the_qt_view.hxx
      Qt/the_qt_thread.hxx
      Qt/the_qt_thread_pool.hxx
      )
    
    qt4_wrap_cpp(moc_cxx ${moc_hxx})
    
    add_library(the_ui_qt STATIC
      ${moc_cxx}
      Qt/qimage_pixel_converter.cxx
      Qt/the_qt_input_device_event.cxx
      Qt/the_qt_thread.cxx
      Qt/the_qt_thread_pool.cxx
      Qt/the_qt_trail.cxx
      Qt/the_qt_view.cxx
      Qt/the_qt_mutex.cxx
      )
    
    install(TARGETS the_ui_qt DESTINATION lib)
  endif (QT4_FOUND)
  
  find_package(FLTK)
  if (FLTK_FOUND)
    include_directories(AFTER ${FLTK_INCLUDE_DIR})
    
    add_library(the_ui_fltk STATIC
      FLTK/Fl_Knot_Vector.cxx
      FLTK/the_fltk_input_device_event.cxx
      FLTK/the_fltk_std_widgets.cxx
      FLTK/the_fltk_trail.cxx
      FLTK/the_fltk_view.cxx
      )
    
    install(TARGETS the_ui_fltk DESTINATION lib)
  endif (FLTK_FOUND)
  
  find_package(Qt3)
  if (QT3_FOUND)
    add_definitions(-DQT_THREAD_SUPPORT)
    
    include_directories(BEFORE
      ${QT3_INCLUDE_DIR}
      )
    
    set(moc3_hxx
      Qt3/the_qt_trail.hxx	
      Qt3/the_qt_view.hxx
      )
    
    qt3_wrap_cpp(moc3_cxx ${moc3_hxx})
    
    add_library(the_ui_qt3 STATIC
      ${moc3_cxx}
      Qt3/qimage_pixel_converter.cxx
      Qt3/the_qt_input_device_event.cxx
      Qt3/the_qt_thread.cxx
      Qt3/the_qt_trail.cxx
      Qt3/the_qt_view.cxx
      Qt3/the_qt_mutex.cxx
      )
    
    install(TARGETS the_ui_qt3 DESTINATION lib)
  endif (QT3_FOUND)
endif (GLEW_FOUND)
