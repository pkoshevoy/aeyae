cmake_minimum_required(VERSION 2.4.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
  $ENV{THE_SRC_DIR}
)
#MESSAGE("CMAKE_MODULE_PATH=[${CMAKE_MODULE_PATH}]")

find_package(Boost REQUIRED)
find_package(Qt4)
find_package(FLTK)
find_package(Cg)
find_package(GLEW)
find_package(ITK)
find_package(FFTW)

include_directories(AFTER
  $ENV{THE_SRC_DIR}
  ${Boost_INCLUDE_DIRS}
  )

#add_definitions(-DDEBUG_THREAD)
#add_definitions(-DDEBUG_MUTEX)

add_definitions(-DUSE_THE_TERMINATORS)

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
  utils/the_bit_tree.cxx
  utils/the_indentation.cxx
  utils/the_log.cxx
  utils/the_text.cxx
  utils/debug.cxx
  utils/histogram.cxx
  )

if (ITK_FOUND AND FFTW_FOUND)
  include(${ITK_USE_FILE})
  include_directories(AFTER ${FFTW_INCLUDE_DIR})
  add_definitions(-DUSE_ITK_TERMINATORS)
  
  add_library(the_itk STATIC
    itk/cluster.cxx
    itk/common.cxx
    itk/fft.cxx
    itk/fft_common.cxx
    itk/itkRBFTransform.cxx
    itk/itkRegularStepGradientDescentOptimizer2.cxx
    itk/stos_common.cxx
    itk/grid_common.cxx
    itk/mosaic_layout_common.cxx
    itk/the_grid_transform.cxx
    )
endif (ITK_FOUND AND FFTW_FOUND)

if (GLEW_FOUND)
  include_directories(AFTER ${GLEW_INCLUDE_DIR})

  if (Cg_FOUND)
    add_definitions(-DUSE_CG)
    include_directories(AFTER ${Cg_INCLUDE_DIR})
  endif (Cg_FOUND)
  
  add_library(the_ui STATIC
    doc/the_document.cxx
    doc/the_document_so.cxx
    doc/the_graph.cxx
    doc/the_primitive.cxx
    doc/the_reference.cxx
    doc/the_registry.cxx
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
    io/the_file_io.cxx
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
    ui/the_cursor.cxx
    ui/the_desktop_metrics.cxx
    ui/the_document_ui.cxx
    ui/the_trail.cxx
    )
endif (GLEW_FOUND)

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
endif (QT4_FOUND)

if (FLTK_FOUND)
  include_directories(AFTER ${FLTK_INCLUDE_DIR})
  
  add_library(the_ui_fltk STATIC
    FLTK/Fl_Knot_Vector.cxx
    FLTK/the_fltk_input_device_event.cxx
    FLTK/the_fltk_std_widgets.cxx
    FLTK/the_fltk_trail.cxx
    FLTK/the_fltk_view.cxx
    )
endif (FLTK_FOUND)
