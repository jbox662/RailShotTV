# RailShotPackaging.cmake
# Windows installer / deployment helpers (WiX / CPack).

include_guard(GLOBAL)

set(CPACK_PACKAGE_NAME "RailShotTV")
set(CPACK_PACKAGE_VENDOR "RailShotTV")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Billiards-first Windows livestreaming broadcast engine")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "RailShotTV")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/../docs/licensing/EULA.txt")
set(CPACK_GENERATOR "WIX;ZIP")
set(CPACK_WIX_UPGRADE_GUID "A1B2C3D4-E5F6-7890-ABCD-EF1234567890")
set(CPACK_WIX_PRODUCT_GUID "*")
set(CPACK_WIX_PROGRAM_MENU_FOLDER "RailShotTV")
set(CPACK_PACKAGE_EXECUTABLES "RailShotTV" "RailShotTV")

# Symbol / crash dump output directory convention
set(RAILSHOT_SYMBOLS_DIR "${CMAKE_BINARY_DIR}/symbols" CACHE PATH "PDB / symbol archive directory")

include(CPack OPTIONAL)
