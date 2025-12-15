# g++ .\tagpro_renderer.cpp -o test.exe -I C:\ffmpeg\ffmpeg-8.0.1-full_build-shared\include -L C:\ffmpeg\ffmpeg-8.0.1-full_build-shared\lib -l :libavutil.dll.a -l ws2_32 -l secur32 -lbcrypt

# FFmpeg + MinGW environment setup
$FFMPEG_ROOT = "C:\ffmpeg\ffmpeg-8.0.1-full_build-shared"

# Add FFmpeg DLLs to PATH for runtime
$env:PATH = "$FFMPEG_ROOT\bin;$env:PATH"

# Add FFmpeg include path
$env:INCLUDE = "$FFMPEG_ROOT\include;$env:INCLUDE"

# Add FFmpeg lib path
$env:LIB = "$FFMPEG_ROOT\lib;$env:LIB"

Write-Host "FFmpeg environment ready"
