# IMGUI
add_library("imgui" "${IMGUI_DIR}/imgui.cpp"
	                        "${IMGUI_DIR}/imgui_demo.cpp"
	                        "${IMGUI_DIR}/imgui_impl_opengl3.cpp"
	                        "${IMGUI_DIR}/imgui_impl_glfw.cpp"
	                        "${IMGUI_DIR}/imgui_draw.cpp"
	                        "${IMGUI_DIR}/imgui_widgets.cpp")

target_link_libraries("imgui" glad)
target_link_libraries("imgui" "glfw" "${GLFW_LIBRARIES}")