@echo off
REM Compile script for OpenGL project with GLAD+GLFW
g++ main.cpp src/glad.c -o main -I./include -L./lib -lglfw3 -lopengl32 -lglu32 -lgdi32 -luser32 -lshell32 -lkernel32
if %ERRORLEVEL% EQU 0 (
    echo Build successful! Run with: .\main
) else (
    echo Build failed!
)
