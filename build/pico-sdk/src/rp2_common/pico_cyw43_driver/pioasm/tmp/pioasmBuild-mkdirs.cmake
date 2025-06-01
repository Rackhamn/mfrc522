# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/rackham/devel/pico-sdk/tools/pioasm"
  "/home/rackham/devel/iot_arch_project/iot_arch_project1/RnD/Rackhamn/mfrc522/build/pioasm"
  "/home/rackham/devel/iot_arch_project/iot_arch_project1/RnD/Rackhamn/mfrc522/build/pioasm-install"
  "/home/rackham/devel/iot_arch_project/iot_arch_project1/RnD/Rackhamn/mfrc522/build/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/tmp"
  "/home/rackham/devel/iot_arch_project/iot_arch_project1/RnD/Rackhamn/mfrc522/build/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src/pioasmBuild-stamp"
  "/home/rackham/devel/iot_arch_project/iot_arch_project1/RnD/Rackhamn/mfrc522/build/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src"
  "/home/rackham/devel/iot_arch_project/iot_arch_project1/RnD/Rackhamn/mfrc522/build/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src/pioasmBuild-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/rackham/devel/iot_arch_project/iot_arch_project1/RnD/Rackhamn/mfrc522/build/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src/pioasmBuild-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/rackham/devel/iot_arch_project/iot_arch_project1/RnD/Rackhamn/mfrc522/build/pico-sdk/src/rp2_common/pico_cyw43_driver/pioasm/src/pioasmBuild-stamp${cfgdir}") # cfgdir has leading slash
endif()
