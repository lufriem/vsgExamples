set(SOURCES
    vsgbuilderinstancing.cpp
)

add_executable(vsgbuilderinstancing ${SOURCES})

target_link_libraries(vsgbuilderinstancing vsg::vsg)

if (vsgXchange_FOUND)
    target_compile_definitions(vsgbuilderinstancing PRIVATE vsgXchange_FOUND)
    target_link_libraries(vsgbuilderinstancing vsgXchange::vsgXchange)
endif()

install(TARGETS vsgbuilderinstancing RUNTIME DESTINATION bin)
