if(NOT APPLE OR IOS)
  return()
endif()

juce_add_gui_app(vmpc2000xl_GuiLab
  BUNDLE_ID nl.izmar.vmpc2000xl.guilab
  COMPANY_NAME Izmar
  ICON_BIG "${CMAKE_SOURCE_DIR}/src/main/resources/icon.png"
  PRODUCT_NAME "VMPC2000XL GUI Lab")

target_sources(vmpc2000xl_GuiLab PRIVATE
  "${CMAKE_SOURCE_DIR}/src/main/gui/arrangement/ArrangementCatalog.cpp"
  "${CMAKE_SOURCE_DIR}/src/main/gui/arrangement/ArrangementCatalog.hpp"
  "${CMAKE_SOURCE_DIR}/src/main/gui/arrangement/ArrangementModel.cpp"
  "${CMAKE_SOURCE_DIR}/src/main/gui/arrangement/ArrangementModel.hpp"
  "${CMAKE_SOURCE_DIR}/src/guilab/Main.cpp"
  "${CMAKE_SOURCE_DIR}/src/guilab/GuiLabComponent.cpp"
  "${CMAKE_SOURCE_DIR}/src/guilab/GuiLabComponent.hpp"
  "${CMAKE_SOURCE_DIR}/src/main/gui/arrangement/PreviewLcd.cpp"
  "${CMAKE_SOURCE_DIR}/src/main/gui/arrangement/PreviewLcd.hpp"
  "${CMAKE_SOURCE_DIR}/src/main/gui/arrangement/PreviewViewUtil.cpp"
  "${CMAKE_SOURCE_DIR}/src/main/gui/arrangement/PreviewViewUtil.hpp"
  "${CMAKE_SOURCE_DIR}/src/main/VmpcJuceResourceUtil.cpp"
  "${CMAKE_SOURCE_DIR}/src/main/VmpcJuceResourceUtil.hpp"
  "${CMAKE_SOURCE_DIR}/src/main/gui/vector/Constants.cpp"
  "${CMAKE_SOURCE_DIR}/src/main/gui/vector/FlexBoxWrapper.cpp"
  "${CMAKE_SOURCE_DIR}/src/main/gui/vector/GridWrapper.cpp"
  "${CMAKE_SOURCE_DIR}/src/main/gui/vector/SvgWithLabelGrid.cpp")

target_include_directories(vmpc2000xl_GuiLab PRIVATE
  "${CMAKE_SOURCE_DIR}/src/main"
  "${CMAKE_SOURCE_DIR}/src/guilab"
  "${CMAKE_CURRENT_BINARY_DIR}/generated"
  "${mpc_SOURCE_DIR}/src/main")

target_compile_definitions(vmpc2000xl_GuiLab PRIVATE
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:vmpc2000xl_GuiLab,JUCE_PRODUCT_NAME>"
  JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:vmpc2000xl_GuiLab,JUCE_VERSION>")

target_link_libraries(vmpc2000xl_GuiLab PRIVATE
  juce::juce_gui_extra
  nlohmann_json::nlohmann_json
  melatonin_blur
  mpc
  juce::juce_recommended_config_flags
  juce::juce_recommended_lto_flags
  juce::juce_recommended_warning_flags
  "-framework Carbon")

file(GLOB_RECURSE _guilab_vmpc_resources
  "${CMAKE_SOURCE_DIR}/resources/*")
list(FILTER _guilab_vmpc_resources EXCLUDE REGEX "\\.DS_Store$")

foreach(_resource IN LISTS _guilab_vmpc_resources)
  file(RELATIVE_PATH _relative_path "${CMAKE_SOURCE_DIR}/resources" "${_resource}")
  get_filename_component(_relative_dir "${_relative_path}" DIRECTORY)
  set_source_files_properties("${_resource}" PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources/${_relative_dir}")
endforeach()

set(_guilab_lcd_fixture
  "${mpc_SOURCE_DIR}/resources/screens/bg/sequencer.png")
set_source_files_properties("${_guilab_lcd_fixture}" PROPERTIES
  MACOSX_PACKAGE_LOCATION "Resources/screens/bg")

target_sources(vmpc2000xl_GuiLab PRIVATE
  ${_guilab_vmpc_resources}
  "${_guilab_lcd_fixture}")

set_target_properties(vmpc2000xl_GuiLab PROPERTIES
  XCODE_GENERATE_SCHEME TRUE
  SKIP_BUILD_RPATH FALSE
  BUILD_RPATH "${CMAKE_BINARY_DIR}/_deps/json-schema-validator-build/$<CONFIG>;${CMAKE_BINARY_DIR}/_deps/mpc-build/$<CONFIG>")

source_group("GUI Lab" FILES
  "${CMAKE_SOURCE_DIR}/src/main/gui/arrangement/ArrangementCatalog.cpp"
  "${CMAKE_SOURCE_DIR}/src/main/gui/arrangement/ArrangementCatalog.hpp"
  "${CMAKE_SOURCE_DIR}/src/main/gui/arrangement/ArrangementModel.cpp"
  "${CMAKE_SOURCE_DIR}/src/main/gui/arrangement/ArrangementModel.hpp"
  "${CMAKE_SOURCE_DIR}/src/guilab/Main.cpp"
  "${CMAKE_SOURCE_DIR}/src/guilab/GuiLabComponent.cpp"
  "${CMAKE_SOURCE_DIR}/src/guilab/GuiLabComponent.hpp"
  "${CMAKE_SOURCE_DIR}/src/main/gui/arrangement/PreviewLcd.cpp"
  "${CMAKE_SOURCE_DIR}/src/main/gui/arrangement/PreviewLcd.hpp"
  "${CMAKE_SOURCE_DIR}/src/main/gui/arrangement/PreviewViewUtil.cpp"
  "${CMAKE_SOURCE_DIR}/src/main/gui/arrangement/PreviewViewUtil.hpp")

if(VMPC_BUILD_TESTS)
  add_executable(vmpc-juce-guilab-tests
    "${CMAKE_SOURCE_DIR}/src/guilab/test/GuiLabArrangementModelTest.cpp"
    "${CMAKE_SOURCE_DIR}/src/main/gui/arrangement/ArrangementCatalog.cpp"
    "${CMAKE_SOURCE_DIR}/src/main/gui/arrangement/ArrangementModel.cpp")

  target_include_directories(vmpc-juce-guilab-tests PRIVATE
    "${CMAKE_SOURCE_DIR}/src/main")

  target_link_libraries(vmpc-juce-guilab-tests PRIVATE
    Catch2::Catch2WithMain
    juce::juce_core
    nlohmann_json::nlohmann_json
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

  target_compile_definitions(vmpc-juce-guilab-tests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)

  set_target_properties(vmpc-juce-guilab-tests PROPERTIES
    XCODE_GENERATE_SCHEME TRUE)

  source_group("GUI Lab Tests" FILES
    "${CMAKE_SOURCE_DIR}/src/guilab/test/GuiLabArrangementModelTest.cpp")
endif()
