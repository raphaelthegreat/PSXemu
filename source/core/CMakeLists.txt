# Add the executables
add_executable(${PROJECT_NAME} "${SRC_DIR}/main.cpp"
							   "emulator.cpp"
							   "${SRC_DIR}/cpu/cpu.cpp"
							   "${SRC_DIR}/cpu/gte.cpp"
							   "${SRC_DIR}/cpu/gte_opcodes.cpp"
							   "${SRC_DIR}/cpu/opcode.cpp"
							   "${SRC_DIR}/devices/cdrom_disk.cpp"
							   "${SRC_DIR}/devices/cdrom_drive.cpp"
							   "${SRC_DIR}/devices/controller.cpp"
							   "${SRC_DIR}/devices/timer.cpp"
							   "${SRC_DIR}/memory/bios.cpp"
							   "${SRC_DIR}/memory/bus.cpp"
							   "${SRC_DIR}/memory/dma.cpp"
							   "${SRC_DIR}/memory/ram.cpp"
							   "${SRC_DIR}/tools/cpu_widget.cpp"
							   "${SRC_DIR}/tools/debugger.cpp"
							   "${SRC_DIR}/video/opengl/shader.cpp"
							   "${SRC_DIR}/video/opengl/stb_image.cpp"
							   "${SRC_DIR}/video/gp1.cpp"
							   "${SRC_DIR}/video/gp0.cpp"
							   "${SRC_DIR}/video/gpu_core.cpp"
							   "${SRC_DIR}/video/renderer.cpp"
							   "${SRC_DIR}/video/vram.cpp")

# Link dependecies
target_link_libraries(${PROJECT_NAME} "glfw" "${GLFW_LIBRARIES}")
target_link_libraries(${PROJECT_NAME}  glad)
target_link_libraries(${PROJECT_NAME}  glm)
target_link_libraries(${PROJECT_NAME} "imgui")