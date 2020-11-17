# 한국 cable DTV

한국 케이블 DTV 표춘은 US cable frequency table 을 사용하면서, modulation 은 VSB8 을 사용하는듯 하다. 다음과 같이 US Cable Standard scan table 에서 modulation 을 VSB8로 바꾸고 dvbv5-scan 하면 채널 검색이 된다.

    dnf install dtv-scan-tables v4l-utils
    mkdir tv && tv
    sed -e 's/DVBC\/ANNEX_B/ATSC/' \
        -e 's/QAM\/256/VSB\/8/' \
        /usr/share/dvbv5/atsc/us-Cable-Standard-center-frequencies-QAM256 > ko-Cable-8VSB
    dvbv5-scan ko-Cable-8VSB
    ...
    ./dvbv5_to_vlc.awk dvb_channel.conf > dvb_channel.xspf

채널 목록은 dvb_channel.conf 에 저장되는데, vlc 가 그중 DTV 를 보기 괜찮은듯..
