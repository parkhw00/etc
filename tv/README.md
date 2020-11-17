
    dnf install dtv-scan-tables v4l-utils
    mkdir tv && tv
    sed -e 's/DVBC\/ANNEX_B/ATSC/' -e 's/QAM\/256/VSB\/8/' /usr/share/dvbv5/atsc/us-Cable-Standard-center-frequencies-QAM256 > ko-Cable-8VSB
    dvbv5-scan ko-Cable-8VSB
    ...

