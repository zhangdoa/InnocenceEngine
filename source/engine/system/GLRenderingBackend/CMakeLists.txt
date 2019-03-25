aux_source_directory(./ DIR_SRCS)
add_library(InnoGLRenderingBackend ${DIR_SRCS})
target_link_libraries(InnoGLRenderingBackend ${OPENGL_gl_LIBRARY})