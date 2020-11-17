#!/usr/bin/awk -f

BEGIN {
	print "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
	print "<playlist xmlns=\"http://xspf.org/ns/0/\" xmlns:vlc=\"http://www.videolan.org/vlc/playlist/ns/0/\" version=\"1\">"
	print "	<title>Playlist</title>"
	print "	<trackList>"
	id = 0;
}

/^\[(.*)\]$/ {
	gsub("\\[|\\s*\\]", "")
	name=$0
	#printf "name: \"%s\"\n", name
}

/	FREQUENCY = / {
	freq = $3
	#printf "freq: \"%s\"\n", freq
}

/	SERVICE_ID = / {
	sid = $3
	#printf "sid: \"%s\"\n", sid
}

/	VIDEO_PID = / {
	vid = $3
	#printf "vid: \"%s\"\n", vid
}

/	DELIVERY_SYSTEM = / {
	printf "		<track>\n"
	printf "			<title>%s</title>\n", name
	printf "			<location>atsc://frequency=%s</location>\n", freq
	printf "			<extension application=\"http://www.videolan.org/vlc/playlist/0\">\n"
	printf "				<vlc:id>%d</vlc:id>\n", id
	printf "				<vlc:option>dvb-adapter=0</vlc:option>\n"
	printf "				<vlc:option>live-caching=300</vlc:option>\n"
	printf "				<vlc:option>program=%s</vlc:option>\n", sid
	printf "			</extension>\n"
	printf "		</track>\n"

	id ++
}

END {
	print "	</trackList>"
	print "</playlist>"
}
