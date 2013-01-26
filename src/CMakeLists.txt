ADD_LIBRARY( softdsp SHARED softdsp.cpp )
target_link_libraries( softdsp ${Boost_LIBRARIES} ${LLVM_LIBS} ${LIBDL_LIBRARIES} )
set_target_properties( softdsp PROPERTIES
	  VERSION 0.0.1
	  SOVERSION 0.0.1
	)
ADD_EXECUTABLE( sample sample.cpp )
target_link_libraries( sample softdsp )
install( TARGETS softdsp
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
)
