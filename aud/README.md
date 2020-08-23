
# required host packages

host에 다음 패키지 설치 필요.

    # @ fedora
    # dnf install \
        pulseaudio-libs-devel \
        alsa-lib-devel
    
    # @ ubuntu
    # apt-get install \
        libasound2-dev \
        libpulse-dev

# 참고

* https://freedesktop.org/software/pulseaudio/doxygen/index.html
* test/pcm.c at https://git.alsa-project.org/?p=alsa-lib.git;a=summary
